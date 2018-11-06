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

/* This is the acquisition backend RTAI module designed to run over a
   realtime Linux node which needs to have parallel and serial ports, and
   AD card(s) supported by COMEDI, in order to acquire N-channels
   of EEG data.

   After launching, channel list will be filled with subsequent channels of
   all AD subdevices of the devices preconfigured in COMEDI. Hence, the system
   operator should be aware of the fact that, although the system measures
   true values in channels, some variance factors such as:

    * Implementation specific sequential scanning of the channels of a device
      may cause minor deterministic delay, compared to other devices' channels,

    * If individual devices' amplitude bit resolutions are different, this
      will totally reflect the amplitude resolution of measured data,

    * The DC Levels, and other differences between devices, which depend on
      inner-calibration details are not compensated, and post-matching of
      channels is the operator's responsability.

   This backend is tested on National Instruments AT-MIO64-E3 (ISA) and
   PCI-6071E DAQ cards together (64+64=128 channel source localization system),
   and mobile systems with a single PCMCIA DAQ Card such as NI DAQCard-6036E
   or NI AI-16-E4 (16 channel mobile acquisition system appropriate for
   usage over older laptop systems with parallel/serial ports). So, also
   tested is the configuration in which NI DAQCard-6036E and NI AI-16-E4
   coexist.

   As long as a free IRQ is available for the second card, this also seem to
   work. In Octopus, Serial and Parallel ports are Interrupt driven, it's
   better if they don't interfere with the COMEDI devices, although Octopus
   does not rely on COMEDI commands in the meantime.

   The data transferred to daemon over the Parallel and Serial ports, are the
   STIMulus events and subject RESPonses, respectively. The triggers are
   designed to originate from any corresponding device, better to be capable
   of sending edge-sensitive signals. The events are then processed via the
   corresponding realtime interrupt handlers. Parallel port must support
   and actually be in bi-directional mode, accepting incoming data.

   EEG data combined with the events are sent to userspace using a SYN-ACK
   handshake protocol controlled over two FIFOs. Data, on the other hand, is
   transferred via the SHM buffer allocated during module initialization. */

#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_math.h>

#define OCTOPUS_ACQ_COMEDI
#define OCTOPUS_VERBOSE

#ifdef OCTOPUS_ACQ_COMEDI
#include <linux/kcomedilib.h>
static lsampl_t dummy_sample;
#else
/* Due to a momentary problem with RTAI this is to be defined manually. */
#define M_PI (double)(3.141519)
#endif

static float dummy_data,dummy_filt;

static SEM octopus_acq_sem;

/* ! Will be changed to be read as module parameter from the command line. */
static int par_base=0x378,ser_base=0x3f8,par_irq=7,ser_irq=3;

/* ========================================================================= */

#define SAMPLE_RATE	(250)  /* Soon, will (may) be dynamically adjustable */

/* Since we depend on amplifier CMRR, range is hard-coded as microvolts.
   but it is for this one to be given as a module parameter as well. */
#define AMP_RANGE	(200.) /* Amplifier range in uVpeak */

static int octopus_acq_rate=SAMPLE_RATE;

static RT_TASK octopus_acq_task;

#include "octopus-acq-backend.h"

static int pivot_be=0,pivot_fe=0,pivot_global_be=0,pivot_global_fe=0;
static int acq_active=0,stim_data=0,resp_data=0;

static int sample_counter=0,total_stim_interrupts=0,total_resp_interrupts=0;

static int alt_counter=0,hz=0;

static int data_loss_flag=0;
static int conv_n,conv_index,dummy_index;

static float stim_trig=0.,resp_trig=0.;

#ifndef OCTOPUS_ACQ_COMEDI
static float theta=0.,t_signal=1.,t_noise=0.02,a_signal=1.,a_noise=0.1;
static float a_noise2=0.7;
#endif

/* ========================================================================= */

static void octopus_acq_thread(int t) {
 int i,j,k,err,buffer_delta;
 while (1) {
  if (acq_active) {
   rt_sem_wait(&octopus_acq_sem);
/* ------------------------------------------------------------------------- */
    /* Kind of circular buffer.. */

    /* ! The new versions of RTAI support a circular buffer structure.
         The code here may be changed to use these. */

    /* User-space client continuously chase this kernel-backend for new data */
    buffer_delta=pivot_global_be-pivot_global_fe;
    /* ERROR!: Userspace caught kernelspace.. result is data loss.. */
    if (buffer_delta>buffer.size) {
     data_loss_flag=1; pivot_global_be=pivot_global_fe+buffer.size;
    } else data_loss_flag=0;

    if (data_loss_flag) b2f_cmd(ACQ_ALERT,ALERT_DATA_LOSS,0,0,0);

    /* Acquire all N channels to the proper ShMem Region..
       The (N-2)th (STIM) and (N-1)th (RESP) channels will be independently
       put into structure by parallel and serial port handlers.. */
    for (i=0;i<ai_channel_count;i++) {
#ifdef OCTOPUS_ACQ_COMEDI
     err=comedi_data_read_delayed(cur_dev[i],cur_sub[i],cur_chn[i],cur_rng[i],
                                  AREF_GROUND,&dummy_sample,2000);
     dummy_data=(float)
                ((short)(dummy_sample)-cur_zer[i])*200./(float)(cur_zer[i]);
     (buffer.data)[pivot_be*ai_total_count+i]=dummy_data;
#else
     /* Range is 400uVpp */
     dummy_data =a_signal*(AMP_RANGE*95./100.) * sin(2.*M_PI*theta/t_signal);
     if (hz)
      dummy_data+=a_noise *(AMP_RANGE*95./100.) * sin(2.*M_PI*theta/t_noise );
     else
      dummy_data+=a_noise2 *(AMP_RANGE*95./100.) * sin(2.*M_PI*theta/t_noise );

     (buffer.data)[pivot_be*ai_total_count+i]=dummy_data;
#endif

     /* APPLY 50Hz FILTER */
     dummy_filt=0.;
     conv_index = (buffer.size+pivot_be-conv_n)%buffer.size;
     for (j=0;j<conv_n;j++) {
      dummy_index=(conv_index+j)%buffer.size;
      dummy_filt+=(buffer.data)[dummy_index*ai_total_count+i];
     } dummy_filt/=conv_n;

     /* Compute 50Hz+Harmonics component by subtraction from original signal */
     dummy_index=(conv_index+(conv_n+1)/2)%buffer.size;
     (buffer.data)[dummy_index*ai_total_count+ai_channel_count+i]=
      (buffer.data)[dummy_index*ai_total_count+i]-dummy_filt;
    }

    /* Append STIM & RESP event codes to the end.. */
    (buffer.data)[(pivot_be+1)*ai_total_count-2]=stim_trig;
    (buffer.data)[(pivot_be+1)*ai_total_count-1]=resp_trig;
    stim_trig=resp_trig=0;

#ifndef OCTOPUS_ACQ_COMEDI
    theta+=1./(float)(octopus_acq_rate); if (theta>1.) theta=0.;
#endif

#ifdef OCTOPUS_VERBOSE
    if (!sample_counter & data_loss_flag) {
     rt_printk("octopus-acq-backend.o: DATA LOSS!! GBE: %d, GFE: %d Delta=%d\n",
               pivot_global_be,pivot_global_fe,
               pivot_global_be-pivot_global_fe);
    }
#endif

#ifndef OCTOPUS_ACQ_COMEDI
    sample_counter++;
    if (sample_counter==octopus_acq_rate) {
     sample_counter=0;
     alt_counter++;
     if (alt_counter==15) {
      alt_counter=0; hz=!hz;
     }
    }
#endif

    /* Since we are done with the channels' acquisition, we inform the
       frontend that it may get the data,
       associated ACK signal will come thru FIFO Handler */
    b2f_msg.id=ACQ_CMD_B2F; b2f_msg.iparam[0]=B2F_DATA_SYN;
    b2f_msg.iparam[1]=pivot_be; b2f_msg.iparam[2]=pivot_global_be;
    pivot_be++; pivot_be%=buffer.size; pivot_global_be++;
    rtf_put(ACQ_B2FFIFO,&b2f_msg,sizeof(fb_command));
/* ------------------------------------------------------------------------- */
   rt_sem_signal(&octopus_acq_sem);
  } /* acq_active */
  rt_task_wait_period();
 }
}

/* ========================================================================= */

static void stim_handler(void) {
 if (acq_active) {
  stim_data=inb(par_base)&0x7f;
  rt_sem_wait(&octopus_acq_sem);
   /* Put STIM event code to (N)th channel */
   stim_trig=(float)(stim_data);
  rt_sem_signal(&octopus_acq_sem);
 } total_stim_interrupts++;
 rt_ack_irq(par_irq);
}

static void resp_handler(void) {
 if (inb(ser_base+5)&1) { /* Line Status Register, Is data ready? */
  resp_data=inb(ser_base)-0x30; /* Convert from ASCII to integer */
  if (acq_active) {
   rt_sem_wait(&octopus_acq_sem);
    /* Put RESP event code to (N+1)th channel */
    resp_trig=(float)(127+resp_data);
   rt_sem_signal(&octopus_acq_sem);
  }
//  rt_printk("ResponsePad=%d\n",resp_data);
  outb(0x7,ser_base+2); /* FCR: FIFOs on, INT every byte and clear'em - 0xc7 */
 } total_resp_interrupts++;
// rt_pend_linux_irq(ser_irq);
 rt_ack_irq(ser_irq);
}

/* ======================================================================== */

static int fb_fifohandler(unsigned int fifo,int rw) {
 if (rw=='w') {
  rtf_get(ACQ_F2BFIFO,&f2b_msg,sizeof(fb_command));
  switch (f2b_msg.id) {
   case ACQ_START:  reset_fifos(); acq_active=1;
                    rt_printk("octopus-acq-backend.o: Acquisition started.\n");
                    break;
   case ACQ_STOP:   acq_active=0;
                    rt_printk("octopus-acq-backend.o: Acquisition stopped.\n");
                    break;
   case ACQ_CMD_F2B:
    switch (f2b_msg.iparam[0]) {
     case F2B_DATA_ACK:        if (acq_active) {
                                /* The offset just read by the frontend */
                                pivot_fe=f2b_msg.iparam[1];
                                pivot_global_fe=f2b_msg.iparam[2];
                                /* Delete previous triggers
                                   -- better to be done in fe */
                                (buffer.data)[(pivot_be+1)*ai_total_count-2]=0.;
                                (buffer.data)[(pivot_be+1)*ai_total_count-1]=0.;
                               } break;
     case F2B_RESET_SYN:       acq_active=0; reset_fifos();
                               b2f_cmd(ACQ_CMD_B2F,B2F_RESET_ACK,
                                 buffer.size,ai_total_count,octopus_acq_rate);
                     rt_printk("octopus-acq-backend.o: ACQ backend reset.\n");
                               break;
     case F2B_TRIGTEST:        rt_sem_wait(&octopus_acq_sem);
                                stim_trig=1.;
                               rt_sem_signal(&octopus_acq_sem); break;
     default: rt_printk("octopus-acq-backend.o: FIFO unknown cmd!\n"); break;
    } break;
   default: rt_printk("octopus-acq-backend.o: FIFO cmd error!\n"); break;
  }
 }
 return 0;
}

/* ======================================================================== */

static int __init octopus_init(void) {
 conv_n=octopus_acq_rate/50;

#ifdef OCTOPUS_ACQ_COMEDI
 setup_comedi();
#else
 setup_diagnostic();
#endif
 setup_shm();
 setup_fifos((int *)fb_fifohandler);
 setup_rtai(octopus_acq_rate,&octopus_acq_task,octopus_acq_thread);
 setup_stim(par_irq,par_base,stim_handler);
 setup_resp(ser_irq,ser_base,resp_handler);

 rt_printk("octopus-acq-backend.o: Acquisition backend module initialized.\n");
 return 0;
}

static void __exit octopus_exit(void) {
 dispose_resp(ser_irq,ser_base);
 dispose_stim(par_irq,par_base);
 dispose_rtai(&octopus_acq_task);
 dispose_fifos((int *)fb_fifohandler);
 dispose_shm();
#ifdef OCTOPUS_ACQ_COMEDI
 dispose_comedi();
#endif

#ifdef OCTOPUS_ACQ_COMEDI
 rt_printk("octopus-acq-backend.o: Total STIM interrupts: %d\n",
           total_stim_interrupts);
 rt_printk("octopus-acq-backend.o: Total RESP interrupts: %d\n\n",
           total_resp_interrupts);
#endif

 rt_printk("octopus-acq-backend.o: Acquisition backend module disposed.\n");
}

module_init(octopus_init);
module_exit(octopus_exit);

MODULE_AUTHOR("Barkin Ilhan (barkin@unrlab.org)");
MODULE_DESCRIPTION("Octopus-ReEL - ACQ Realtime kernel-space backend");
MODULE_LICENSE("GPL"); 
