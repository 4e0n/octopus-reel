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

#define SEC_C_L300		101	// "A:   C  -> L300    "
#define SEC_L300_L600		102	// "B: L300 -> L600    "
#define SEC_L600_L300		103	// "C: L600 -> L300    "
#define SEC_L300_C		104	// "D: L300 ->  C      "
#define SEC_C_R300		105	// "E:   C  -> R300    "
#define SEC_R300_R600		106	// "F: R300 -> R600    "
#define SEC_R600_R300		107	// "G: R600 -> R300    "
#define SEC_R300_C		108	// "H: R300 ->  C      "
#define SEC_C_L600		109	// "I:   C  -> L600 *  "
#define SEC_L600_R600		110	// "J: L600 -> R600 ** "
#define SEC_R600_C		111	// "K: R600 ->  C   *  "
#define SEC_C_R600		112	// "L:   C  -> R600 *  "
#define SEC_R600_L600		113	// "M: R600 -> L600 ** "
#define SEC_L600_C		114	// "N: L600 ->  C      "

static int para_itdlintest_experiment_loop=0;

static int para_itdlintest_rollback_count=3;
static int para_itdlintest_pause_interleave=0,para_itdlintest_trigger_interleave=0;

static int para_itdlintest_trigger,
	   para_itdlintest_soa,para_itdlintest_hi_duration,
	   para_itdlintest_click_period,
	   para_itdlintest_delta1,para_itdlintest_delta2,
	   para_itdlintest_stim_instant,
	   para_itdlintest_stim_instant_minus1,para_itdlintest_stim_instant_minus2,
	   para_itdlintest_stim_instant_plus1,para_itdlintest_stim_instant_plus2;

static void para_itdlintest_init(void) {
 current_pattern_offset=0;
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

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_itdlintest_start(void) {
 lights_dimm();
 counter0=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_itdlintest_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_itdlintest_pause(void) {
 audio_active=0;
 lights_on();

 // Resume rewinding to "para_itdlintest_rollback_count" trials before..
 // Trigger won't happen for this "para_itdlintest_rollback_count" of trials..
 if (manual_pause==1) {
  manual_pause=0;
  para_itdlintest_pause_interleave=0; /* No @ ahead, so don't skip some.. */
  current_pattern_offset=current_pattern_offset-para_itdlintest_rollback_count;
  para_itdlintest_trigger_interleave=para_itdlintest_rollback_count+1;
 } else if (para_itdlintest_rollback_count>0) {
  para_itdlintest_pause_interleave=1; /* Skip the @.. */
  current_pattern_offset=current_pattern_offset-para_itdlintest_rollback_count-1;
  para_itdlintest_trigger_interleave=para_itdlintest_rollback_count+1;
 }



 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_itdlintest_resume(void) {
 lights_dimm();
 counter0=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}


static void para_itdlintest(void) {
 int dummy_counter=0;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */

  if (para_itdlintest_trigger_interleave>0) para_itdlintest_trigger_interleave--;

  /* Interblock pause */
  if (current_pattern_data=='@') {
   if (para_itdlintest_pause_interleave==0) para_itdlintest_pause(); /* legitimate pause */
   else { /* supposed to *not* pause */
    para_itdlintest_pause_interleave=0;
    current_pattern_offset++;
    current_pattern_data=patt_buf[current_pattern_offset]; /* fetch next.. */
   }
  }
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
 if ( (counter0==para_itdlintest_stim_instant) && trigger_active &&
      (current_pattern_offset>0) && (para_itdlintest_trigger_interleave==0) )
  trigger_set(para_itdlintest_trigger);

 /* ------------------------------------------------------------------- */
 if (counter0==0) {
  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_itdlintest_experiment_loop==0) para_itdlintest_stop();
   //else para_itdlintest_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */

 dummy_counter=counter0%para_itdlintest_click_period;
 dac_0=dac_1=DACZERO;

 switch (current_pattern_data) {
  case 'D':	// Destination is Center
  case 'H':
  case 'K':
  case 'N':
	if ((dummy_counter >= para_itdlintest_stim_instant) &&
	    (dummy_counter <  para_itdlintest_stim_instant \
			 +para_itdlintest_hi_duration)) {
	 dac_0=dac_1=AMP_OPPCHN;
	}
        break;
  case 'A':	// Destination is L300
  case 'C':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus1 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus1+1 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
        break;
  case 'E':	// Destination is R300
  case 'G':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus1 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus1) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus1+1 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
        break;
  case 'B':	// Destination is L600
  case 'I':
  case 'M':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
        break;
  case 'F':	// Destination is R600
  case 'J':
  case 'L':
	if ((dummy_counter >= para_itdlintest_stim_instant_minus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_minus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest_stim_instant_plus2) &&
	    (dummy_counter <  para_itdlintest_stim_instant_plus2 \
			 +para_itdlintest_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
  default:  break;
 }

 /* ------------------------------------------------------------------- */

 counter0++; counter0%=para_itdlintest_soa;
}
