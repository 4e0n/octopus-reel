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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* ANOTHER PARADIGM for 'Testing of Opponent Channels hypothesis'
     *** a.k.a. EXPERIMENT 2 ***

     S12 Lateral -> Contralateral -> Center
     S10 Lateral -> Center
     S10 Lateral (Short/100ms) -> Center
     (for both sides)

    counter0: soa counter
    -- longer SOA
    -- squareburst of 400ms stim duration */

static int experiment_loop=0;

static int para_itdlintest2_trigger,
	   para_itdlintest2_no_stim_types,
	   para_itdlintest2_soa,
	   para_itdlintest2_hi_duration,
	   para_itdlintest2_click_period,
	   para_itdlintest2_delta,
	   para_itdlintest2_stim_instant,
	   para_itdlintest2_stim_instant_minus,
	   para_itdlintest2_stim_instant_plus,
	   para_itdlintest2_stim_timeoffset,
	   para_itdlintest2_stim_duration,
	   para_itdlintest2_stim12_part1_duration;

static void para_itdlintest2_init(void) {
 current_pattern_offset=0;
 para_itdlintest2_no_stim_types=6;
 para_itdlintest2_soa=(2.9401)*AUDIO_RATE; /* 150150 steps - 3.02 seconds */
 para_itdlintest2_hi_duration=(0.0005001)*AUDIO_RATE; /* 500us - 25 steps */
 para_itdlintest2_click_period=(0.014)*AUDIO_RATE; /* 14ms - 700 steps */
 para_itdlintest2_delta=(0.0006001)*AUDIO_RATE; /* L-R delta: 600us - 30 steps */
 
 para_itdlintest2_stim_timeoffset=para_itdlintest2_soa/2;
 para_itdlintest2_stim_duration=(0.50001)*AUDIO_RATE;
 para_itdlintest2_stim12_part1_duration=(0.10001)*AUDIO_RATE;

 para_itdlintest2_stim_instant=para_itdlintest2_click_period/2;
 para_itdlintest2_stim_instant_minus=para_itdlintest2_stim_instant \
				     -para_itdlintest2_delta/2;
 para_itdlintest2_stim_instant_plus=para_itdlintest2_stim_instant \
				    +para_itdlintest2_delta/2;
 experiment_loop=1;

 rt_printk("%d %d %d %d %d %d %d %d %d %d\n",
		 	 para_itdlintest2_hi_duration,
		 	 para_itdlintest2_click_period,
		 	 para_itdlintest2_delta,
		 	 para_itdlintest2_soa,
		 	 para_itdlintest2_stim_timeoffset,
		 	 para_itdlintest2_stim_duration,
		 	 para_itdlintest2_stim12_part1_duration,
		 	 para_itdlintest2_stim_instant,
		 	 para_itdlintest2_stim_instant_minus,
		 	 para_itdlintest2_stim_instant_plus);

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_itdlintest2_start(void) {
 lights_dimm();
 counter0=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_itdlintest2_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_itdlintest2_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_itdlintest2_resume(void) {
 lights_dimm();
 counter0=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}


static void para_itdlintest2(void) {
 int dummy_counter=0;

 if (counter0==0) {
  switch (current_pattern_offset) {
   case 0: /* Center to Left to Right 600us */
	     para_itdlintest2_trigger=SEC_S12_L2R;
             break;
   case 1: /* Center to Right to Left 600us */
	     para_itdlintest2_trigger=SEC_S12_R2L;
             break;
   case 2: /* Center to Left 600us */
	     para_itdlintest2_trigger=SEC_S10_L;
             break;
   case 3: /* Center to Right 600us */
	     para_itdlintest2_trigger=SEC_S10_R;
             break;
   case 4: /* Center to Left 600us  - 100ms duration */
	     para_itdlintest2_trigger=SEC_S10S_L;
             break;
   case 5: /* Center to Left 600us  - 100ms duration */
	     para_itdlintest2_trigger=SEC_S10S_R;
   default:  break;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_itdlintest2_stim_timeoffset) && trigger_active)
      // && (current_pattern_offset>0) )
  trigger_set(para_itdlintest2_trigger);

 /* ------------------------------------------------------------------- */

 dummy_counter=counter0%para_itdlintest2_click_period;
 dac_0=dac_1=0; /* default is zero */

 /* We'll decide depending on time offset that;
    whether, either left or right channel will turn to high. */
 if ( (counter0>=para_itdlintest2_stim_timeoffset) && \
      (counter0< para_itdlintest2_stim_timeoffset \
                 +para_itdlintest2_stim_duration) ) {
  switch (current_pattern_offset) {
   case 0:
	if ( (counter0>=para_itdlintest2_stim_timeoffset) && \
	     (counter0< para_itdlintest2_stim_timeoffset \
	      		+para_itdlintest2_stim12_part1_duration) ) {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	} else {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	}
	break;
   case 1:
	if ( (counter0>=para_itdlintest2_stim_timeoffset) && \
	     (counter0< para_itdlintest2_stim_timeoffset \
	      		+para_itdlintest2_stim12_part1_duration) ) {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	} else {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	}
	break;
   case 2:
	if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	    (dummy_counter <  para_itdlintest2_stim_instant_minus \
			 +para_itdlintest2_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	    (dummy_counter <  para_itdlintest2_stim_instant_plus \
			 +para_itdlintest2_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
	break;
   case 3:
	if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	    (dummy_counter <  para_itdlintest2_stim_instant_minus \
			 +para_itdlintest2_hi_duration)) {
	 dac_1=AMP_OPPCHN;
	}
	if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	    (dummy_counter <  para_itdlintest2_stim_instant_plus \
			 +para_itdlintest2_hi_duration)) {
	 dac_0=AMP_OPPCHN;
	}
	break;
   case 4:
	if ( (counter0>=para_itdlintest2_stim_timeoffset) && \
	     (counter0< para_itdlintest2_stim_timeoffset \
	      		+para_itdlintest2_stim12_part1_duration) ) {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	} else if ((dummy_counter >= para_itdlintest2_stim_instant) &&
	           (dummy_counter <  para_itdlintest2_stim_instant \
	                             +para_itdlintest2_hi_duration)) {
	 dac_0=dac_1=AMP_OPPCHN;
	}
	break;
   case 5:
	if ( (counter0>=para_itdlintest2_stim_timeoffset) && \
	     (counter0< para_itdlintest2_stim_timeoffset \
	      		+para_itdlintest2_stim12_part1_duration) ) {
	 if ((dummy_counter >= para_itdlintest2_stim_instant_minus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_minus \
	 		       +para_itdlintest2_hi_duration)) {
	  dac_1=AMP_OPPCHN;
	 }
	 if ((dummy_counter >= para_itdlintest2_stim_instant_plus) &&
	     (dummy_counter <  para_itdlintest2_stim_instant_plus \
			       +para_itdlintest2_hi_duration)) {
	  dac_0=AMP_OPPCHN;
	 }
	} else if ((dummy_counter >= para_itdlintest2_stim_instant) &&
	     (dummy_counter <  para_itdlintest2_stim_instant \
	                       +para_itdlintest2_hi_duration)) {
	 dac_0=dac_1=AMP_OPPCHN;
	}
   default: break;
  }
 } else if ((dummy_counter >= para_itdlintest2_stim_instant) &&
            (dummy_counter <  para_itdlintest2_stim_instant \
                              +para_itdlintest2_hi_duration)) {
  dac_0=dac_1=AMP_OPPCHN;
 }

 /* ------------------------------------------------------------------- */

 counter0++; counter0%=para_itdlintest2_soa;

 if (counter0==0) {
  current_pattern_offset++;
  current_pattern_offset%=para_itdlintest2_no_stim_types;
  if (current_pattern_offset==0 && experiment_loop==0) para_itdlintest2_stop();
 }

}
