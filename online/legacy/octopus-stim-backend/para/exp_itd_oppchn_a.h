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

/* PARADIGM 'Testing of Opponent Channels hypothesis'
    counter0: soa counter */

#define SEC_OPPCHN_CO		80	// "OPPCHN CO"
#define SEC_OPPCHN_CC		81	// "OPPCHN CC"
#define SEC_OPPCHN_CL		82	// "OPPCHN CL"
#define SEC_OPPCHN_CR		83	// "OPPCHN CR"
#define SEC_OPPCHN_LC		84	// "OPPCHN LC"
#define SEC_OPPCHN_LL		85	// "OPPCHN LL"
#define SEC_OPPCHN_LR		86	// "OPPCHN LR"
#define SEC_OPPCHN_RC		87	// "OPPCHN RC"
#define SEC_OPPCHN_RL		88	// "OPPCHN RL"
#define SEC_OPPCHN_RR		89	// "OPPCHN RR"

static int para_itdoppchn_pulse,para_itdoppchn_delta,
	   para_itdoppchn_stim,para_itdoppchn_soa,
	   para_itdoppchn_adapter_instant,para_itdoppchn_probe_instant,
	   para_itdoppchn_adapter_instant_1,para_itdoppchn_probe_instant_1,
	   para_itdoppchn_adapter_instant_2,para_itdoppchn_probe_instant_2,
           para_itdoppchn_trigger;

static void para_itdoppchn_init(void) {
 counter0=0; current_pattern_offset=0;
 para_itdoppchn_pulse=(0.0001)*AUDIO_RATE; /* 5 steps - 100 us */
 para_itdoppchn_delta=(0.00061)*AUDIO_RATE; /*  L-R delta: 30 steps */
 para_itdoppchn_stim=(0.1)*AUDIO_RATE; /* 5000 steps - 100 ms - 0.005 */
 para_itdoppchn_soa=(2.0)*AUDIO_RATE; /* 100000 steps - 2 seconds - 1.0 */
 para_itdoppchn_adapter_instant=para_itdoppchn_soa/2-para_itdoppchn_stim/2;
 para_itdoppchn_adapter_instant_1=para_itdoppchn_adapter_instant-para_itdoppchn_delta/2;
 para_itdoppchn_adapter_instant_2=para_itdoppchn_adapter_instant+para_itdoppchn_delta/2;
 para_itdoppchn_probe_instant=para_itdoppchn_soa/2+para_itdoppchn_stim/2;
 para_itdoppchn_probe_instant_1=para_itdoppchn_probe_instant-para_itdoppchn_delta/2;
 para_itdoppchn_probe_instant_2=para_itdoppchn_probe_instant+para_itdoppchn_delta/2;

// rt_printk("%d %d %d %d %d %d %d %d %d %d\n",para_itdoppchn_pulse,
//		 	 para_itdoppchn_delta,
//		 	 para_itdoppchn_stim,
//		 	 para_itdoppchn_soa,
//		 	 para_itdoppchn_adapter_instant,
//		 	 para_itdoppchn_adapter_instant_1,
//		 	 para_itdoppchn_adapter_instant_2,
//		 	 para_itdoppchn_probe_instant,
//		 	 para_itdoppchn_probe_instant_1,
//		 	 para_itdoppchn_probe_instant_2);
}

static void para_itdoppchn(void) {
 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
  current_pattern_offset++;
  /* Roll-over */
  if (current_pattern_offset==pattern_size) current_pattern_offset=0;
  switch (current_pattern_data) {
   case '0': /* Adapter: Center - Probe: Center */
	     para_itdoppchn_trigger=SEC_OPPCHN_CO;
             break;
   case 'A': /* Adapter: Center - Probe: Center */
	     para_itdoppchn_trigger=SEC_OPPCHN_CC;
             break;
   case 'B': /* Adapter: Center - Probe: Left-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_CL;
             break;
   case 'C': /* Adapter: Center - Probe: Right-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_CR;
             break;
   case 'D': /* Adapter: Left-leading - Probe: Center */
	     para_itdoppchn_trigger=SEC_OPPCHN_LC;
             break;
   case 'E': /* Adapter: Left-leading - Probe: Left-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_LL;
             break;
   case 'F': /* Adapter: Left-leading - Probe: Right-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_LR;
             break;
   case 'G': /* Adapter: Right-leading - Probe: Center */
	     para_itdoppchn_trigger=SEC_OPPCHN_RC;
             break;
   case 'H': /* Adapter: Right-leading - Probe: Left-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_RL;
             break;
   case 'I': /* Adapter: Right-leading - Probe: Right-leading */
	     para_itdoppchn_trigger=SEC_OPPCHN_RR;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if (trigger_active && (counter0 == para_itdoppchn_adapter_instant)) 
  trigger_set(para_itdoppchn_trigger);
 /* ------------------------------------------------------------------- */

 dac_0=dac_1=0;

 /* Adapter - Early */
 if ((counter0 >= para_itdoppchn_adapter_instant_1) &&
     (counter0 <  para_itdoppchn_adapter_instant_1+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case 'D':
   case 'E':
   case 'F': dac_1=AMP_H20;
	     break;
   case 'G':
   case 'H':
   case 'I': dac_0=AMP_H20;
   default:  break;
  }
 }

 /* Adapter - Center */
 if ((counter0 >= para_itdoppchn_adapter_instant) &&
     (counter0 <  para_itdoppchn_adapter_instant+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case '0':
   case 'A':
   case 'B':
   case 'C': dac_0=AMP_H20; dac_1=AMP_H20;
   default:  break;
  }
 }

 /* Adapter - Late */
 if ((counter0 >= para_itdoppchn_adapter_instant_2) &&
     (counter0 <  para_itdoppchn_adapter_instant_2+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case 'D':
   case 'E':
   case 'F': dac_0=AMP_H20;
	     break;
   case 'G':
   case 'H':
   case 'I': dac_1=AMP_H20;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */

 /* Probe - Early */
 if ((counter0 >= para_itdoppchn_probe_instant_1) &&
     (counter0 <  para_itdoppchn_probe_instant_1+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case 'B':
   case 'E':
   case 'H': dac_1=AMP_H20;
	     break;
   case 'C':
   case 'F':
   case 'I': dac_0=AMP_H20;
   default:  break;
  }
 }

 /* Probe - Center */
 if ( (counter0 >= para_itdoppchn_probe_instant) &&
      (counter0 <  para_itdoppchn_probe_instant+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case 'A':
   case 'D':
   case 'G': dac_0=AMP_H20; dac_1=AMP_H20;
   default:  break;
  }
 }

 /* Probe - Late */
 if ((counter0 >= para_itdoppchn_probe_instant_2) &&
     (counter0 <  para_itdoppchn_probe_instant_2+para_itdoppchn_pulse)) {
  switch (current_pattern_data) {
   case 'B':
   case 'E':
   case 'H': dac_0=AMP_H20;
	     break;
   case 'C':
   case 'F':
   case 'I': dac_1=AMP_H20;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */

 counter0++; counter0%=para_itdoppchn_soa; /* Two seconds? */
}
