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
    counter0: soa counter
    -- longer SOA
    -- squareburst of 400ms stim duration */

static int para_itdoppchn2_soa,para_itdoppchn2_trigger,
	   para_itdoppchn2_delta,
	   para_itdoppchn2_hilo_duration,para_itdoppchn2_hilo_duration2,
	   para_itdoppchn2_adapter_duration,para_itdoppchn2_adapter_instant,
	   para_itdoppchn2_adapter_instant_1,para_itdoppchn2_adapter_instant_2,
	   para_itdoppchn2_probe_duration,para_itdoppchn2_probe_instant,
	   para_itdoppchn2_probe_instant_1,para_itdoppchn2_probe_instant_2,
           dummyCounter,dummyCounter2;

static void para_itdoppchn2_init(void) {
 counter0=0; current_pattern_offset=0;
 para_itdoppchn2_delta=(0.00061)*AUDIO_RATE; /*  L-R delta: 30 steps - 0.00061 */
 para_itdoppchn2_hilo_duration2=(0.005)*AUDIO_RATE; /* 250 steps - 5 ms */
 para_itdoppchn2_hilo_duration=(0.05)*AUDIO_RATE; /* 2500 steps - 50 ms */
 para_itdoppchn2_soa=(4.0)*AUDIO_RATE; /* 200000 steps - 2 seconds */
 para_itdoppchn2_adapter_duration=(0.3)*AUDIO_RATE; /* 15000 steps - 300 ms */
 para_itdoppchn2_adapter_instant=para_itdoppchn2_soa/2;
 para_itdoppchn2_adapter_instant_1=para_itdoppchn2_adapter_instant-para_itdoppchn2_delta/2;
 para_itdoppchn2_adapter_instant_2=para_itdoppchn2_adapter_instant+para_itdoppchn2_delta/2;
 para_itdoppchn2_probe_duration=(0.1)*AUDIO_RATE; /* 5000 steps - 100 ms */
 para_itdoppchn2_probe_instant=para_itdoppchn2_adapter_instant+para_itdoppchn2_adapter_duration;
 para_itdoppchn2_probe_instant_1=para_itdoppchn2_probe_instant-para_itdoppchn2_delta/2;
 para_itdoppchn2_probe_instant_2=para_itdoppchn2_probe_instant+para_itdoppchn2_delta/2;

 rt_printk("%d %d %d %d %d %d %d %d %d %d %d %d\n",para_itdoppchn2_delta,
		 	 para_itdoppchn2_hilo_duration2,
		 	 para_itdoppchn2_hilo_duration,
		 	 para_itdoppchn2_soa,
		 	 para_itdoppchn2_adapter_instant,
		 	 para_itdoppchn2_adapter_instant_1,
		 	 para_itdoppchn2_adapter_instant_2,
		 	 para_itdoppchn2_adapter_duration,
		 	 para_itdoppchn2_probe_instant,
		 	 para_itdoppchn2_probe_instant_1,
		 	 para_itdoppchn2_probe_instant_2,
		 	 para_itdoppchn2_probe_duration);
}

static void para_itdoppchn2(void) {
 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
  current_pattern_offset++;
  /* Roll-over */
  if (current_pattern_offset==pattern_size) current_pattern_offset=0;

  switch (current_pattern_data) {
   case 'A': /* Adapter: Left leading - Probe: Left leading */
	     para_itdoppchn2_trigger=SEC_OPPCHN_LL;
             break;
   case 'B': /* Adapter: Left leading - Probe: Right leading */
	     para_itdoppchn2_trigger=SEC_OPPCHN_LR;
             break;
   case 'C': /* Adapter: Right leading - Probe: Right leading */
	     para_itdoppchn2_trigger=SEC_OPPCHN_RR;
             break;
   case 'D': /* Adapter: Right leading - Probe: Left leading */
	     para_itdoppchn2_trigger=SEC_OPPCHN_RL;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if (trigger_active && (counter0 == para_itdoppchn2_adapter_instant)) 
  trigger_set(para_itdoppchn2_trigger);
 /* ------------------------------------------------------------------- */

 dac_0=dac_1=DACZERO;

 /* Adapter - Early */
 if ((counter0 >= para_itdoppchn2_adapter_instant_1) &&
     (counter0 <  para_itdoppchn2_adapter_instant_1+para_itdoppchn2_adapter_duration)) {
  dummyCounter=((counter0-para_itdoppchn2_adapter_instant_1)/para_itdoppchn2_hilo_duration)&1;
  dummyCounter2=((counter0-para_itdoppchn2_adapter_instant_1)/para_itdoppchn2_hilo_duration2)&1;
  if ( !dummyCounter && !dummyCounter2 )
   switch (current_pattern_data) {
    case 'A':
    case 'B': dac_1=AMP_H20;
	      break;
    case 'C':
    case 'D': dac_0=AMP_H20;
    default:  break;
   }
 }

 /* Adapter - Late */
 if ((counter0 >= para_itdoppchn2_adapter_instant_2) &&
     (counter0 <  para_itdoppchn2_adapter_instant_2+para_itdoppchn2_adapter_duration)) {
  dummyCounter=((counter0-para_itdoppchn2_adapter_instant_2)/para_itdoppchn2_hilo_duration)&1;
  dummyCounter2=((counter0-para_itdoppchn2_adapter_instant_2)/para_itdoppchn2_hilo_duration2)&1;
  if ( !dummyCounter && !dummyCounter2 )
   switch (current_pattern_data) {
    case 'A':
    case 'B': dac_0=AMP_H20;
	      break;
    case 'C':
    case 'D': dac_1=AMP_H20;
    default:  break;
   }
 }

 /* ------------------------------------------------------------------- */

 /* Probe - Early */
 if ((counter0 >= para_itdoppchn2_probe_instant_1) &&
     (counter0 <  para_itdoppchn2_probe_instant_1+para_itdoppchn2_probe_duration)) {
  dummyCounter=((counter0-para_itdoppchn2_probe_instant_1)/para_itdoppchn2_hilo_duration)&1;
  dummyCounter2=((counter0-para_itdoppchn2_probe_instant_1)/para_itdoppchn2_hilo_duration2)&1;
  if ( dummyCounter && dummyCounter2 )
   switch (current_pattern_data) {
    case 'A':
    case 'D': dac_1=AMP_H20;
	      break;
    case 'B':
    case 'C': dac_0=AMP_H20;
    default:  break;
   }
 }

 /* Probe - Late */
 if ((counter0 >= para_itdoppchn2_probe_instant_2) &&
     (counter0 <  para_itdoppchn2_probe_instant_2+para_itdoppchn2_probe_duration)) {
  dummyCounter=((counter0-para_itdoppchn2_probe_instant_2)/para_itdoppchn2_hilo_duration)&1;
  dummyCounter2=((counter0-para_itdoppchn2_probe_instant_2)/para_itdoppchn2_hilo_duration2)&1;
  if ( dummyCounter && dummyCounter2 )
   switch (current_pattern_data) {
    case 'A':
    case 'D': dac_0=AMP_H20;
	      break;
    case 'B':
    case 'C': dac_1=AMP_H20;
    default:  break;
   }
 }

 /* ------------------------------------------------------------------- */

 counter0++; counter0%=para_itdoppchn2_soa; /* Four seconds? */
}
