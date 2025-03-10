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

/* Supporting functions of acq-backend..
    Most crucial action is performed here.. */

#include "../acqglobals.h"
#include "../fb_command.h"

#define ACQ_F2BFIFO_SIZE	(1000)
#define ACQ_B2FFIFO_SIZE	(1000)

/* This is the number of seconds backward data is preserved..
   If data is not fetched in this interval it is lost! */
#define GRACE_COUNT		(5) /* In terms of seconds.. */

#define MAX_CHN			(128) /* Maximally hardcoded */

static fb_command f2b_msg,b2f_msg;

#ifdef OCTOPUS_ACQ_COMEDI
typedef struct _octopus_daqcard {
 comedi_t *dev;
 int subdev;		/* Analog input subdevice */
 int range;		/* (-10V,+10V) range */
 int chn_count;		/* How many single ended AI channels? */
 int zero_sample;	/* Corresponds to +0V */
} octopus_daqcard;
static int daqcard_count;
static octopus_daqcard *daqcard;
static int cur_idx;
static comedi_t* cur_dev[MAX_CHN];
static int cur_sub[MAX_CHN];
static int cur_chn[MAX_CHN];
static int cur_rng[MAX_CHN];
static int cur_zer[MAX_CHN];
#endif

/* Analog Input Channels in pool, originating all DAQ cards */
typedef struct _octopus_ai_channel {
 int daqcard_no;
 int channel_no;
} octopus_ai_channel;
static int ai_channel_count,ai_total_count;
static octopus_ai_channel *ai_channel;

typedef struct _octopus_buffer {
 int size; /* ACQ_BUFFER_SLOTS count */
 float *data;
} octopus_buffer;
static octopus_buffer buffer;

static RTIME rtai_ticks;

#ifdef OCTOPUS_ACQ_COMEDI
#define OCTOPUS_AD_DEV_COUNT	(16) /* Predefined COMEDI devices */
int setup_comedi(void) {
 int i,j,k=0,subdev,range; comedi_t *dev; /* Allocate available ADC devices */
 char devstr[14]; char devnum[4]; comedi_krange kr;
 for (i=0;i<OCTOPUS_AD_DEV_COUNT;i++) { /* Probe all available cards */
  sprintf(devstr,"%s","/dev/comedi"); sprintf(devnum,"%d",i);
  if (dev=comedi_open(strcat(devstr,devnum))) {
   /* Is there any ADC subdevice of this device? */
   if ((subdev=comedi_find_subdevice_by_type(dev,COMEDI_SUBD_AI,0))!=-1) {
    /* If so, does it support (-10V,+10V) range? */
    for (j=0;j<comedi_get_n_ranges(dev,subdev,0);j++) {
     range=comedi_get_krange(dev,subdev,0,j,&kr);
     if (kr.min==-10000000 && kr.max==10000000) { k++; break; }
     /* One more card satisfying the requirements.. */
    }
   }
   comedi_close(dev);
  }
 }
 /* Allocate daq structure space.. */
 if ((daqcard_count=k)>0) {
  if ((daqcard=rtai_kmalloc('CRDS',
                           sizeof(octopus_daqcard)*daqcard_count))==NULL) {
   rt_printk("octopus-acq-backend.o: Error while allocating card array!\n");
   return -1;
  }
  for (i=0,k=0;i<OCTOPUS_AD_DEV_COUNT;i++) { /* Setup all available cards */
   sprintf(devstr,"%s","/dev/comedi"); sprintf(devnum,"%d",i);
   if (dev=comedi_open(strcat(devstr,devnum))) {
    /* Get the ADC subdevice of this device.. */
    if ((subdev=comedi_find_subdevice_by_type(dev,COMEDI_SUBD_AI,0))!=-1) {
     /* Get the (-10V,+10V) range? */
     for (j=0;j<comedi_get_n_ranges(dev,subdev,0);j++) {
      range=comedi_get_krange(dev,subdev,0,j,&kr);
      if (kr.min==-10000000 && kr.max==10000000) {
       daqcard[k].dev=dev; daqcard[k].subdev=subdev; daqcard[k].range=range;
       daqcard[k].chn_count=comedi_get_n_channels(dev,subdev);
       daqcard[k].zero_sample=(comedi_get_maxdata(dev,subdev,0)+1)/2;
       comedi_lock(daqcard[k].dev,daqcard[k].subdev); k++;
      } /* One more card satisfying the requirements.. */
     }
    }
   }
  }
#ifdef OCTOPUS_VERBOSE
  // Give detailed info about each card..
  for (i=0;i<daqcard_count;i++) {
   rt_printk("octopus-acq-backend.o: CARD#%d: %s, %s\n",i,
             comedi_get_board_name(daqcard[i].dev),
             comedi_get_driver_name(daqcard[i].dev));
   rt_printk("octopus-acq-backend.o:  AI-Subdev:%d, (-10V,+10V) range:%d\n",
             daqcard[i].subdev,daqcard[i].range);
   rt_printk("octopus-acq-backend.o:  Channel Count:%d, Zero Val:%d\n",
             daqcard[i].chn_count,daqcard[i].zero_sample);
  }
#endif

  /* Construct the channel table */
  for (i=0,ai_channel_count=0;i<daqcard_count;i++)
   ai_channel_count+=daqcard[i].chn_count;
  if ((ai_channel=rtai_kmalloc('CHNS',
                       sizeof(octopus_ai_channel)*ai_channel_count))==NULL) {
   rt_printk("octopus-acq-backend.o: Error while allocating channel array!\n");
   return -1;
  }
  for (i=0;i<daqcard_count;i++) {
   for (j=0;j<daqcard[i].chn_count;j++) {
    ai_channel[i*daqcard[i].chn_count+j].daqcard_no=i;
    ai_channel[i*daqcard[i].chn_count+j].channel_no=j;
   }
  }

  /* half of the total count are the notch filtered version */
  ai_total_count=2*ai_channel_count+2;

  /* Make ready the precalculated channel values for use in main rtai task */
  for (i=0;i<ai_channel_count;i++) {
   cur_idx=ai_channel[i].daqcard_no;
   cur_dev[i]=daqcard[cur_idx].dev; cur_sub[i]=daqcard[cur_idx].subdev;
   cur_chn[i]=ai_channel[i].channel_no; cur_rng[i]=daqcard[cur_idx].range;
   cur_zer[i]=daqcard[cur_idx].zero_sample;
  }

#ifdef OCTOPUS_VERBOSE
  rt_printk("octopus-acq-backend.o: CHANNEL TABLE:\n");
  for (i=0;i<ai_channel_count;i++)
   rt_printk("octopus-acq-backend.o:  Card:%d, Channel:%d\n",
             ai_channel[i].daqcard_no,ai_channel[i].channel_no);
  rt_printk("octopus-acq-backend.o: Comedi Device Allocation successful..\n");

#endif
 }
#ifdef OCTOPUS_VERBOSE
 else {
  rt_printk("octopus-acq-backend.o: No suitable daqcards found for AI.\n");
 }
#endif

 return 0;
}

int dispose_comedi(void) {
 int i; /* Deallocate comedi structs.. */
 for (i=0;i<daqcard_count;i++) {
  comedi_unlock(daqcard[i].dev,daqcard[i].subdev);
  comedi_close(daqcard[i].dev);
 }
 rtai_kfree('CHNS'); rtai_kfree('CRDS');
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: Comedi device(s) freed.\n");
#endif
 return 0;
}
#endif

/* ************************************************************************* */

int setup_diagnostic(void) {
 ai_channel_count=MAX_CHN; ai_total_count=2*ai_channel_count+2;
 return 0;
}

/* ************************************************************************* */

int setup_shm(void) {
 int i; /* Initialize Acquisition Buffer */
 buffer.size=octopus_acq_rate*GRACE_COUNT; /* 5sec grace in case of overrun */
 if ((buffer.data=(float *)rtai_kmalloc('BUFF',
                   (buffer.size)*ai_total_count*sizeof(float)))==NULL) {
  rt_printk("octopus-acq-backend.o: Error while allocating SHM buffer!\n");
  return -1;
 } else {
  rt_printk("octopus-acq-backend.o: SHM buffer allocated -> %p\n",buffer.data);
 }
 for (i=0;i<ai_total_count*buffer.size;i++) (buffer.data)[i]=0.;

 return 0;
}

int dispose_shm(void) {
 /* Dispose Acquisition Buffer */
 rtai_kfree('BUFF');
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: SHM Buffer disposed.\n");
#endif
 return 0;
}

/* ************************************************************************* */

int setup_fifos(int *f_handler) {
 /* Initialize transfer fifos */
 rtf_create_using_bh(ACQ_F2BFIFO,ACQ_F2BFIFO_SIZE*sizeof(fb_command),0);
 rtf_create_using_bh(ACQ_B2FFIFO,ACQ_B2FFIFO_SIZE*sizeof(fb_command),0);
 rtf_create_handler(ACQ_F2BFIFO,f_handler); /* Set FIFO Handler */
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: Communication FIFOs initialized.\n");
#endif
 return 0;
}

int dispose_fifos(int *f_handler) {
 /* Dispose transfer fifos */
 rtf_create_handler(ACQ_F2BFIFO,f_handler);
 rtf_destroy(ACQ_B2FFIFO); rtf_destroy(ACQ_F2BFIFO);
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: Communication FIFOs disposed.\n");
#endif
 return 0;
}

/* ************************************************************************* */

int setup_rtai(RT_TASK *task,void *r_thread) {
 /* Setup Realtime task */
 rt_task_init(task,(void *)r_thread,0,2000,0,1,0);
 rtai_ticks=start_rt_timer(nano2count(1e9/octopus_acq_rate));
 rt_task_make_periodic(task,rt_get_time()+rtai_ticks,rtai_ticks);
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: Acq RTAI task started at %d Hz.\n",octopus_acq_rate);
#endif
 return 0;
}

int dispose_rtai(RT_TASK *task) {
 /* Delete the bottom half task of vidirq handler */
 stop_rt_timer();
 rt_busy_sleep(1e7);
 rt_task_delete(task);
#ifdef OCTOPUS_VERBOSE
 rt_printk("octopus-acq-backend.o: Acquisition RTAI task disposed.\n");
#endif
 return 0;
}

/* ************************************************************************* */

int setup_stim(int p_irq,int p_base,void *p_handler) {
 int data;
 /* Initalize stim irq handler */
 rt_disable_irq(p_irq);
  rt_request_global_irq(p_irq,p_handler);
 rt_enable_irq(p_irq);

 outb(inb(p_base+2)&0xdf,p_base+2); /* Test bi-directional mode */
 outb(inb(p_base+2)|0x30,p_base+2); outb(34,p_base);
 rt_busy_sleep(nano2count(10000));
 data=inb(p_base);
 rt_busy_sleep(nano2count(10000));
 if (data==34) { /* still the same value? */
  rt_printk("octopus-acq-backend.o: Warning!\n");
  rt_printk("  Parallel port bidirectional mode not supported;\n");
  rt_printk("  Won't receive any triggers...\n");
  return -1;
 }
#ifdef OCTOPUS_VERBOSE
 rt_printk(
  "octopus-acq-backend.o: STIM (Parallel Port) IRQ Handler initialized.\n");
#endif
 return 0;
}

int dispose_stim(int p_irq,int p_base) {
 /* Dispose stim irq handler */
 outb(inb(p_base+3)&0xef,p_base+3);
 rt_disable_irq(p_irq); rt_free_global_irq(p_irq);
 outb(0x00,p_base);
#ifdef OCTOPUS_VERBOSE
 rt_printk(
  "octopus-acq-backend.o: STIM (Parallel Port) IRQ Handler disposed.\n");
#endif
 return 0;
}

/* ************************************************************************* */

int setup_resp(int s_irq,int s_base,void *s_handler) {
 /* Initalize response irq handler */
 rt_disable_irq(s_irq); rt_request_global_irq(s_irq,s_handler);
 outb(0x80,s_base+3); /* Set DLAB */
 outb(0x0c,s_base+0); /* Set baud rate to 9600bps - Divisor Latch Low */
 outb(0x00,s_base+1); /* Set baud rate to 9600bps - Divisor Latch High */
 outb(0x03,s_base+3); /* DLAB off and 8N1 */
 outb(0x07,s_base+2); /* FCR: FIFOs on, INT every byte & clear them - 0xc7 */
 outb(0x0b,s_base+4); /* Turn on DTR,RTS,OUT */
 rt_enable_irq(s_irq);
 outb(0x01,s_base+1); /* Turn on RECV interrupt */
#ifdef OCTOPUS_VERBOSE
 rt_printk(
  "octopus-acq-backend.o: RESP (Serial Port) IRQ Handler initialized.\n");
#endif
 return 0;
}

int dispose_resp(int s_irq,int s_base) {
 /* Dispose RESP irq handler */
 outb(0x00,s_base+1); /* disable serial interrupts */
 rt_disable_irq(s_irq); rt_free_global_irq(s_irq);
#ifdef OCTOPUS_VERBOSE
 rt_printk(
  "octopus-acq-backend.o: RESP (Serial Port) IRQ Handler disposed.\n");
#endif
 return 0;
}

/* ************************************************************************* */

static void b2f_cmd(unsigned short cmd,int p0,int p1,int p2,int p3) {
 b2f_msg.id=cmd;
 b2f_msg.iparam[0]=p0; b2f_msg.iparam[1]=p1;
 b2f_msg.iparam[2]=p2; b2f_msg.iparam[3]=p3;
 rtf_put(ACQ_B2FFIFO,&b2f_msg,sizeof(fb_command));
}

static void reset_fifos(void) {
 rt_sem_wait(&octopus_acq_sem);
  rtf_reset(ACQ_B2FFIFO); rtf_reset(ACQ_F2BFIFO);
 rt_sem_signal(&octopus_acq_sem);
}
