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

/* PARADIGM 'Testing of Opponent Channels hypothesis'
    counter0: soa counter
    -- longer SOA
    -- squareburst of 400ms stim duration */

static int para_itdlintest_trigger,
	   para_itdlintest_soa,para_itdlintest_hi_duration,
	   para_itdlintest_click_period,
	   para_itdlintest_delta1,para_itdlintest_delta2,
	   para_itdlintest_stim_instant,
	   para_itdlintest_stim_instant_minus1,para_itdlintest_stim_instant_minus2,
	   para_itdlintest_stim_instant_plus1,para_itdlintest_stim_instant_plus2;

static void para_itdlintest_init(void) {
 counter0=0; current_pattern_offset=0;
 para_itdlintest_soa=(2.00201)*AUDIO_RATE; /* 100100 steps - 2.02 seconds - 143 cycles */
 para_itdlintest_hi_duration=(0.00051)*AUDIO_RATE; /* 25 steps */
 para_itdlintest_click_period=(0.014)*AUDIO_RATE; /* 700 steps */
 para_itdlintest_delta1=(0.00031)*AUDIO_RATE; /*  L-R delta1: 15 steps */
 para_itdlintest_delta2=(0.00061)*AUDIO_RATE; /*  L-R delta2: 30 steps */

 para_itdlintest_stim_instant=para_itdlintest_click_period/2;
 para_itdlintest_stim_instant_minus1=para_itdlintest_stim_instant-para_itdlintest_delta1/2;
 para_itdlintest_stim_instant_minus2=para_itdlintest_stim_instant-para_itdlintest_delta2/2;
 para_itdlintest_stim_instant_plus1=para_itdlintest_stim_instant+para_itdlintest_delta1/2;
 para_itdlintest_stim_instant_plus2=para_itdlintest_stim_instant+para_itdlintest_delta2/2;

 rt_printk("%d %d %d %d %d %d %d %d %d %d\n",para_itdlintest_soa,
		 	 para_itdlintest_hi_duration,
		 	 para_itdlintest_click_period,
		 	 para_itdlintest_delta1,
		 	 para_itdlintest_delta2,
		 	 para_itdlintest_stim_instant,
		 	 para_itdlintest_stim_instant_minus1,
		 	 para_itdlintest_stim_instant_minus2,
		 	 para_itdlintest_stim_instant_plus1,
		 	 para_itdlintest_stim_instant_plus2);
}

static void para_itdlintest(void) {
 int dummy_counter=0;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
  current_pattern_offset++;
  /* Roll-over */
  if (current_pattern_offset==pattern_size) current_pattern_offset=0;

  switch (current_pattern_data) {
   case 'A': /* Center - Left 300us */
	     para_itdlintest_trigger=SEC_C_L300;
             break;
   case 'B': /* Left 300us - Left 600us */
	     para_itdlintest_trigger=SEC_L300_L600;
             break;
   case 'C': /* Left 600us - Left 300us */
	     para_itdlintest_trigger=SEC_L600_L300;
             break;
   case 'D': /* Left 300us - Center */
	     para_itdlintest_trigger=SEC_L300_C;
             break;
   case 'E': /* Center - Right 300us */
	     para_itdlintest_trigger=SEC_C_R300;
             break;
   case 'F': /* Right 300us - Right 600us */
	     para_itdlintest_trigger=SEC_R300_R600;
             break;
   case 'G': /* Right 600us - Right 300us */
	     para_itdlintest_trigger=SEC_R600_R300;
             break;
   case 'H': /* Right 300us - Center */
	     para_itdlintest_trigger=SEC_R300_C;
             break;
   case 'I': /* Center - Left 600us */
	     para_itdlintest_trigger=SEC_C_L600;
             break;
   case 'J': /* Left 600us - Right 600us */
	     para_itdlintest_trigger=SEC_L600_R600;
             break;
   case 'K': /* Right 600us - Center */
	     para_itdlintest_trigger=SEC_R600_C;
             break;
   case 'L': /* Center - Right 600us */
	     para_itdlintest_trigger=SEC_C_R600;
             break;
   case 'M': /* Right 600us - Left 600us */
	     para_itdlintest_trigger=SEC_R600_L600;
             break;
   case 'N': /* Left 600us - Center */
	     para_itdlintest_trigger=SEC_L600_C;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if (trigger_active && (counter0 == para_itdlintest_stim_instant)) 
  trigger_set(para_itdlintest_trigger);
 /* ------------------------------------------------------------------- */

 dummy_counter=counter0%para_itdlintest_click_period;
 dac_0=dac_1=0;

 switch (current_pattern_data) {
  case 'D':	// Destination is Center
  case 'H':
  case 'K':
  case 'N':
	if ((dummy_counter >= para_itdlintest_stim_instant) &&
	    (dummy_counter <  para_itdlintest_stim_instant \
			 +para_itdlintest_hi_duration)) {
	 dac_0=dac_1=AMP_H20;
	}
	    break;
  case 'A':	// Destination is L300
  case 'C':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus1 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_H20;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus1+1 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_H20;
	}
	    break;
  case 'E':	// Destination is R300
  case 'G':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus1 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_H20;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus1+1 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_H20;
	}
	    break;
  case 'B':	// Destination is L600
  case 'I':
  case 'M':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_H20;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_H20;
	}
	    break;
  case 'F':	// Destination is R600
  case 'J':
  case 'L':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_H20;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_H20;
	}
  default:  break;
 }

 /* ------------------------------------------------------------------- */

 counter0++; counter0%=para_itdlintest_soa;
}
