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

/* PARADIGM 'Adapter-Probe' matched vs. unmatched
 *          'Ipsilateral vs. Contralateral'
 *          'Tones vs click trains'

 * Tones are 500Hz and 1150Hz
 *
 * In this version there's the first adapter followed by a second adapter
 * which is relatively longer with a randomized duration (i.e. 200 to 500ms)
 * The probe coming afterwards is identical to the first adapter. */

#define RAMP0 ((double)(0.2001))
#define RAMP1 ((double)(0.8001))
#define RAMP2 ((double)(1.0001)/(double)(60.0001))
#define RAMP3 ((double)(1.0001)-(double)(1.0001)/(double)(60.0001))

static sampl_t *adac_0,*adac_1,*adac_2,*adac_3; /* for easy switching in between dac output channels */

static int para_itd_pip_rand_experiment_loop=1;

static int para_itd_pip_rand_ad2_hi_dur,
	   para_itd_pip_rand_ad2_hi_dur_min, /* 400ms */
	   para_itd_pip_rand_ad2_rand,       /* +400ms */
	   para_itd_pip_rand_ad2_randmax;

static int para_itd_pip_rand_trigger,

	   para_itd_pip_rand_soa_intertrial,

	   /* Stim atomic properties */
	   para_itd_pip_rand_click_period, /* 10ms */
	   para_itd_pip_rand_hi_period,    /* One period: 500us high,9.5ms low = 10ms */

	   para_itd_pip_rand_adaprobe_hi_dur, /* 50ms */
	   para_itd_pip_rand_adaprobe_lo_dur, /* 200ms */
	   para_itd_pip_rand_adaprobe_tot_dur, /* 250ms */

	   para_itd_pip_rand_no_periods_adaprobe, /* 5: 50ms (same for both first adapter and probe) */
	   para_itd_pip_rand_lr_delta,

	   para_itd_pip_rand_stim_instant, /* 200ms (arbitrary) after absolute zero  */

	   para_itd_pip_rand_m_mm,
	   para_itd_pip_rand_istone,

	   para_itd_pip_rand_stim_local_offset,
	   para_itd_pip_rand_stim_instant_minus,
	   para_itd_pip_rand_stim_instant_plus;

static double para_itd_pip_rand_theta,para_itd_pip_rand_phi,
              para_itd_pip_rand_delta_phi, /* For modulation */
              para_itd_pip_rand_delta_theta500,
	      para_itd_pip_rand_delta_theta1150;

// ----------------------------------------------------------------------

static void para_itd_pip_rand_init(void) {
 current_pattern_offset=0;

 para_itd_pip_rand_soa_intertrial=(3.50001)*AUDIO_RATE; /* 3.5 seconds */

 para_itd_pip_rand_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps high - 675 steps low */
 para_itd_pip_rand_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 500 steps - was 700 steps (14ms) in some exp. */

 /* --------- */
 para_itd_pip_rand_no_periods_adaprobe=5; /* 5*10 ms */

 para_itd_pip_rand_adaprobe_hi_dur=para_itd_pip_rand_click_period*para_itd_pip_rand_no_periods_adaprobe; /* 50ms */
 para_itd_pip_rand_adaprobe_lo_dur=para_itd_pip_rand_adaprobe_hi_dur*4; /* 200ms */
 para_itd_pip_rand_adaprobe_tot_dur=para_itd_pip_rand_adaprobe_hi_dur+para_itd_pip_rand_adaprobe_lo_dur; /* 250ms */

 para_itd_pip_rand_ad2_hi_dur_min=para_itd_pip_rand_adaprobe_hi_dur*8; /* 400ms at its minimum */
 para_itd_pip_rand_ad2_randmax=para_itd_pip_rand_adaprobe_hi_dur*8;    /* randomize upto +400ms */

 /* --------- */

 para_itd_pip_rand_lr_delta=(0.00061)*AUDIO_RATE; /*  L-R delta2: 30 steps */

 para_itd_pip_rand_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_itd_pip_rand_stim_instant_minus=para_itd_pip_rand_stim_instant-para_itd_pip_rand_lr_delta/2;
 para_itd_pip_rand_stim_instant_plus=para_itd_pip_rand_stim_instant+para_itd_pip_rand_lr_delta/2;

 para_itd_pip_rand_delta_phi=50.0/(double)(AUDIO_RATE); /* f=50Hz */
 para_itd_pip_rand_delta_theta500=500.0/(double)(AUDIO_RATE); /* f=500Hz */
 para_itd_pip_rand_delta_theta1150=1150.0/(double)(AUDIO_RATE); /* f=1150Hz */

 rt_printk("%d %d %d %d %d %d %d %d %d\n",
		 	 para_itd_pip_rand_soa_intertrial,	// 100000 2s
		 	 para_itd_pip_rand_ad2_rand,		// 43
		 	 para_itd_pip_rand_hi_period,		// 25    500us
		 	 para_itd_pip_rand_click_period,	// 500   10ms
			 para_itd_pip_rand_adaprobe_hi_dur,	// 2500 50ms
			 (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP2),	// 41 820us -- ramp up-down
		 	 para_itd_pip_rand_lr_delta,		// 30    600us
		 	 para_itd_pip_rand_stim_instant,	// 10000 +200ms
		 	 para_itd_pip_rand_stim_instant_minus,	//  9985 +200ms-300us
		 	 para_itd_pip_rand_stim_instant_plus);	// 10015 +200ms+300us

 rt_printk("%d %d %d %d\n",(int)(1000.0*RAMP0),(int)(1000.0*RAMP1),(int)(1000.0*RAMP2),(int)(1000.0*RAMP3)); // 0.2,0.8,0.016,0.983

 //for (i=0;i<1000;i++) {
 // get_random_bytes(&para_itd_pip_rand_ad2_rand,sizeof(int));
 // para_itd_pip_rand_ad2_rand%=para_itd_pip_rand_ad2_randmax;
 // rt_printk("%ld ",(abs(para_itd_pip_rand_ad2_rand)));
 //}
 //rt_printk("\n");

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_itd_pip_rand_start(void) {
 dac_0=dac_1=0;
 lights_dimm();
 counter0=0; counter1=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_itd_pip_rand_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_itd_pip_rand_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_itd_pip_rand_resume(void) {
 lights_dimm();
 counter0=0; counter1=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_itd_pip_rand(void) {
 double adapt_theta_add,probe_theta_add;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */

  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_itd_pip_rand_pause(); /* legitimate pause */
	     break;

   case 'A': /* Tone Pip 500Hz -> 500Hz */
	     para_itd_pip_rand_trigger=SEC_PIP_TONE_LL;
	     para_itd_pip_rand_istone=1; para_itd_pip_rand_m_mm=0;
             break;
   case 'B': /* Tone Pip 1150Hz -> 1150Hz */
	     para_itd_pip_rand_trigger=SEC_PIP_TONE_HH;
	     para_itd_pip_rand_istone=1; para_itd_pip_rand_m_mm=1;
             break;
   case 'C': /* Tone Pip 500Hz -> 1150Hz */
	     para_itd_pip_rand_trigger=SEC_PIP_TONE_LH;
	     para_itd_pip_rand_istone=1; para_itd_pip_rand_m_mm=2;
             break;
   case 'D': /* Tone Pip 1150Hz -> 500Hz */
	     para_itd_pip_rand_trigger=SEC_PIP_TONE_HL;
	     para_itd_pip_rand_istone=1; para_itd_pip_rand_m_mm=3;
             break;
   case 'E': /* Click Train Pip Left -> Left */
	     para_itd_pip_rand_trigger=SEC_PIP_CTRAIN_LL;
	     para_itd_pip_rand_istone=0; para_itd_pip_rand_m_mm=0;
             break;
   case 'F': /* Click Train Pip Right -> Right */
	     para_itd_pip_rand_trigger=SEC_PIP_CTRAIN_RR;
	     para_itd_pip_rand_istone=0; para_itd_pip_rand_m_mm=1;
             break;
   case 'G': /* Click Train Pip Left -> Right */
	     para_itd_pip_rand_trigger=SEC_PIP_CTRAIN_LR;
	     para_itd_pip_rand_istone=0; para_itd_pip_rand_m_mm=2;
             break;
   case 'H': /* Click Train Pip Right -> Left */
	     para_itd_pip_rand_trigger=SEC_PIP_CTRAIN_RL;
	     para_itd_pip_rand_istone=0; para_itd_pip_rand_m_mm=3;
   default:  break;
  }

  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_itd_pip_rand_experiment_loop==0) para_itd_pip_rand_stop();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger #1 (Adapter-Probe Combination) */
 if ( (counter0==para_itd_pip_rand_stim_instant) && trigger_active && \
      (current_pattern_offset>0) ) {
  trigger_set(para_itd_pip_rand_trigger);
 } 

 if (para_itd_pip_rand_istone) {
  switch (para_itd_pip_rand_m_mm) {
   case 0: /* Center (0deg ITD) - 500Hz -> 500Hz  */
           adapt_theta_add=para_itd_pip_rand_delta_theta500; probe_theta_add=para_itd_pip_rand_delta_theta500; break;
   case 1: /* Center (0deg ITD) - 1150Hz -> 1150Hz  */
           adapt_theta_add=para_itd_pip_rand_delta_theta1150; probe_theta_add=para_itd_pip_rand_delta_theta1150; break;
   case 2: /* Center (0deg ITD) - 500Hz -> 1150Hz  */
           adapt_theta_add=para_itd_pip_rand_delta_theta500; probe_theta_add=para_itd_pip_rand_delta_theta1150; break;
   case 3: /* Center (0deg ITD) - 1150Hz -> 500Hz  */
           adapt_theta_add=para_itd_pip_rand_delta_theta1150; probe_theta_add=para_itd_pip_rand_delta_theta500; break;
  }
 } else { // Lateralized ITD
  switch (para_itd_pip_rand_m_mm) {
   case 0: /* RIGHT IPSILATERAL ITD  -- Adapter: DAC1-DAC0 Probe: DAC1-DAC0 */
	   adac_0=&dac_1; adac_1=&dac_0; adac_2=&dac_1; adac_3=&dac_0; break;
   case 1: /* LEFT  IPSILATERAL ITD  -- Adapter: DAC0-DAC1 Probe: DAC0-DAC1 */
	   adac_0=&dac_0; adac_1=&dac_1; adac_2=&dac_0; adac_3=&dac_1; break;
   case 2: /* RIGHT TRANSLATERAL ITD -- Adapter: DAC1-DAC0 Probe: DAC0-DAC1 */
	   adac_0=&dac_1; adac_1=&dac_0; adac_2=&dac_0; adac_3=&dac_1; break;
   case 3: /* LEFT  TRANSLATERAL ITD -- Adapter: DAC0-DAC1 Probe: DAC1-DAC0 */
	   adac_0=&dac_0; adac_1=&dac_1; adac_2=&dac_1; adac_3=&dac_0; break;
  }
 }

 /* Identify current temporal region */

 if (para_itd_pip_rand_istone) { // IS TONE {'A','B','C','D'}

  // ADAPTER #1 -- TONE

  if (counter0 >= para_itd_pip_rand_stim_instant && \
      counter0 <  para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur) {
   if (counter0==para_itd_pip_rand_stim_instant) {
    para_itd_pip_rand_theta=para_itd_pip_rand_phi=0.0; counter1=0; // Reset sine params

    //dac_0=dac_1=32767; // Edge trigger for oscilloscope view
   }

   // Reset phi for outro damping
   if (counter1 == (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1)) para_itd_pip_rand_phi=0.0;

   /* Intro (cosine ramp up) */
   //if (counter1 >=1 && counter1 < (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP0)) { // Osc.trig config
   if (counter1 >=0 && counter1 < (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP0)) { // 0.2
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 );
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Plateau */
   if (counter1 >= (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP0) && \
       counter1 <  (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1)) { // 0.8
    dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_rand_theta));
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
   }

   /* Outro (cosine ramp down) */
   if (counter1 >= (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1) && \
     counter1 <  para_itd_pip_rand_adaprobe_hi_dur) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1- (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 ));
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Silent for 200ms */
   if (counter1 >= para_itd_pip_rand_adaprobe_hi_dur) dac_0=dac_1=0;

   counter1++;
  }

  // ADAPTER #2 (RANDOMIZED DURATION) -- TONE

  if (counter0==para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur) {
   para_itd_pip_rand_theta=para_itd_pip_rand_phi=0.0; counter1=0; // Reset sine params
   // A random amount (0ms-400ms) will be added here
   get_random_bytes(&para_itd_pip_rand_ad2_rand,sizeof(int));
   para_itd_pip_rand_ad2_rand%=para_itd_pip_rand_ad2_randmax;
   // A random period in between 400ms-800ms
   para_itd_pip_rand_ad2_hi_dur=para_itd_pip_rand_ad2_hi_dur_min+abs(para_itd_pip_rand_ad2_rand);
   //rt_printk("%d\n",para_itd_pip_rand_ad2_rand); // log for hist plotting
  }

  if (counter0 >= para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur && \
      counter0 <  para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur \
                                                +para_itd_pip_rand_ad2_hi_dur \
					        +para_itd_pip_rand_adaprobe_lo_dur) {
   // Reset phi for outro damping
   if (counter1 == (int)(para_itd_pip_rand_ad2_hi_dur-500)) para_itd_pip_rand_phi=0.0;

   /* Intro (cosine ramp up) */
   if (counter1 < (int)(500)) { // 10ms
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 );
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Plateau */
   // --> For the calculated duration
   if (counter1 >= (int)(500) && \
       counter1 <  (int)(para_itd_pip_rand_ad2_hi_dur-500)) {
    dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_rand_theta));
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
   }

   /* Outro (cosine ramp down) */
   if (counter1 >= (int)(para_itd_pip_rand_ad2_hi_dur-500) && \
       counter1 <  para_itd_pip_rand_ad2_hi_dur) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1- (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 ));
    para_itd_pip_rand_theta+=adapt_theta_add; // Adapter
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Silent for 200ms */
   if (counter1 >= para_itd_pip_rand_ad2_hi_dur) dac_0=dac_1=0;

   counter1++;
  }

  // PROBE (IDENTICAL TO ADAPTER #1 IN TERMS OF DURATIONS) -- TONE
 
  if (counter0 >= para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur \
		                                +para_itd_pip_rand_ad2_hi_dur \
					        +para_itd_pip_rand_adaprobe_lo_dur && \
      counter0 <  para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur \
                                                +para_itd_pip_rand_ad2_hi_dur \
					        +para_itd_pip_rand_adaprobe_lo_dur\
 					        +para_itd_pip_rand_adaprobe_tot_dur) {

   if (counter0==para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur \
 		                               +para_itd_pip_rand_ad2_hi_dur \
					       +para_itd_pip_rand_adaprobe_lo_dur) {
    /* Trigger #2 (Probe instant) */
    trigger_set(SEC_PIP_PROBE);
    para_itd_pip_rand_theta=para_itd_pip_rand_phi=0.0; counter1=0; // Reset sine params
   }

   // Reset phi for outro damping
   if (counter1 == (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1)) para_itd_pip_rand_phi=0.0;

   /* Intro (cosine ramp up) */
   if (counter1 < (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP0)) { // 0.2
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 );
    para_itd_pip_rand_theta+=probe_theta_add; // Probe
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Plateau */
   if (counter1 >= (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP0) && \
       counter1 <  (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1)) { // 0.8
    dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_rand_theta));
    para_itd_pip_rand_theta+=probe_theta_add; // Probe
   }

   /* Outro (cosine ramp down) */
   if (counter1 >= (int)(para_itd_pip_rand_adaprobe_hi_dur*RAMP1) && \
     counter1 <  para_itd_pip_rand_adaprobe_hi_dur) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_rand_theta)* \
                 (1- (1-cos(2.0*M_PI*para_itd_pip_rand_phi))/2.0 ));
    para_itd_pip_rand_theta+=probe_theta_add; // Probe
    para_itd_pip_rand_phi+=para_itd_pip_rand_delta_phi;
   }

   /* Silent for 200ms */
   if (counter1 >= para_itd_pip_rand_adaprobe_hi_dur) dac_0=dac_1=0;

   counter1++;
  }

 } else { // IS LATERALIZED (ITD) CLICKTRAIN {'E','F','G','H'}

  // ADAPTER #1 -- LAT.ITD

  // LEAD
  if (counter0 >= para_itd_pip_rand_stim_instant_minus && \
      counter0 <  para_itd_pip_rand_stim_instant_minus+para_itd_pip_rand_adaprobe_tot_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-para_itd_pip_rand_stim_instant_minus;
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_adaprobe_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_0=AMP_OPPCHN;
    else *adac_0=0;
   } else *adac_0=0;
  }
  // LAG
  if (counter0 >= para_itd_pip_rand_stim_instant_plus && \
      counter0 <  para_itd_pip_rand_stim_instant_plus+para_itd_pip_rand_adaprobe_tot_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-para_itd_pip_rand_stim_instant_plus;
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_adaprobe_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_1=AMP_OPPCHN;
    else *adac_1=0;
   } else *adac_1=0;
  }

  // ADAPTER #2 (RANDOMIZED DURATION) -- LAT.ITD
 
  if (counter0==para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur) {
   // A random amount (0ms-400ms) will be added here
   get_random_bytes(&para_itd_pip_rand_ad2_rand,sizeof(int));
   para_itd_pip_rand_ad2_rand%=para_itd_pip_rand_ad2_randmax;
   // A random period in between 400ms-800ms
   para_itd_pip_rand_ad2_hi_dur=para_itd_pip_rand_ad2_hi_dur_min+abs(para_itd_pip_rand_ad2_rand);
   //rt_printk("%d\n",para_itd_pip_rand_ad2_rand); // log for hist plotting
  }

  // LEAD
  if (counter0 >= para_itd_pip_rand_stim_instant_minus+para_itd_pip_rand_adaprobe_tot_dur && \
      counter0 <  para_itd_pip_rand_stim_instant_minus+para_itd_pip_rand_adaprobe_tot_dur \
                                                      +para_itd_pip_rand_ad2_hi_dur \
					              +para_itd_pip_rand_adaprobe_lo_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-(para_itd_pip_rand_stim_instant_minus+
		                                 +para_itd_pip_rand_adaprobe_tot_dur);
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_ad2_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_0=AMP_OPPCHN;
    else *adac_0=0;
   } else *adac_0=0;
  }
  // LAG
  if (counter0 >= para_itd_pip_rand_stim_instant_plus+para_itd_pip_rand_adaprobe_tot_dur && \
      counter0 <  para_itd_pip_rand_stim_instant_plus+para_itd_pip_rand_adaprobe_tot_dur \
                                                     +para_itd_pip_rand_ad2_hi_dur \
					             +para_itd_pip_rand_adaprobe_lo_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-(para_itd_pip_rand_stim_instant_plus+
		                                 +para_itd_pip_rand_adaprobe_tot_dur);
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_ad2_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_1=AMP_OPPCHN;
    else *adac_1=0;
   } else *adac_1=0;
  }


  // PROBE (IDENTICAL TO ADAPTER #1 IN TERMS OF DURATIONS) -- LAT.ITD

  if (counter0==para_itd_pip_rand_stim_instant+para_itd_pip_rand_adaprobe_tot_dur \
	                                      +para_itd_pip_rand_ad2_hi_dur \
					      +para_itd_pip_rand_adaprobe_lo_dur) {
   /* Trigger #2 (Probe instant) */
   trigger_set(SEC_PIP_PROBE);
  }

  // LEAD
  if (counter0 >= para_itd_pip_rand_stim_instant_minus+para_itd_pip_rand_adaprobe_tot_dur \
		                                      +para_itd_pip_rand_ad2_hi_dur \
					              +para_itd_pip_rand_adaprobe_lo_dur && \
      counter0 <  para_itd_pip_rand_stim_instant_minus+para_itd_pip_rand_adaprobe_tot_dur \
                                                      +para_itd_pip_rand_ad2_hi_dur \
					              +para_itd_pip_rand_adaprobe_lo_dur\
 					              +para_itd_pip_rand_adaprobe_tot_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-(para_itd_pip_rand_stim_instant_minus+
		                                 +para_itd_pip_rand_adaprobe_tot_dur
						 +para_itd_pip_rand_ad2_hi_dur
						 +para_itd_pip_rand_adaprobe_lo_dur);
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_adaprobe_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_2=AMP_OPPCHN;
    else *adac_2=0;
   } else *adac_2=0;
  }
  // LAG
  if (counter0 >= para_itd_pip_rand_stim_instant_plus+para_itd_pip_rand_adaprobe_tot_dur \
		                                     +para_itd_pip_rand_ad2_hi_dur \
					             +para_itd_pip_rand_adaprobe_lo_dur && \
      counter0 <  para_itd_pip_rand_stim_instant_plus+para_itd_pip_rand_adaprobe_tot_dur \
                                                     +para_itd_pip_rand_ad2_hi_dur \
					             +para_itd_pip_rand_adaprobe_lo_dur\
 					             +para_itd_pip_rand_adaprobe_tot_dur) {
   para_itd_pip_rand_stim_local_offset=counter0-(para_itd_pip_rand_stim_instant_plus+
		                                 +para_itd_pip_rand_adaprobe_tot_dur
						 +para_itd_pip_rand_ad2_hi_dur
						 +para_itd_pip_rand_adaprobe_lo_dur);
   if (para_itd_pip_rand_stim_local_offset < para_itd_pip_rand_adaprobe_hi_dur) {
    if (para_itd_pip_rand_stim_local_offset%para_itd_pip_rand_click_period < para_itd_pip_rand_hi_period)
     *adac_3=AMP_OPPCHN;
    else *adac_3=0;
   } else *adac_3=0;
  }

 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_itd_pip_rand_soa_intertrial;
}

