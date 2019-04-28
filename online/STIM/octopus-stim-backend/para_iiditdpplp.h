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

/* Psychophysical IIDITD Laterality Determination (PPLP). */

static int para_iiditdpplp_base_level_l,
           para_iiditdpplp_base_level_r,
           para_iiditdpplp_current_iid_amp_l,
           para_iiditdpplp_current_iid_amp_r,
           para_iiditdpplp_t1,
           para_iiditdpplp_t2,
           para_iiditdpplp_t3,
           para_iiditdpplp_t4,
	   para_iiditdpplp_t5,
	   para_iiditdpplp_t6,
           para_iiditdpplp_itd_lr_delta;

static void para_iiditdpplp_init(void) {
 para_iiditdpplp_base_level_l=para_iiditdpplp_base_level_r=0;
 para_iiditdpplp_current_iid_amp_l=para_iiditdpplp_current_iid_amp_r=DA_NORM;
 para_iiditdpplp_t1=25; /* 500us up, 9.5ms down */
 para_iiditdpplp_t2=500;
 para_iiditdpplp_t3=DA_SYNTH_RATE;
 para_iiditdpplp_t4=3*DA_SYNTH_RATE;
 para_iiditdpplp_t5=25*3*DA_SYNTH_RATE;    /* Pause at 2 x this value. */
 para_iiditdpplp_t6=25*3*DA_SYNTH_RATE*11; /* 10 rounds total + 1 pretrial */
 para_iiditdpplp_itd_lr_delta=0;
 stim_paused=pause_trighi=0;
}

static void para_iiditdpplp(void) {
 if (!stim_paused) {
  if (para_iiditdpplp_base_level_l) /* Set IID Left Amplitude */
   dac_0=para_iiditdpplp_current_iid_amp_l; else dac_0=0;
  if (para_iiditdpplp_base_level_r) /* Set IID Right Amplitude */
   dac_1=para_iiditdpplp_current_iid_amp_r; else dac_1=0;
  counter1++; counter1%=para_iiditdpplp_t2;
  counter0=counter1-para_iiditdpplp_t2/2; /* Adjust to [-250,+250) */

  /* ITD shift for left */
  if (para_iiditdpplp_base_level_l &&
      counter0==-para_iiditdpplp_itd_lr_delta/2+para_iiditdpplp_t1)
   para_iiditdpplp_base_level_l=0;
  else if (counter0==-para_iiditdpplp_itd_lr_delta/2)
   para_iiditdpplp_base_level_l=1;

  /* ITD shift for right */
  if (para_iiditdpplp_base_level_r &&
      counter0==para_iiditdpplp_itd_lr_delta/2+para_iiditdpplp_t1)
   para_iiditdpplp_base_level_r=0;
  else if (counter0==para_iiditdpplp_itd_lr_delta/2)
   para_iiditdpplp_base_level_r=1;

  if (counter2==0) {
   current_pattern_data=himem_buffer[current_pattern_offset]; /* fetch new.. */
   current_pattern_offset++;

   /* Roll-over */
   if (current_pattern_offset==pattern_size) current_pattern_offset=0;

   switch (current_pattern_data) {
    /* CENTER */
    case '0': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD=0,IID: */
              para_iiditdpplp_current_iid_amp_r=DA_NORM; // Both L&R norm
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_CENTER,STIM_PARBASE);
              break;

    /* IID */
    case '1': para_iiditdpplp_current_iid_amp_l=1150; /* IID: Left +6 */
              para_iiditdpplp_current_iid_amp_r=363;  /* (1dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L6,STIM_PARBASE);
              break;
    case '2': para_iiditdpplp_current_iid_amp_l=913;  /* IID: Left +5 */
              para_iiditdpplp_current_iid_amp_r=458;  /* (2dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L5,STIM_PARBASE);
              break;
    case '3': para_iiditdpplp_current_iid_amp_l=814;  /* IID: Left +4 */
              para_iiditdpplp_current_iid_amp_r=513;  /* (3dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L4,STIM_PARBASE);
              break;
    case '4': para_iiditdpplp_current_iid_amp_l=768;  /* IID: Left +3 */
              para_iiditdpplp_current_iid_amp_r=544;  /* (4dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L3,STIM_PARBASE);
              break;
    case '5': para_iiditdpplp_current_iid_amp_l=725;  /* IID: Left +2 */
              para_iiditdpplp_current_iid_amp_r=576;  /* (6dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L2,STIM_PARBASE);
              break;
    case '6': para_iiditdpplp_current_iid_amp_l=685;  /* IID: Left +1 */
              para_iiditdpplp_current_iid_amp_r=610;  /* (10dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_L1,STIM_PARBASE);
              break;

    case '7': para_iiditdpplp_current_iid_amp_l=610;  /* IID: Right +1 */
              para_iiditdpplp_current_iid_amp_r=685;  /* (10dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R1,STIM_PARBASE);
              break;
    case '8': para_iiditdpplp_current_iid_amp_l=576;  /* IID: Right +2 */
              para_iiditdpplp_current_iid_amp_r=725;  /* (6dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R2,STIM_PARBASE);
              break;
    case '9': para_iiditdpplp_current_iid_amp_l=544;  /* IID: Right +3 */
              para_iiditdpplp_current_iid_amp_r=768;  /* (4dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R3,STIM_PARBASE);
              break;
    case 'a': para_iiditdpplp_current_iid_amp_l=544;  /* IID: Right +4 */
              para_iiditdpplp_current_iid_amp_r=768;  /* (3dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R4,STIM_PARBASE);
              break;
    case 'b': para_iiditdpplp_current_iid_amp_l=458;  /* IID: Right +5 */
              para_iiditdpplp_current_iid_amp_r=913;  /* (2dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R5,STIM_PARBASE);
              break;
    case 'c': para_iiditdpplp_current_iid_amp_l=363;  /* IID: Right +6 */
              para_iiditdpplp_current_iid_amp_r=1150; /* (1dB) */
              para_iiditdpplp_itd_lr_delta=0;
              if (trig_active) outb(0x80|SEC_PP_IID_R6,STIM_PARBASE);
              break;

    /* ITD */
    case 'd': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +6 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=30; /* 600usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L6,STIM_PARBASE);
              break;
    case 'e': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +5 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=10; /* 200usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L5,STIM_PARBASE);
              break;
    case 'f': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +4 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=5; /* 100usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L4,STIM_PARBASE);
              break;
    case 'g': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +3 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=3; /* 60usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L3,STIM_PARBASE);
              break;
    case 'h': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +2 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=2; /* 40usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L2,STIM_PARBASE);
              break;
    case 'i': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Left +1 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=1; /* 20usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_L1,STIM_PARBASE);
              break;

    case 'j': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +1 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-1; /* 20usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R1,STIM_PARBASE);
              break;
    case 'k': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +2 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-2; /* 40usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R2,STIM_PARBASE);
              break;
    case 'l': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +3 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-3; /* 60usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R3,STIM_PARBASE);
              break;
    case 'm': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +4 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-5; /* 100usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R4,STIM_PARBASE);
              break;
    case 'n': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +5 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-10; /* 200usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R5,STIM_PARBASE);
              break;
    case 'o': para_iiditdpplp_current_iid_amp_l=DA_NORM; /* ITD: Right +6 */
              para_iiditdpplp_current_iid_amp_r=DA_NORM;
              para_iiditdpplp_itd_lr_delta=-30; /* 600usec */
              if (trig_active) outb(0x80|SEC_PP_ITD_R6,STIM_PARBASE);
    default:  break;
   }
  } else if (counter2==TRIG_HI_STEPS) outb(0x00,STIM_PARBASE); /* Pull down */
  else if (counter2==para_iiditdpplp_t3) {
    para_iiditdpplp_current_iid_amp_l=0; /* Silence */
    para_iiditdpplp_current_iid_amp_r=0;
    para_iiditdpplp_itd_lr_delta=0;
  }
  counter2++; counter2%=para_iiditdpplp_t4; /* 3seconds? */
  counter3++;
  if (counter3==para_iiditdpplp_t6) { /* Stop */
   stim_active=lp0_data=0;
//   stim_reset();
#ifdef OCTOPUS_STIM_COMEDI
   comedi_data_write(comedi_da_dev,1,0,0,AREF_GROUND,dac_0);
   comedi_data_write(comedi_da_dev,1,1,0,AREF_GROUND,dac_1);
   comedi_data_write(comedi_da_dev,1,2,0,AREF_GROUND,dac_2);
   comedi_data_write(comedi_da_dev,1,3,0,AREF_GROUND,dac_3);
#endif
   rt_sem_wait(&stim_sem);
    rtf_reset(BFFIFO);
    rtf_reset(FBFIFO);
   rt_sem_signal(&stim_sem);
   rt_printk("octopus-stim-rtai.o: Stim stopped.\n");
  } else if (counter3%para_iiditdpplp_t5==0) {
   dac_0=dac_1=0; stim_paused=1; pause_trighi=0; /* 39let? */
   outb(0x80|SEC_PAUSE,STIM_PARBASE);
   rt_printk("offset: %d\n",counter3);
  }
 } else {
  pause_trighi++;
  if (pause_trighi==TRIG_HI_STEPS) outb(0x00,STIM_PARBASE);
 }
 dac_2=dac_3=0;
}
