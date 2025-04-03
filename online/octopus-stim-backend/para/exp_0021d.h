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

/* PARADIGM 'Adapter-Probe' matched vs. unmatched
 *          'Ipsilateral vs. Contralateral' Clicktrains

    counter0: Counter for single trial
    -- squareburst of 400ms stim duration

    -- multiple adapter support added. */

#define PARA_0021D_CONT_ADAPT

#define PARA_0021D_L600_L600	1	// PARA0021 L600->L600 (Adapter->Probe)
#define PARA_0021D_L600_R600	2	// PARA0021 L600->R600 (Adapter->Probe)
#define PARA_0021D_R600_R600	3	// PARA0021 R600->R600 (Adapter->Probe)
#define PARA_0021D_R600_L600	4	// PARA0021 R600->L600 (Adapter->Probe)

#define PARA_0021D_L400_L400	5	// PARA0021 L400->L400 (Adapter->Probe)
#define PARA_0021D_L400_R400	6	// PARA0021 L400->R400 (Adapter->Probe)
#define PARA_0021D_R400_R400	7	// PARA0021 R400->R400 (Adapter->Probe)
#define PARA_0021D_R400_L400	8	// PARA0021 R400->L400 (Adapter->Probe)

#define PARA_0021D_L200_L200	9	// PARA0021 L200->L200 (Adapter->Probe)
#define PARA_0021D_L200_R200	10	// PARA0021 L200->R200 (Adapter->Probe)
#define PARA_0021D_R200_R200	11	// PARA0021 R200->R200 (Adapter->Probe)
#define PARA_0021D_R200_L200	12	// PARA0021 R200->L200 (Adapter->Probe)

#define PARA_0021D_C_C		13	// PARA0021 C->C (Adapter->Probe)

static int para_0021d_experiment_loop=1;

static int para_0021d_trigger,para_0021d_soa,
	   para_0021d_iai,         /* Inter-adapter-interval */

	   /* Stim atomic properties */
	   para_0021d_click_period,para_0021d_no_periods_adapter,para_0021d_no_periods_probe,
	   para_0021d_hi_period,
	   para_0021d_adapter_burst_start,
	   para_0021d_adapter_total_dur_base, /* 750ms */
           para_0021d_adapter_total_dur_rand,
           para_0021d_adapter_total_dur_randmax, /* 100ms in 10 steps of 10ms */
	   para_0021d_adapter_period0,para_0021d_adapter_period1,para_0021d_probe_period,
	   para_0021d_stim_local_offset,

	   //para_0021d_lr_delta,
	   para_0021d_lr_delta200,para_0021d_lr_delta400,para_0021d_lr_delta600,

	   para_0021d_adapter_region_center,
	   para_0021d_adapter_region_lag,para_0021d_adapter_region_lead,
	   para_0021d_probe_region_center,
	   para_0021d_probe_region_lag,para_0021d_probe_region_lead,
	   para_0021d_stim_instant,

	   para_0021d_adapter_type,para_0021d_probe_type,
	   para_0021d_ap_offset,

	   para_0021d_stim_instant_center,
	   para_0021d_stim_instant_minus,para_0021d_stim_instant_minus200,
	   para_0021d_stim_instant_minus400,para_0021d_stim_instant_minus600,
	   para_0021d_stim_instant_plus,para_0021d_stim_instant_plus200,
	   para_0021d_stim_instant_plus400,para_0021d_stim_instant_plus600;

static void para_0021d_init(void) {
 current_pattern_offset=0;

 para_0021d_soa=(4.00001)*AUDIO_RATE; /* 4.0 seconds */
 para_0021d_ap_offset=(0.800001)*AUDIO_RATE; /* DISCRETE */
#ifdef PARA_0021D_CONT_ADAPT
 para_0021d_ap_offset=(1.000001)*AUDIO_RATE; /* CONTINUOUS */
#endif
 para_0021d_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps */
 para_0021d_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 700 steps */
 /* --------- */
 para_0021d_no_periods_adapter=5*12; /* 60*10 ms */
 para_0021d_no_periods_probe=5; /* 5*10 ms */

 para_0021d_adapter_burst_start=(0.200001)*AUDIO_RATE; /* 200ms - Start of burst in CONTINUOUS adapter case */
 para_0021d_adapter_total_dur_base=para_0021d_ap_offset; /* 800ms - DISCRETE (e.g. quadruple) adapter case */
#ifdef PARA_0021D_CONT_ADAPT
 para_0021d_adapter_total_dur_base=(0.750001)*AUDIO_RATE; /* 750ms<->850ms - CONTINUOUS adapter case */
 para_0021d_adapter_total_dur_randmax=((0.100001)*AUDIO_RATE)/para_0021d_click_period; /* 0ms->100ms (rand) */
#endif
 para_0021d_probe_period=para_0021d_click_period*para_0021d_no_periods_probe; /* 50ms */
 para_0021d_adapter_period0=para_0021d_probe_period; /*  50ms - for DISCRETE (e.g. quadruple) adapter setting */
 para_0021d_adapter_period1=para_0021d_probe_period;
#ifdef PARA_0021D_CONT_ADAPT
 para_0021d_adapter_period1*=4; /* 200ms - for CONTINUOUS adapter setting --- */
#endif

 para_0021d_iai=(0.200001)*AUDIO_RATE; /* Inter-adapter Interval: 1s -- never to happen for long single adapter case */;
 /* --------- */

 para_0021d_lr_delta200=(0.00021)*AUDIO_RATE; /*  L-R delta: 200us - 10 steps */
 para_0021d_lr_delta400=(0.00041)*AUDIO_RATE; /*  L-R delta: 400us - 20 steps */
 para_0021d_lr_delta600=(0.00061)*AUDIO_RATE; /*  L-R delta: 600us - 30 steps */

 para_0021d_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_0021d_stim_instant_center=para_0021d_stim_instant;
 para_0021d_stim_instant_minus200=para_0021d_stim_instant-para_0021d_lr_delta200/2;
 para_0021d_stim_instant_plus200=para_0021d_stim_instant+para_0021d_lr_delta200/2;
 para_0021d_stim_instant_minus400=para_0021d_stim_instant-para_0021d_lr_delta400/2;
 para_0021d_stim_instant_plus400=para_0021d_stim_instant+para_0021d_lr_delta400/2;
 para_0021d_stim_instant_minus600=para_0021d_stim_instant-para_0021d_lr_delta600/2;
 para_0021d_stim_instant_plus600=para_0021d_stim_instant+para_0021d_lr_delta600/2;

 rt_printk("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                         para_0021d_adapter_total_dur_base,
                         para_0021d_adapter_total_dur_randmax,
		 	 para_0021d_hi_period,
		 	 para_0021d_click_period,
		 	 para_0021d_lr_delta200,
		 	 para_0021d_lr_delta400,
		 	 para_0021d_lr_delta600,
		 	 para_0021d_stim_instant,
		 	 para_0021d_stim_instant_center,
		 	 para_0021d_stim_instant_minus200,
		 	 para_0021d_stim_instant_minus400,
		 	 para_0021d_stim_instant_minus600,
		 	 para_0021d_stim_instant_plus200,
		 	 para_0021d_stim_instant_plus400,
		 	 para_0021d_stim_instant_plus600);

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_0021d_start(void) {
 lights_dimm();
 counter0=0; counter1=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_0021d_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_0021d_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_0021d_resume(void) {
 lights_dimm();
 counter0=0; counter1=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_0021d(void) {
 //int dummy_counter=0;

 if (counter0==0) {
refetch:
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_0021d_pause(); /* legitimate pause */
	     break;
   case 'A': /* Click Train L600 -> L600 */
	     para_0021d_trigger=PARA_0021D_L600_L600;
	     para_0021d_adapter_type=1; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus600;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus600;
             break;
   case 'B': /* Click Train L600 -> R600 */
	     para_0021d_trigger=PARA_0021D_L600_R600;
	     para_0021d_adapter_type=1; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus600;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus600;
             break;
   case 'C': /* Click Train R600 -> R600 */
	     para_0021d_trigger=PARA_0021D_R600_R600;
	     para_0021d_adapter_type=2; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus600;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus600;
             break;
   case 'D': /* Click Train R600 -> L600 */
	     para_0021d_trigger=PARA_0021D_R600_L600;
	     para_0021d_adapter_type=2; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus600;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus600;
             break;
   case 'E': /* Click Train L400 -> L400 */
	     para_0021d_trigger=PARA_0021D_L400_L400;
	     para_0021d_adapter_type=1; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus400;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus400;
             break;
   case 'F': /* Click Train L400 -> R400 */
	     para_0021d_trigger=PARA_0021D_L400_R400;
	     para_0021d_adapter_type=1; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus400;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus400;
             break;
   case 'G': /* Click Train R400 -> R400 */
	     para_0021d_trigger=PARA_0021D_R400_R400;
	     para_0021d_adapter_type=2; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus400;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus400;
             break;
   case 'H': /* Click Train R400 -> L400 */
	     para_0021d_trigger=PARA_0021D_R400_L400;
	     para_0021d_adapter_type=2; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus400;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus400;
             break;
   case 'I': /* Click Train L200 -> L200 */
	     para_0021d_trigger=PARA_0021D_L200_L200;
	     para_0021d_adapter_type=1; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus200;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus200;
             break;
   case 'J': /* Click Train L200 -> R200 */
	     para_0021d_trigger=PARA_0021D_L200_R200;
	     para_0021d_adapter_type=1; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus200;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus200;
             break;
   case 'K': /* Click Train R200 -> R200 */
	     para_0021d_trigger=PARA_0021D_R200_R200;
	     para_0021d_adapter_type=2; para_0021d_probe_type=2;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus200;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus200;
             break;
   case 'L': /* Click Train R200 -> L200 */
	     para_0021d_trigger=PARA_0021D_R200_L200;
	     para_0021d_adapter_type=2; para_0021d_probe_type=1;
	     para_0021d_stim_instant_minus=para_0021d_stim_instant_minus200;
	     para_0021d_stim_instant_plus=para_0021d_stim_instant_plus200;
             break;
   case 'M': /* Click Train C -> C -1 */
	     para_0021d_trigger=PARA_0021D_C_C;
	     para_0021d_adapter_type=0; para_0021d_probe_type=0;
             break;
   case 'N': /* Click Train C -> C -2 */
	     para_0021d_trigger=PARA_0021D_C_C;
	     para_0021d_adapter_type=0; para_0021d_probe_type=0;
	     break;
   case '.': /* Randomize AP-ISI */
             // A random amount (0ms-100ms) will be added here
             get_random_bytes(&para_0021d_adapter_total_dur_rand,sizeof(int));
             para_0021d_adapter_total_dur_rand%=para_0021d_adapter_total_dur_randmax;
             para_0021d_adapter_total_dur_rand=abs(para_0021d_adapter_total_dur_rand);
             rt_printk("Random x 10ms: %d\n",para_0021d_adapter_total_dur_rand); // log for hist plotting
             para_0021d_adapter_total_dur_rand*=para_0021d_click_period; /* {0:10:90}ms */
             current_pattern_offset++;
             if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
              if (para_0021d_experiment_loop==0) para_0021d_stop();
              //else para_0021d_pause();
              current_pattern_offset=0;
             }
	     goto refetch;
   default:  break;
  }

  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_0021d_experiment_loop==0) para_0021d_stop();
   //else para_0021d_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_0021d_stim_instant+para_0021d_ap_offset) && trigger_active &&
      (current_pattern_offset>0) )
  trigger_set(para_0021d_trigger);
 /* ------------------------------------------------------------------- */

 /* Adapter region center */
 if (counter0 >= para_0021d_stim_instant_center && \
     counter0 <  para_0021d_stim_instant_center+para_0021d_adapter_total_dur_base \
                                               +para_0021d_adapter_total_dur_rand) {
  if (counter0 < para_0021d_stim_instant_center+para_0021d_adapter_burst_start) {
   if ((counter0-para_0021d_stim_instant_center)%para_0021d_iai \
 		                         < para_0021d_adapter_period0) {
    para_0021d_adapter_region_center=1;
   } else {
    para_0021d_adapter_region_center=0;
   }
  } else {
   if ((counter0-para_0021d_stim_instant_center)%para_0021d_iai \
 		                         < para_0021d_adapter_period1) {
    para_0021d_adapter_region_center=1;
   } else {
    para_0021d_adapter_region_center=0;
   }
  }
 } else {
  para_0021d_adapter_region_center=0;
 }

 /* Adapter region (left) lead */
 if (counter0 >= para_0021d_stim_instant_minus && \
     counter0 <  para_0021d_stim_instant_minus+para_0021d_adapter_total_dur_base \
                                              +para_0021d_adapter_total_dur_rand) {
  if (counter0 < para_0021d_stim_instant_minus+para_0021d_adapter_burst_start) {
   if ((counter0-para_0021d_stim_instant_minus)%para_0021d_iai \
		                         < para_0021d_adapter_period0) {
    para_0021d_adapter_region_lead=1;
   } else {
    para_0021d_adapter_region_lead=0;
   }
  } else {
   if ((counter0-para_0021d_stim_instant_minus)%para_0021d_iai \
		                      < para_0021d_adapter_period1) {
    para_0021d_adapter_region_lead=1;
   } else {
    para_0021d_adapter_region_lead=0;
   }
  }
 } else {
  para_0021d_adapter_region_lead=0;
 }

 /* Adapter region (left) lag */
 if (counter0 >= para_0021d_stim_instant_plus && \
     counter0 <  para_0021d_stim_instant_plus+para_0021d_adapter_total_dur_base \
                                             +para_0021d_adapter_total_dur_rand) {
  if (counter0 < para_0021d_stim_instant_plus+para_0021d_adapter_burst_start) {
   if ((counter0-para_0021d_stim_instant_plus)%para_0021d_iai \
		                         < para_0021d_adapter_period0) {
    para_0021d_adapter_region_lag=1;
   } else {
    para_0021d_adapter_region_lag=0;
   }
  } else {
   if ((counter0-para_0021d_stim_instant_plus)%para_0021d_iai \
		                         < para_0021d_adapter_period1) {
    para_0021d_adapter_region_lag=1;
   } else {
    para_0021d_adapter_region_lag=0;
   }
  }
 } else {
  para_0021d_adapter_region_lag=0;
 }

 /* Probe region center */
 if (counter0 >= para_0021d_stim_instant_center+para_0021d_ap_offset && \
     counter0 <  para_0021d_stim_instant_center+para_0021d_ap_offset \
                                                 +para_0021d_probe_period) {
  para_0021d_probe_region_center=1;
 } else {
  para_0021d_probe_region_center=0;
 }

 /* Probe region (left) lead */
 if (counter0 >= para_0021d_stim_instant_minus+para_0021d_ap_offset && \
     counter0 <  para_0021d_stim_instant_minus+para_0021d_ap_offset \
                                                 +para_0021d_probe_period) {
  para_0021d_probe_region_lead=1;
 } else {
  para_0021d_probe_region_lead=0;
 }

 /* Probe region (left) lag */
 if (counter0 >= para_0021d_stim_instant_plus+para_0021d_ap_offset && \
     counter0 <  para_0021d_stim_instant_plus+para_0021d_ap_offset \
                                                 +para_0021d_probe_period) {
  para_0021d_probe_region_lag=1;
 } else {
  para_0021d_probe_region_lag=0;
 }

 /* ------ */

 dac_0=dac_1=0;

 switch (para_0021d_adapter_type) {
  case 0: /* Center */
   if (para_0021d_adapter_region_center) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant%para_0021d_probe_period;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=dac_1=AMP_OPPCHN;
    else dac_0=dac_1=0;
   }
   break;
  case 1: /* Left Lead */
   if (para_0021d_adapter_region_lead) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_minus;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_0021d_adapter_region_lag) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_plus;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;
  case 2: /* Right Lead*/
   if (para_0021d_adapter_region_lead) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_minus;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_0021d_adapter_region_lag) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_plus;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   break;
 }

 switch (para_0021d_probe_type) {
  case 0: /* Center */
   if (para_0021d_probe_region_center) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant \
                                               -para_0021d_ap_offset;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=dac_1=AMP_OPPCHN;
    else dac_0=dac_1=0;
   }
   break;
  case 1: /* Left Lead (L600,L400,L200) */
   if (para_0021d_probe_region_lead) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_minus \
                                               -para_0021d_ap_offset;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_0021d_probe_region_lag) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_plus \
                                               -para_0021d_ap_offset;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;
  case 2: /* Right Lead (R600,R400,R200) */
   if (para_0021d_probe_region_lead) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_minus \
                                               -para_0021d_ap_offset;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_0021d_probe_region_lag) {
    para_0021d_stim_local_offset=counter0-para_0021d_stim_instant_plus \
                                               -para_0021d_ap_offset;
    if (para_0021d_stim_local_offset%para_0021d_click_period < \
                                                    para_0021d_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
  default: break;
 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_0021d_soa;
}
