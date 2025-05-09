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

/* Psychophysical IIDITD Laterality Determination (PPLP). */

#define SEC_PP_CENTER		50	// "PP CENTER",
#define SEC_PP_IID_L6		51	// "PP IID -6",
#define SEC_PP_IID_L5		52	// "PP IID -5",
#define SEC_PP_IID_L4		53	// "PP IID -4",
#define SEC_PP_IID_L3		54	// "PP IID -3",
#define SEC_PP_IID_L2		55	// "PP IID -2",
#define SEC_PP_IID_L1		56	// "PP IID -1",
#define SEC_PP_IID_R1		57	// "PP IID +1",
#define SEC_PP_IID_R2		58	// "PP IID +2",
#define SEC_PP_IID_R3		59	// "PP IID +3",
#define SEC_PP_IID_R4		60	// "PP IID +4",
#define SEC_PP_IID_R5		61	// "PP IID +5",
#define SEC_PP_IID_R6		62	// "PP IID +6",
#define SEC_PP_ITD_L6		63	// "PP ITD -6",
#define SEC_PP_ITD_L5		64	// "PP ITD -5",
#define SEC_PP_ITD_L4		65	// "PP ITD -4",
#define SEC_PP_ITD_L3		66	// "PP ITD -3",
#define SEC_PP_ITD_L2		67	// "PP ITD -2",
#define SEC_PP_ITD_L1		68	// "PP ITD -1",
#define SEC_PP_ITD_R1		69	// "PP ITD +1",
#define SEC_PP_ITD_R2		70	// "PP ITD +2",
#define SEC_PP_ITD_R3		71	// "PP ITD +3",
#define SEC_PP_ITD_R4		72	// "PP ITD +4",
#define SEC_PP_ITD_R5		73	// "PP ITD +5",
#define SEC_PP_ITD_R6		74	// "PP ITD +6",

#define SEC_PAUSE		126	// "Pause"

static int audio_paused=0,pause_trigger_hi=0;

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
 para_iiditdpplp_current_iid_amp_l=para_iiditdpplp_current_iid_amp_r=AMP_0DB;
 para_iiditdpplp_t1=25; /* 500us up, 9.5ms down */
 para_iiditdpplp_t2=500;
 para_iiditdpplp_t3=AUDIO_RATE;
 para_iiditdpplp_t4=3*AUDIO_RATE;
 para_iiditdpplp_t5=25*3*AUDIO_RATE;    /* Pause at 2 x this value. */
 para_iiditdpplp_t6=25*3*AUDIO_RATE*11; /* 10 rounds total + 1 pretrial */
 para_iiditdpplp_itd_lr_delta=0;
 audio_paused=pause_trigger_hi=0;
}

static void para_iiditdpplp(void) {
 if (!audio_paused) {
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
   current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
   current_pattern_offset++;

   /* Roll-over */
   if (current_pattern_offset==pattern_size) current_pattern_offset=0;

   switch (current_pattern_data) {
    /* CENTER */
    case '0': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD=0,IID: */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB; // Both L&R norm
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_CENTER);
              break;

    /* IID */
    case '1': para_iiditdpplp_current_iid_amp_l=1150; /* IID: Left +6 */
              para_iiditdpplp_current_iid_amp_r=363;  /* (1dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L6);
              break;
    case '2': para_iiditdpplp_current_iid_amp_l=913;  /* IID: Left +5 */
              para_iiditdpplp_current_iid_amp_r=458;  /* (2dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L5);
              break;
    case '3': para_iiditdpplp_current_iid_amp_l=814;  /* IID: Left +4 */
              para_iiditdpplp_current_iid_amp_r=513;  /* (3dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L4);
              break;
    case '4': para_iiditdpplp_current_iid_amp_l=768;  /* IID: Left +3 */
              para_iiditdpplp_current_iid_amp_r=544;  /* (4dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L3);
              break;
    case '5': para_iiditdpplp_current_iid_amp_l=725;  /* IID: Left +2 */
              para_iiditdpplp_current_iid_amp_r=576;  /* (6dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L2);
              break;
    case '6': para_iiditdpplp_current_iid_amp_l=685;  /* IID: Left +1 */
              para_iiditdpplp_current_iid_amp_r=610;  /* (10dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_L1);
              break;

    case '7': para_iiditdpplp_current_iid_amp_l=610;  /* IID: Right +1 */
              para_iiditdpplp_current_iid_amp_r=685;  /* (10dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R1);
              break;
    case '8': para_iiditdpplp_current_iid_amp_l=576;  /* IID: Right +2 */
              para_iiditdpplp_current_iid_amp_r=725;  /* (6dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R2);
              break;
    case '9': para_iiditdpplp_current_iid_amp_l=544;  /* IID: Right +3 */
              para_iiditdpplp_current_iid_amp_r=768;  /* (4dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R3);
              break;
    case 'a': para_iiditdpplp_current_iid_amp_l=544;  /* IID: Right +4 */
              para_iiditdpplp_current_iid_amp_r=768;  /* (3dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R4);
              break;
    case 'b': para_iiditdpplp_current_iid_amp_l=458;  /* IID: Right +5 */
              para_iiditdpplp_current_iid_amp_r=913;  /* (2dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R5);
              break;
    case 'c': para_iiditdpplp_current_iid_amp_l=363;  /* IID: Right +6 */
              para_iiditdpplp_current_iid_amp_r=1150; /* (1dB) */
              para_iiditdpplp_itd_lr_delta=0;
	      trigger_set(SEC_PP_IID_R6);
              break;

    /* ITD */
    case 'd': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +6 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=30; /* 600usec */
	      trigger_set(SEC_PP_ITD_L6);
              break;
    case 'e': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +5 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=10; /* 200usec */
	      trigger_set(SEC_PP_ITD_L5);
              break;
    case 'f': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +4 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=5; /* 100usec */
	      trigger_set(SEC_PP_ITD_L4);
              break;
    case 'g': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +3 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=3; /* 60usec */
	      trigger_set(SEC_PP_ITD_L3);
              break;
    case 'h': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +2 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=2; /* 40usec */
	      trigger_set(SEC_PP_ITD_L2);
              break;
    case 'i': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Left +1 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=1; /* 20usec */
	      trigger_set(SEC_PP_ITD_L1);
              break;

    case 'j': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +1 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-1; /* 20usec */
	      trigger_set(SEC_PP_ITD_R1);
              break;
    case 'k': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +2 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-2; /* 40usec */
	      trigger_set(SEC_PP_ITD_R2);
              break;
    case 'l': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +3 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-3; /* 60usec */
	      trigger_set(SEC_PP_ITD_R3);
              break;
    case 'm': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +4 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-5; /* 100usec */
	      trigger_set(SEC_PP_ITD_R4);
              break;
    case 'n': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +5 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-10; /* 200usec */
	      trigger_set(SEC_PP_ITD_R5);
              break;
    case 'o': para_iiditdpplp_current_iid_amp_l=AMP_0DB; /* ITD: Right +6 */
              para_iiditdpplp_current_iid_amp_r=AMP_0DB;
              para_iiditdpplp_itd_lr_delta=-30; /* 600usec */
	      trigger_set(SEC_PP_ITD_R6);
    default:  break;
   }
  }
  else if (counter2==para_iiditdpplp_t3) {
    para_iiditdpplp_current_iid_amp_l=0; /* Silence */
    para_iiditdpplp_current_iid_amp_r=0;
    para_iiditdpplp_itd_lr_delta=0;
  }
  counter2++; counter2%=para_iiditdpplp_t4; /* 3seconds? */
  counter3++;
  if (counter3==para_iiditdpplp_t6) { /* Stop */
   audio_active=0;
//   stim_reset();
#ifdef OCTOPUS_STIM_COMEDI
   comedi_data_write(daqcard,1,0,0,AREF_GROUND,dac_0);
   comedi_data_write(daqcard,1,1,0,AREF_GROUND,dac_1);
#endif
   rt_sem_wait(&audio_sem);
    rtf_reset(STIM_B2FFIFO);
    rtf_reset(STIM_F2BFIFO);
   rt_sem_signal(&audio_sem);
   rt_printk("octopus-stim-rtai.o: Stim stopped.\n");
  } else if (counter3%para_iiditdpplp_t5==0) {
   dac_0=dac_1=0; audio_paused=1; pause_trigger_hi=0; /* 39let? */
   trigger_set(SEC_PAUSE);
   rt_printk("offset: %d\n",counter3);
  }
 } else {
  pause_trigger_hi++;
 }
}
