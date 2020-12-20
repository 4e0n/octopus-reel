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

/* PARADIGM 'IIDITD'
    counter0: mod counter
    counter1: base counter
    counter2: t3 counter */

static int para_iiditd_base_level_l,para_iiditd_base_level_r,
           para_iiditd_current_iid_amp_l,para_iiditd_current_iid_amp_r,
           para_iiditd_t1,para_iiditd_t2,para_iiditd_t3,
           para_iiditd_itd_lr_delta,para_iiditd_mono;

static void para_iiditd_init(void) {
 para_iiditd_base_level_l=para_iiditd_base_level_r=0;
 para_iiditd_current_iid_amp_l=para_iiditd_current_iid_amp_r=DA_NORM;
 para_iiditd_t1=25;   /*  500usec up, 9.5 msec down */
 para_iiditd_t2=500;  /* 10.0msec */
 para_iiditd_t3=2500; /* 50.0msec */
 para_iiditd_itd_lr_delta=PHASE_LL; /* Left is leading */
 para_iiditd_mono=0; current_pattern_offset=0;
}

static void para_iiditd(void) {
 if (para_iiditd_base_level_l) /* Set IID Left Amplitude */
  dac_0=para_iiditd_current_iid_amp_l; else dac_0=0;
 if (para_iiditd_base_level_r) /* Set IID Right Amplitude */
  dac_1=para_iiditd_current_iid_amp_r; else dac_1=0;
 counter0++; counter0%=para_iiditd_t2;
 counter1=counter0-para_iiditd_t2/2; /* Adjust to [-250,+250) */

 /* ITD shift for left */
 if (para_iiditd_base_level_l &&
     counter1==-para_iiditd_itd_lr_delta/2+para_iiditd_t1)
  para_iiditd_base_level_l=0;
 else if (counter1==-para_iiditd_itd_lr_delta/2)
  para_iiditd_base_level_l=1;

 /* ITD shift for right */
 if (para_iiditd_base_level_r &&
     counter1== para_iiditd_itd_lr_delta/2+para_iiditd_t1)
  para_iiditd_base_level_r=0;
 else if (counter1==para_iiditd_itd_lr_delta/2)
  para_iiditd_base_level_r=1;

 if (counter2==0) {
  current_pattern_data=himem_buffer[current_pattern_offset]; /* fetch new.. */
  current_pattern_offset++;

  /* Roll-over */
  if (current_pattern_offset==pattern_size) current_pattern_offset=0;
  switch (current_pattern_data) {
   case '0': para_iiditd_current_iid_amp_l=AMP_L20; /* IID: Both L & R low.. */
             para_iiditd_current_iid_amp_r=AMP_L20; /* ITD=0 */
             para_iiditd_itd_lr_delta=0;
             break;
   case '1': para_iiditd_current_iid_amp_l=AMP_H20; /* IID: Both L & R high.. */
             para_iiditd_current_iid_amp_r=AMP_H20; /* ITD=0; */
             para_iiditd_itd_lr_delta=0;
             break;
   case '2': para_iiditd_current_iid_amp_l=AMP_H20; /* IID: L high, R low.. */
             para_iiditd_current_iid_amp_r=AMP_L20;
             para_iiditd_itd_lr_delta=0;
             if (trigger_active) trigger_set(SEC_IID_LEFT);
             break;
   case '3': para_iiditd_current_iid_amp_l=AMP_H20; /* ITD: Source to Left.. */
             para_iiditd_current_iid_amp_r=AMP_H20;
             para_iiditd_itd_lr_delta=PHASE_LL;
             if (trigger_active) trigger_set(SEC_ITD_LEFT);
             break;
   case '4': para_iiditd_current_iid_amp_l=AMP_L20; /* IID: L low, R high.. */
             para_iiditd_current_iid_amp_r=AMP_H20;
             para_iiditd_itd_lr_delta=0;
             if (trigger_active) trigger_set(SEC_IID_RIGHT);
             break;
   case '5': para_iiditd_current_iid_amp_l=AMP_H20; /* ITD: Source to Right.. */
             para_iiditd_current_iid_amp_r=AMP_H20;
             para_iiditd_itd_lr_delta=PHASE_RL;
             if (trigger_active) trigger_set(SEC_ITD_RIGHT);
   default:  break;
  }
 } else if (counter2==TRIG_HI_STEPS) trigger_reset(); //pull down

 counter2++; counter2%=para_iiditd_t3; /* 50ms? */
 if (para_iiditd_mono==1) dac_1=0;      /* Left Only */
 else if (para_iiditd_mono==2) dac_0=0;	/* Right Only */
}
