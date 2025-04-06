/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

   This backend also has a "CaN=Calibration and Normalization" mod, which
   can be used to normalize the 128 channel amplifier's per-channel DC, and
   amplifier gain values, that can be saved to a compensation file in the
   operator computer.

   The commands to this backend can be given via the Front-to-Back FIFO,
   over the STIM daemon listening to a TCP port given.

   Independent test and paradigm routines scheduled in main realtime thread
   are implemented in separate header files. This enables the developers
   to implement new paradigms in a more abstract way, without touching the
   main stim engine.

 -------- Improvements after December 2020: --------
   We started an attempt for generalizing the octopus-stim structure
   for different types of future bi-lateral stimulus types. Accordingly,
   system now is more tailored to NI DAQCard 6036E, which has two 16-bit
   D/A converters for Left and Right audio. Besides, triggers are now
   t/riggers are now given in three different ways
   possible to be given also via two 6036E digital outputs (one for trig,
   another for code), and serial port, in addition to the original parallel
   port code.

   The most important thing being worked on is some "generalization" of
   original new stimulus paradigm. */

#define OCTOPUS_STIM_COMEDI
#define OCTOPUS_STIM_TRIG_COMEDI

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_math.h>
#include <linux/delay.h>
#include <linux/random.h>

#ifdef OCTOPUS_STIM_COMEDI
#include <comedi.h>
#include <linux/kcomedilib.h>
#endif

#include "../fb_command.h"
#include "../stimglobals.h"
#include "../stim_test_para.h"

/* ========================================================================= */

#define M_PI (double)(3.141519)

#ifdef OCTOPUS_STIM_COMEDI
static comedi_t *daqcard; static sampl_t dac_0,dac_1;
#else
static unsigned short dac_0,dac_1;
#endif

static char *himem_buffer,*xfer_shm,*patt_buf;
static fb_command fb_msg,bf_msg;
static RT_TASK audio_task,trigger_task;
static RTIME tickperiod; static SEM audio_sem; //,trigger_sem;
static int audio_rate=AUDIO_RATE,trigger_rate=TRIG_RATE;;
static int trigger_code=0,trigger_bit_shift=0;
static char current_pattern_data; static int current_pattern_offset=0;
static int audio_active=0,trigger_active=0,paradigm=0;
static int pattern_size,pattern_size_dummy;
static int manual_pause=0;

/* ========================================================================= */

static int counter0,counter1,counter2,counter3, //counter4,counter5,
           event0, //event1,event2,event3,event4,event5,
           var0; //,var1,var2,var3,var4,var5;
static double theta=0.0;
static int i=0;

/* ========================================================================= */

static void trigger_set(int t_code) {
#ifdef OCTOPUS_STIM_TRIG_COMEDI
 trigger_code=(t_code<<1) & 0x1fe; /* 10-bit trigger code (inc.start and stop bits)
				      to be fetched one bit at a time within task */
 trigger_bit_shift=10; /* Causes it to be launched within the trigger task */
#else
 outb(0x80|(fb_msg.iparam[0] & 0x7f),0x378);
#endif
}

static void trigger_reset(void) {
#ifdef OCTOPUS_STIM_TRIG_COMEDI
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,1,1); /* Code - failsafe zeroing */
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,0,1); /* Trigger */
#else
 outb(0x00,0x378);
#endif
}

/* ========================================================================= */
/* Experimental code */
static void lights_on(void) {
#ifdef OCTOPUS_STIM_TRIG_COMEDI
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,4,1);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,5,1);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,6,1);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,7,1);
#endif
}
static void lights_off(void) {
#ifdef OCTOPUS_STIM_TRIG_COMEDI
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,4,0);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,5,0);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,6,0);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,7,0);
#endif
}
static void lights_dimm(void) {
#ifdef OCTOPUS_STIM_TRIG_COMEDI
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,4,1);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,5,0);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,6,1);
 comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,7,0);
#endif
}

/* TESTS */
#include "para/acqtest_calibration.h"
#include "para/acqtest_sinecosine.h"
#include "para/acqtest_square.h"
#include "para/acqtest_steadysquare.h"

/* PARADIGMS */
#include "para/simple_click.h"
#include "para/square_burst.h"
#include "para/exp_iiditd.h"
#include "para/exp_iiditd_pplp.h"
#include "para/exp_itd_oppchn_a.h"
#include "para/exp_itd_oppchn_b.h"
#include "para/exp_itd_lintest_a.h"
#include "para/exp_itd_lintest_b.h"
#include "para/exp_itd_tompres.h"
#include "para/exp_itd_pip_ctrain.h"
#include "para/exp_itd_pip_rand.h"
#include "para/exp_0021a.h"
#include "para/exp_0021a1.h"
#include "para/exp_0021a2.h"
#include "para/exp_0021a3.h"
#include "para/exp_0021b.h"
#include "para/exp_0021c.h"
#include "para/exp_0021d.h"
#include "para/exp_0021e.h"

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
     case PARA_ITD_OPPCHN:    para_itdoppchn();	   break;
     case PARA_ITD_OPPCHN2:   para_itdoppchn2();   break;
     case PARA_ITD_LINTEST:   para_itdlintest();   break;
     case PARA_ITD_LINTEST2:  para_itdlintest2();  break;
     case PARA_ITD_TOMPRES:   para_itd_tompres();  break;
     case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain(); break;
     case PARA_ITD_PIP_RAND:  para_itd_pip_rand(); break;
     case PARA_0021A:         para_0021a();        break;
     case PARA_0021A1:        para_0021a1();       break;
     case PARA_0021A2:        para_0021a2();       break;
     case PARA_0021A3:        para_0021a3();       break;
     case PARA_0021B:         para_0021b();        break;
     case PARA_0021C:         para_0021c();        break;
     case PARA_0021D:         para_0021d();        break;
     case PARA_0021E:         para_0021e();        break;
    }
#ifdef OCTOPUS_STIM_COMEDI
    comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0); /* L */
    comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1); /* R */
#endif
   rt_sem_signal(&audio_sem);
  } /* audio_active */
  rt_task_wait_period();
 }
}

static void trigger_thread(int t) {
 while (1) {
  if (trigger_bit_shift) {
//   rt_sem_wait(&trigger_sem);

   if (trigger_bit_shift==10) comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,0,0); /* Trigger to low */

   comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,1,trigger_code&0x01);
   trigger_code>>=1; trigger_bit_shift--;
   if (!trigger_bit_shift) {
    comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,1,1); comedi_dio_write(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,0,1); /* Trigger to high */
   }
//   rt_sem_signal(&trigger_sem);
  }
  rt_task_wait_period();
 }
}

/* ========================================================================= */

static void stim_reset(void) {
 dac_0=dac_1=0; current_pattern_offset=0;
#ifdef OCTOPUS_STIM_COMEDI
 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0);
 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1);
 trigger_reset();

 lights_on();
#endif /* OCTOPUS_STIM_COMEDI */
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
  case PARA_ITD_OPPCHN:    para_itdoppchn_init();    break;
  case PARA_ITD_OPPCHN2:   para_itdoppchn2_init();   break;
  case PARA_ITD_LINTEST:   para_itdlintest_init();   break;
  case PARA_ITD_LINTEST2:  para_itdlintest2_init();  break;
  case PARA_ITD_TOMPRES:   para_itd_tompres_init();  break;
  case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain_init(); break;
  case PARA_ITD_PIP_RAND:  para_itd_pip_rand_init(); break;
  case PARA_0021A:         para_0021a_init();        break;
  case PARA_0021A1:        para_0021a1_init();       break;
  case PARA_0021A2:        para_0021a2_init();       break;
  case PARA_0021A3:        para_0021a3_init();       break;
  case PARA_0021B:         para_0021b_init();        break;
  case PARA_0021C:         para_0021c_init();        break;
  case PARA_0021D:         para_0021d_init();        break;
  case PARA_0021E:         para_0021e_init();        break;
 }
}

static void start_test_para(int tp) {
 switch (tp) {
  // others will be registered here by convention,
  // even if they don't exist
  case PARA_ITD_LINTEST:   para_itdlintest_start();   break;
  case PARA_ITD_LINTEST2:  para_itdlintest2_start();  break;
  case PARA_ITD_TOMPRES:   para_itd_tompres_start();  break;
  case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain_start(); break;
  case PARA_ITD_PIP_RAND:  para_itd_pip_rand_start(); break;
  case PARA_0021A:         para_0021a_start();        break;
  case PARA_0021A1:        para_0021a1_start();       break;
  case PARA_0021A2:        para_0021a2_start();       break;
  case PARA_0021A3:        para_0021a3_start();       break;
  case PARA_0021B:         para_0021b_start();        break;
  case PARA_0021C:         para_0021c_start();        break;
  case PARA_0021D:         para_0021d_start();        break;
  case PARA_0021E:         para_0021e_start();        break;
 }
}

static void stop_test_para(int tp) {
 switch (tp) {
  // others will be registered here by convention,
  // even if they don't exist
  case PARA_ITD_LINTEST:   para_itdlintest_stop();   break;
  case PARA_ITD_LINTEST2:  para_itdlintest2_stop();  break;
  case PARA_ITD_TOMPRES:   para_itd_tompres_stop();  break;
  case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain_stop(); break;
  case PARA_ITD_PIP_RAND:  para_itd_pip_rand_stop(); break;
  case PARA_0021A:         para_0021a_stop();        break;
  case PARA_0021A1:        para_0021a1_stop();       break;
  case PARA_0021A2:        para_0021a2_stop();       break;
  case PARA_0021A3:        para_0021a3_stop();       break;
  case PARA_0021B:         para_0021b_stop();        break;
  case PARA_0021C:         para_0021c_stop();        break;
  case PARA_0021D:         para_0021d_stop();        break;
  case PARA_0021E:         para_0021e_stop();        break;
 }
}

static void pause_test_para(int tp) {
 switch (tp) {
  // others will be registered here by convention,
  // even if they don't exist
  case PARA_ITD_LINTEST:   para_itdlintest_pause();   break;
  case PARA_ITD_LINTEST2:  para_itdlintest2_pause();  break;
  case PARA_ITD_TOMPRES:   para_itd_tompres_pause();  break;
  case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain_pause(); break;
  case PARA_ITD_PIP_RAND:  para_itd_pip_rand_pause(); break;
  case PARA_0021A:         para_0021a_pause();        break;
  case PARA_0021A2:        para_0021a2_pause();       break;
  case PARA_0021A3:        para_0021a3_pause();       break;
  case PARA_0021B:         para_0021b_pause();        break;
  case PARA_0021C:         para_0021c_pause();        break;
  case PARA_0021D:         para_0021d_pause();        break;
  case PARA_0021E:         para_0021e_pause();        break;
 }
}

static void resume_test_para(int tp) {
 switch (tp) {
  // others will be registered here by convention,
  // even if they don't exist
  case PARA_ITD_LINTEST:   para_itdlintest_resume();   break;
  case PARA_ITD_LINTEST2:  para_itdlintest2_resume();  break;
  case PARA_ITD_TOMPRES:   para_itd_tompres_resume();  break;
  case PARA_ITD_PIP_CTRAIN: para_itd_pip_ctrain_resume(); break;
  case PARA_ITD_PIP_RAND:  para_itd_pip_rand_resume(); break;
  case PARA_0021A:         para_0021a_resume();        break;
  case PARA_0021A1:        para_0021a1_resume();       break;
  case PARA_0021A2:        para_0021a2_resume();       break;
  case PARA_0021A3:        para_0021a3_resume();       break;
  case PARA_0021B:         para_0021b_resume();        break;
  case PARA_0021C:         para_0021c_resume();        break;
  case PARA_0021D:         para_0021d_resume();        break;
  case PARA_0021E:         para_0021e_resume();        break;
 }
}


/* ========================================================================= */

int fbfifohandler(unsigned int fifo,int rw) {
 if (rw=='w') {
  rtf_get(STIM_F2BFIFO,&fb_msg,sizeof(fb_command));
  switch (fb_msg.id) {
   case STIM_SET_PARADIGM: audio_active=0; dac_0=dac_1=0;
#ifdef OCTOPUS_STIM_COMEDI
                           comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0);
                           comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1);
			   trigger_reset();
#endif
                           paradigm=fb_msg.iparam[0];
                           rt_printk("octopus-stim-backend.o: Para -> %d.\n",
                                     paradigm);
                           init_test_para(paradigm);
                           rt_sem_wait(&audio_sem);
                            rtf_reset(STIM_B2FFIFO); rtf_reset(STIM_F2BFIFO);
                           rt_sem_signal(&audio_sem);
                           break;
   case STIM_LOAD_PATTERN: pattern_size=fb_msg.iparam[0]; /* Byte count */
                           pattern_size_dummy=0;	/* Current index */
                           break;
   case STIM_XFER_SYN:	   for (i=0;i<fb_msg.iparam[0];i++) /* Burst count */
                            patt_buf[pattern_size_dummy++]=xfer_shm[i];

			   bf_msg.id=STIM_XFER_ACK;
			   bf_msg.iparam[0]=fb_msg.iparam[0];
                           rtf_put(STIM_B2FFIFO,&bf_msg,sizeof(fb_command));
                           /* Burst acknowledged! */

			   if (pattern_size==pattern_size_dummy) /* Finished? */
                            rt_printk(
 "octopus-stim-backend.o: Stimulus pattern Xferred to backend successfully.\n");
                           break;
   case STIM_START:        start_test_para(paradigm);
                           break;
   case STIM_PAUSE:        manual_pause=1;
			   pause_test_para(paradigm);
                           break;
   case STIM_RESUME:       resume_test_para(paradigm);
                           break;
   case STIM_STOP:         stop_test_para(paradigm);
                           stim_reset();
#ifdef OCTOPUS_STIM_COMEDI
			   dac_0=dac_1=0;
                           comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0);
                           comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1);
			   trigger_reset();
#endif
                           rt_sem_wait(&audio_sem);
                            rtf_reset(STIM_B2FFIFO); rtf_reset(STIM_F2BFIFO);
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
   case STIM_LIGHTS_ON:    lights_on();
			   break;
   case STIM_LIGHTS_DIMM:  lights_dimm();
			   break;
   case STIM_LIGHTS_OFF:   lights_off();
			   break;
   case STIM_RST_SYN:      audio_active=0;
                           rt_sem_wait(&audio_sem);
                            rtf_reset(STIM_B2FFIFO); rtf_reset(STIM_F2BFIFO);
                            stim_reset();
                           rt_sem_signal(&audio_sem);
                           bf_msg.id=STIM_RST_ACK;
                           bf_msg.iparam[0]=bf_msg.iparam[1]=0;
                           rtf_put(STIM_B2FFIFO,&bf_msg,sizeof(fb_command));
                        rt_printk("octopus-stim-backend.o: Backend reset.\n");
                           break;
   case STIM_SYNTH_EVENT:  trigger_set(fb_msg.iparam[0]);
   default:                break;
  }
 }
 return 0;
}

/* ========================================================================= */

static int __init octopus_stim_init(void) {
 /* Initialize comedi device for auditory stimulus presentation */
#ifdef OCTOPUS_STIM_COMEDI
 daqcard=comedi_open("/dev/comedi0"); comedi_lock(daqcard,OCTOPUS_COMEDI_AO_SUBDEV);

 //lsampl_t maxdata;
 //comedi_krange* range_info;

 //range_info=comedi_get_krange(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,0);
 //maxdata=comedi_get_maxdata(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0);
 //if (range_info)
 // rt_printk("[0,%d] -> [%g,%g]\n",maxdata,range_info->min,range_info->max);
 //physical_value=comedi_to_phys(data_range_info,maxdata);

 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0);
 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1);
 rt_printk("octopus-stim-backend.o: Comedi Device Allocation successful. ->\n");
 rt_printk("octopus-stim-backend.o:  (daqcard=0x%p).\n",daqcard);
#else
 dac_0=dac_1=0;
 rt_printk("octopus-stim-backend.o: Diagnostic mode.. no COMEDI..\n");
#endif

 /* Initialize Himem Buffer for Wavefile uploading.. */
 himem_buffer=ioremap(HIMEMSTART,128*0x100000); /* toppest 128M */
 rt_printk("octopus-stim-backend.o: Himem dedicated mem: (0x%x).\n",
           himem_buffer);
 /* Initialize SHM Buffer for Large Object Transfer */
 xfer_shm=rtai_kmalloc('XFER',XFERBUFSIZE);
 rt_printk("octopus-stim-backend.o: Object transfer SHM allocated (0x%x).\n",
           xfer_shm);

 /* Initialize SHM Buffer for Stimulus Presentation Pattern Array */
 patt_buf=rtai_kmalloc('PATT',PATTBUFSIZE);
 rt_printk("octopus-stim-backend.o: Pattern buffer allocated (0x%x).\n",
           patt_buf);

 /* Initialize transfer fifos */
 rtf_create_using_bh(STIM_F2BFIFO,1000*sizeof(fb_command),0);
 rtf_create_using_bh(STIM_B2FFIFO,1000*sizeof(fb_command),0);
 rtf_create_handler(STIM_F2BFIFO,(void*)&fbfifohandler);
 rt_printk(
  "octopus-stim-backend.o: Up&downstream transfer FIFOs initialized.\n");

 /* Initialize auditory stimulation task */
 rt_task_init(&audio_task,(void *)audio_thread,0,2000,0,1,0);
 tickperiod=start_rt_timer(nano2count(1e9/audio_rate));
 rt_task_make_periodic(&audio_task,rt_get_time()+tickperiod,tickperiod);
 rt_printk(
  "octopus-stim-backend.o: Auditory Stimulus task initialized (50 KHz).\n");

#ifdef OCTOPUS_STIM_TRIG_COMEDI
 comedi_lock(daqcard,OCTOPUS_COMEDI_DO_SUBDEV);
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,0,COMEDI_OUTPUT); /* Trigger direction -trig */
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,1,COMEDI_OUTPUT); /* Trigger direction -code */

 // For arduino test
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,4,COMEDI_OUTPUT); /* Relay 0 */
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,5,COMEDI_OUTPUT); /* Relay 1 */
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,6,COMEDI_OUTPUT); /* Relay 2 */
 comedi_dio_config(daqcard,OCTOPUS_COMEDI_DO_SUBDEV,7,COMEDI_OUTPUT); /* Relay 3 */

 trigger_reset();

 /* Initialize trigger task */
 rt_task_init(&trigger_task,(void *)trigger_thread,0,2000,0,1,0);
 tickperiod=start_rt_timer(nano2count(1e9/trigger_rate));
 rt_task_make_periodic(&trigger_task,rt_get_time()+tickperiod,tickperiod);
 rt_printk("octopus-stim-backend.o: Trigger task initialized (9600 Hz).\n");
#endif

 return 0;
}

static void __exit octopus_stim_exit(void) {

 stop_rt_timer();
 rt_busy_sleep(1e7);

#ifdef OCTOPUS_STIM_TRIG_COMEDI
 /* Delete trigger task */
 rt_task_delete(&trigger_task);
 rt_printk("octopus-stim-backend.o: Trigger task disposed.\n");
 trigger_reset();
 comedi_unlock(daqcard,OCTOPUS_COMEDI_DO_SUBDEV);
#endif

// Delete visual stimulus task - to be implemented/merged from AURAVIS

 /* Delete auditory stimulus task */
 rt_task_delete(&audio_task);
 rt_printk("octopus-stim-backend.o: Audio task disposed.\n");

 /* Dispose transfer fifos */
 rtf_create_handler(STIM_F2BFIFO,(void*)&fbfifohandler);
 rtf_destroy(STIM_B2FFIFO); rtf_destroy(STIM_F2BFIFO);
 rt_printk("octopus-stim-backend.o: Up&downstream transfer FIFOs disposed.\n");

 /* Dispose Pattern Buffer */
 rtai_kfree('PATT');
 rt_printk("octopus-stim-backend.o: Pattern buffer disposed.\n");
 /* Dispose XFER Buffer */
 rtai_kfree('XFER');
 rt_printk("octopus-stim-backend.o: Large Object transfer SHM disposed.\n");
 /* Dispose Linux Unmapped Himem Buffer for media objects */
 iounmap(himem_buffer);
 rt_printk("octopus-stim-backend.o: Himem dedicated memory disposed.\n");

#ifdef OCTOPUS_STIM_COMEDI
 dac_0=dac_1=0;
 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,0,0,AREF_GROUND,DACZERO+dac_0);
 comedi_data_write(daqcard,OCTOPUS_COMEDI_AO_SUBDEV,1,0,AREF_GROUND,DACZERO+dac_1);
 comedi_unlock(daqcard,OCTOPUS_COMEDI_AO_SUBDEV);
 comedi_close(daqcard);
 rt_printk("octopus-stim-backend.o: Comedi DAQcard freed.\n");
#else
 rt_printk("octopus-stim-backend.o: STIM Backend exited successfully.\n");
#endif
}

module_init(octopus_stim_init);
module_exit(octopus_stim_exit);

MODULE_AUTHOR("Barkin Ilhan (barkin@unrlabs.org)");
MODULE_DESCRIPTION("Octopus STIM Backend v0.9");
MODULE_LICENSE("GPL"); 
