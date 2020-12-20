/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* This is the STIM backend RTAI+comedi module which generates one of
   the selected directional hearing stimulus patterns designed in one of
   Octopus stimulus design tools. Currently the only pattern available is the
   random IID and ITD trade-off pattern.
   The patterns get transferred from userspace via SHM, and applied
   binaurally over NI PCI-6711 DAC card, synchronously sending the events
   via parallel port to ACQ.

   This backend also has a "can=Calibration And Normalization" mod, which
   can be used to normalize the 128 channel amplifier's per-channel DC, and
   amplifier gain values, that can be saved to a compensation file in the
   operator computer.

   The commands to this backend can be given via the Front-to-Back FIFO,
   over the STIM daemon listening to a TCP port given.

   Independent test and paradigm routines scheduled in main realtime thread
   are implemented in separate header files. This enables the developers
   to implement new paradigms in a more abstract way, without touching the
   main stim engine. */

#define OCTOPUS_STIM_COMEDI

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_math.h>
#include <linux/delay.h>

#ifdef OCTOPUS_STIM_COMEDI
#include <linux/kcomedilib.h>
#endif

#include "../fb_command.h"
#include "../octopus-stim.h"
#include "../stim_event_codes.h"
#include "../stim_test_para.h"

/* ========================================================================= */

#define M_PI (double)(3.141519)

#ifdef OCTOPUS_STIM_COMEDI
static comedi_t *daqcard; static sampl_t dac_0,dac_1;
#else
static unsigned short dac_0,dac_1;
#endif

static char *himem_buffer,*patt_shm; static fb_command fb_msg,bf_msg;
static RT_TASK audio_task,trigger_task;
static RTIME tickperiod; static SEM audio_sem,trigger_sem;
static int audio_rate=AUDIO_RATE;
static int trigger_rate=TRIG_RATE;
static char current_pattern_data; static int current_pattern_offset=0;
static int audio_active=0,audio_paused=0,trigger_active=0,paradigm=0,lp0_data=0;
static int pattern_size,pattern_size_dummy;

/* ========================================================================= */

static int counter0,counter1,counter2,counter3,counter4,counter5,
           event0,event1,event2,event3,event4,event5,
           var0,var1,var2,var3,var4,var5;
static double theta=0.0;
static int i=0,pause_trigger_hi=0;

/* ========================================================================= */

/* TESTS */
#include "test_calibration.h"
#include "test_sinecosine.h"
#include "test_square.h"
#include "test_steadysquare.h"

/* PARADIGMS */
#include "para_click.h"
#include "para_squareburst.h"
#include "para_iiditd.h"
#include "para_iiditdpplp.h"
#include "para_itdoppchn_v1.h"

/* ========================================================================= */

static void audio_thread(int t) {
 while (1) {
  if (audio_active) {
   rt_sem_wait(&audio_sem);
    switch (paradigm) {
     case TEST_CALIBRATION:   test_calibration();  break;
     default:
     case TEST_SINECOSINE:    test_sinecosine();   break;
     case TEST_SQUARE:        test_square();       break;
     case TEST_STEADYSQUARE:  test_steadysquare(); break;
     case PARA_CLICK:         para_click();        break;
     case PARA_SQUAREBURST:   para_squareburst();  break;
     case PARA_IIDITD:        para_iiditd();       break;
     case PARA_IIDITD_PPLP:   para_iiditdpplp();   break;
     case PARA_ITD_OPPCHN_V1: para_itdoppchn_v1(); break;
    }
#ifdef OCTOPUS_STIM_COMEDI
    comedi_data_write(daqcard,1,0,0,AREF_GROUND,DACZERO+dac_0); // L
    comedi_data_write(daqcard,1,1,0,AREF_GROUND,DACZERO+dac_1); // R
#endif
   rt_sem_signal(&audio_sem);
  } /* audio_active */
  rt_task_wait_period();
 }
}

static void trigger_thread(int t) {
 while (1) {
  if (trigger_active) {
   rt_sem_wait(&trigger_sem);
//#ifdef OCTOPUS_STIM_COMEDI
//    comedi_data_write(daqcard,2,0,0); // Trigger
//    comedi_data_write(daqcard,2,1,1); // Code
//#endif
   rt_sem_signal(&trigger_sem);
  } /* trigger_active */
  rt_task_wait_period();
 }
}

/* ========================================================================= */

static void stim_reset(void) {
 dac_0=dac_1=DACZERO; current_pattern_offset=0;
#ifdef OCTOPUS_STIM_COMEDI
 comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
 comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);
 comedi_dio_write(daqcard,2,0,0);
 comedi_dio_write(daqcard,2,1,0);
#endif
}

static void init_test_para(int tp) {
 switch (tp) {
  case TEST_CALIBRATION:   test_calibration_init();  break;
  case TEST_SINECOSINE:    test_sinecosine_init();   break;
  case TEST_SQUARE:        test_square_init();       break;
  case TEST_STEADYSQUARE:  test_steadysquare_init(); break;
  case PARA_CLICK:         para_click_init();        break;
  case PARA_SQUAREBURST:   para_squareburst_init();  break;
  case PARA_IIDITD:        para_iiditd_init();       break;
  case PARA_IIDITD_PPLP:   para_iiditdpplp_init();   break;
  case PARA_ITD_OPPCHN_V1: para_itdoppchn_v1_init(); break;
 }
}

/* ========================================================================= */

int fbfifohandler(unsigned int fifo,int rw) {
 if (rw=='w') {
  rtf_get(FBFIFO,&fb_msg,sizeof(fb_command));
  switch (fb_msg.id) {
   case STIM_SET_PARADIGM: audio_active=0; dac_0=dac_1=DACZERO;
#ifdef OCTOPUS_STIM_COMEDI
                           comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
                           comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);
			   comedi_dio_write(daqcard,2,0,0);
			   comedi_dio_write(daqcard,2,1,0);
#endif
                           paradigm=fb_msg.iparam[0];
                           rt_printk("octopus-stim-backend.o: Para -> %d.\n",
                                     paradigm);
                           init_test_para(paradigm);
                           rt_sem_wait(&audio_sem);
                            rtf_reset(BFFIFO); rtf_reset(FBFIFO);
                           rt_sem_signal(&audio_sem);
                           break;
   case STIM_LOAD_PATTERN: pattern_size=fb_msg.iparam[0]; /* Byte count */
                           pattern_size_dummy=0;	/* Current index */
                           break;
   case STIM_PATT_XFER_SYN:for (i=0;i<fb_msg.iparam[0];i++) /* Burst count */
                            himem_buffer[pattern_size_dummy++]=patt_shm[i];

			   bf_msg.id=STIM_PATT_XFER_ACK;
			   bf_msg.iparam[0]=fb_msg.iparam[0];
                           rtf_put(BFFIFO,&bf_msg,sizeof(fb_command));
                           /* Burst acknowledged! */

			   if (pattern_size==pattern_size_dummy) /* Finished? */
                            rt_printk(
 "octopus-stim-backend.o: Stimulus pattern Xferred to backend successfully.\n");
                           break;
   case STIM_START:        audio_paused=0; audio_active=1;
                 rt_printk("octopus-stim-backend.o: Stim resumed/started.\n");
                           break;
   case STIM_PAUSE:        pause_trigger_hi=0; audio_paused=1;
                           rt_printk("octopus-stim-backend.o: Stim paused.\n");
                           break;
   case STIM_STOP:         audio_active=lp0_data=0;
                           stim_reset();
                           init_test_para(paradigm);
#ifdef OCTOPUS_STIM_COMEDI
                           comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
                           comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);
			   comedi_dio_write(daqcard,2,0,0);
			   comedi_dio_write(daqcard,2,1,0);
#endif
                           rt_sem_wait(&audio_sem);
                            rtf_reset(BFFIFO); rtf_reset(FBFIFO);
                           rt_sem_signal(&audio_sem);
                           rt_printk(
			    "octopus-stim-backend.o: Audio stopped.\n");
                           break;
   case TRIG_START:        trigger_active=1;
                           rt_printk(
			    "octopus-stim-backend.o: Trigger started.\n");
                           break;
   case TRIG_STOP:         trigger_active=0;
                           rt_printk(
			    "octopus-stim-backend.o: Trigger stopped.\n");
                           break;
   case STIM_RST_SYN:      audio_active=lp0_data=0;
                           rt_sem_wait(&audio_sem);
                            rtf_reset(BFFIFO); rtf_reset(FBFIFO);
                            stim_reset();
                           rt_sem_signal(&audio_sem);
                           bf_msg.id=STIM_RST_ACK;
                           bf_msg.iparam[0]=bf_msg.iparam[1]=0;
                           rtf_put(BFFIFO,&bf_msg,sizeof(fb_command));
                        rt_printk("octopus-stim-backend.o: Backend reset.\n");
                           break;
   case STIM_SYNTH_EVENT:  //trigger_set(fb_msg.iparam[0]);
			   //outb(0x80|(fb_msg.iparam[0] & 0x7f),0x378);
                           msleep(1);
                           //trigger_reset();;
   default:                break;
  }
 }
 return 0;
}

/* ========================================================================= */

static int __init octopus_stim_init(void) {
 /* Initialize comedi devices for auditory stimulus presentation and  */
#ifdef OCTOPUS_STIM_COMEDI
 daqcard=comedi_open("/dev/comedi0"); comedi_lock(daqcard,1);
 comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
 comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);
 comedi_dio_config(daqcard,2,0,COMEDI_OUTPUT); /* Trigger direction -trig */
 comedi_dio_config(daqcard,2,1,COMEDI_OUTPUT); /* Trigger direction -code */
 comedi_dio_write(daqcard,2,0,0); /* Set low */
 comedi_dio_write(daqcard,2,1,0);
 rt_printk("octopus-stim-backend.o: Comedi Device Allocation successful. ->\n");
 rt_printk("octopus-stim-backend.o:  (daqcard=0x%p).\n",daqcard);
#else
 dac_0=dac_1=DACZERO;
 rt_printk("octopus-stim-backend.o: Diagnostic mode.. no COMEDI..\n");
#endif

 /* Initialize SHM Buffer for Pattern Transfer */
 himem_buffer=ioremap(HIMEMSTART,2*0x100000); /* 2M is enough */
 rt_printk("octopus-stim-backend.o: Himem dedicated mem: (0x%x).\n",
           himem_buffer);
 patt_shm=rtai_kmalloc('PATT',SHMBUFSIZE);
 rt_printk("octopus-stim-backend.o: Pattern transfer SHM allocated (0x%x).\n",
           patt_shm);

 /* Initialize transfer fifos */
 rtf_create_using_bh(FBFIFO,1000*sizeof(fb_command),0);
 rtf_create_using_bh(BFFIFO,1000*sizeof(fb_command),0);
 rtf_create_handler(FBFIFO,(void*)&fbfifohandler);
 rt_printk(
  "octopus-stim-backend.o: Up&downstream transfer FIFOs initialized.\n");

 /* Initialize auditory stimulation task */
 rt_task_init(&audio_task,(void *)audio_thread,0,2000,0,1,0);
 tickperiod=start_rt_timer(nano2count(1e9/audio_rate));
 rt_task_make_periodic(&audio_task,rt_get_time()+tickperiod,tickperiod);
 rt_printk(
  "octopus-stim-backend.o: Auditory Stimulus task initialized (50 KHz).\n");

 /* Initialize trigger task */
 rt_task_init(&trigger_task,(void *)trigger_thread,0,2000,0,1,0);
 tickperiod=start_rt_timer(nano2count(1e9/trigger_rate));
 rt_task_make_periodic(&trigger_task,rt_get_time()+tickperiod,tickperiod);
 rt_printk("octopus-stim-backend.o: Trigger task initialized (9600 Hz).\n");

 return 0;
}

static void __exit octopus_stim_exit(void) {
 /* Delete stimulus task */
 stop_rt_timer();
 rt_busy_sleep(1e7);
 rt_task_delete(&trigger_task);
 rt_printk("octopus-stim-backend.o: Trigger task disposed.\n");
 rt_task_delete(&audio_task);
 rt_printk("octopus-stim-backend.o: Audio task disposed.\n");

 /* Dispose transfer fifos */
 rtf_create_handler(FBFIFO,(void*)&fbfifohandler);
 rtf_destroy(BFFIFO); rtf_destroy(FBFIFO);
 rt_printk("octopus-stim-backend.o: Up&downstream transfer FIFOs disposed.\n");

 /* Dispose SHM Buffer */
 rtai_kfree('PATT');
 rt_printk("octopus-stim-backend.o: Pattern transfer SHM disposed.\n");
 iounmap(himem_buffer);
 rt_printk("octopus-stim-backend.o: Himem dedicated memory disposed.\n");

 dac_0=dac_1=DACZERO;
#ifdef OCTOPUS_STIM_COMEDI
 comedi_dio_write(daqcard,2,0,0);
 comedi_dio_write(daqcard,2,1,0);
 comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
 comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);

 comedi_unlock(daqcard,1);
 comedi_close(daqcard);
 rt_printk("octopus-stim-backend.o: Comedi DAQcard freed.\n");
//#else
// rt_printk("octopus-stim-backend.o: STIM Backend exited successfully.\n");
#endif
}

module_init(octopus_stim_init);
module_exit(octopus_stim_exit);

MODULE_AUTHOR("Barkin Ilhan (barkin@unrlabs.org)");
MODULE_DESCRIPTION("Octopus Auditory STIM Backend v0.8");
MODULE_LICENSE("GPL"); 
