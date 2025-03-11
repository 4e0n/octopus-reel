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

/* PARADIGM 'Temporal Offset for Minimum Probe Response'
    counter0: Counter for single trial
    -- squareburst of 400ms stim duration */

#define SEC_TOMPRES_L100	1	// TOMPRES Left 100ms
#define SEC_TOMPRES_L200	2	// TOMPRES Left 200ms
#define SEC_TOMPRES_L300	3	// TOMPRES Left 300ms
#define SEC_TOMPRES_L400	4	// TOMPRES Left 400ms
#define SEC_TOMPRES_L500	5	// TOMPRES Left 500ms
#define SEC_TOMPRES_L600	6	// TOMPRES Left 600ms
#define SEC_TOMPRES_L800	7	// TOMPRES Left 800ms
#define SEC_TOMPRES_L1400	8	// TOMPRES Left 1400ms
#define SEC_TOMPRES_R100	9	// TOMPRES Right 100ms
#define SEC_TOMPRES_R200	10	// TOMPRES Right 200ms
#define SEC_TOMPRES_R300	11	// TOMPRES Right 300ms
#define SEC_TOMPRES_R400	12	// TOMPRES Right 400ms
#define SEC_TOMPRES_R500	13	// TOMPRES Right 500ms
#define SEC_TOMPRES_R600	14	// TOMPRES Right 600ms
#define SEC_TOMPRES_R800	15	// TOMPRES Right 800ms
#define SEC_TOMPRES_R1400	16	// TOMPRES Right 1400ms
#define SEC_TOMPRES_L100X	17	// TOMPRES Left 100ms - Translateral
#define SEC_TOMPRES_L200X	18	// TOMPRES Left 200ms - T
#define SEC_TOMPRES_L300X	19	// TOMPRES Left 300ms - T
#define SEC_TOMPRES_L400X	20	// TOMPRES Left 400ms - T
#define SEC_TOMPRES_L500X	21	// TOMPRES Left 500ms - T
#define SEC_TOMPRES_L600X	22	// TOMPRES Left 600ms - T
#define SEC_TOMPRES_L800X	23	// TOMPRES Left 800ms - T
#define SEC_TOMPRES_L1400X	24	// TOMPRES Left 1400ms - T
#define SEC_TOMPRES_R100X	25	// TOMPRES Right 100ms - T
#define SEC_TOMPRES_R200X	26	// TOMPRES Right 200ms - T
#define SEC_TOMPRES_R300X	27	// TOMPRES Right 300ms - T
#define SEC_TOMPRES_R400X	28	// TOMPRES Right 400ms - T
#define SEC_TOMPRES_R500X	29	// TOMPRES Right 500ms - T
#define SEC_TOMPRES_R600X	30	// TOMPRES Right 600ms - T
#define SEC_TOMPRES_R800X	31	// TOMPRES Right 800ms - T
#define SEC_TOMPRES_R1400X	32	// TOMPRES Right 1400ms - T

static int para_itd_tompres_experiment_loop=1;

static int para_itd_tompres_trigger,

	   para_itd_tompres_soa_total,
	   //para_itd_tompres_soa,para_itd_tompres_soa_rand,
	   //para_itd_tompres_soa_randmax,
	   para_itd_tompres_isi,

	   /* Stim atomic properties */
	   para_itd_tompres_click_period,para_itd_tompres_no_periods,
	   para_itd_tompres_hi_period,
	   para_itd_tompres_stim_period,
	   para_itd_tompres_stim_local_offset,

	   para_itd_tompres_delta,

	   para_itd_tompres_adapter_region_lead,
	   para_itd_tompres_probe_region_lead,
	   para_itd_tompres_adapter_region_lag,
	   para_itd_tompres_probe_region_lag,
	   para_itd_tompres_stim_instant,

	   para_itd_tompres_laterality,
	   para_itd_tompres_ap_offset,
	   para_itd_tompres_ap_offset100,
	   para_itd_tompres_ap_offset200,
	   para_itd_tompres_ap_offset300,
	   para_itd_tompres_ap_offset400,
	   para_itd_tompres_ap_offset500,
	   para_itd_tompres_ap_offset600,
	   para_itd_tompres_ap_offset800,
	   para_itd_tompres_ap_offset1400,

	   para_itd_tompres_stim_instant_minus,para_itd_tompres_stim_instant_plus;

static void para_itd_tompres_init(void) {
 current_pattern_offset=0;

 //para_itd_tompres_soa=(5.00001)*AUDIO_RATE; /* 2.4 seconds */
 //para_itd_tompres_soa_randmax=(0.50001)*AUDIO_RATE; /* 0.5 seconds */
 //get_random_bytes(&para_itd_tompres_soa_rand,sizeof(int));
 //para_itd_tompres_soa_rand=0; //%=para_itd_tompres_soa_randmax;
 //para_itd_tompres_soa_total=para_itd_tompres_soa+para_itd_tompres_soa_rand;
 //rt_printk("%d %d %d\n",para_itd_tompres_soa,para_itd_tompres_soa_rand,
 //		 	para_itd_tompres_soa_total);
 para_itd_tompres_isi=(2.50001)*AUDIO_RATE; /* 2.5 seconds */

 para_itd_tompres_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps */
 para_itd_tompres_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 700 steps */
 para_itd_tompres_no_periods=5;

 para_itd_tompres_delta=(0.00061)*AUDIO_RATE; /*  L-R delta2: 30 steps */

 para_itd_tompres_stim_period=para_itd_tompres_click_period*para_itd_tompres_no_periods;

 para_itd_tompres_ap_offset100=(0.100001)*AUDIO_RATE; /* 100ms */
 para_itd_tompres_ap_offset200=(0.200001)*AUDIO_RATE; /* 200ms */
 para_itd_tompres_ap_offset300=(0.300001)*AUDIO_RATE; /* 300ms */
 para_itd_tompres_ap_offset400=(0.400001)*AUDIO_RATE; /* 400ms */
 para_itd_tompres_ap_offset500=(0.500001)*AUDIO_RATE; /* 500ms */
 para_itd_tompres_ap_offset600=(0.600001)*AUDIO_RATE; /* 600ms */
 para_itd_tompres_ap_offset800=(0.800001)*AUDIO_RATE; /* 800ms */
 para_itd_tompres_ap_offset1400=(1.400001)*AUDIO_RATE; /* 1400ms */

 //para_itd_tompres_stim_instant=para_itd_tompres_click_period/2;
 //para_itd_tompres_stim_instant=para_itd_tompres_soa_total/2;
 para_itd_tompres_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_itd_tompres_stim_instant_minus=para_itd_tompres_stim_instant-para_itd_tompres_delta/2;
 para_itd_tompres_stim_instant_plus=para_itd_tompres_stim_instant+para_itd_tompres_delta/2;

 rt_printk("%d %d %d %d %d %d %d\n", //para_itd_tompres_soa_total,
		         para_itd_tompres_isi,
		 	 para_itd_tompres_hi_period,
		 	 para_itd_tompres_click_period,
		 	 para_itd_tompres_delta,
		 	 para_itd_tompres_stim_instant,
		 	 para_itd_tompres_stim_instant_minus,
		 	 para_itd_tompres_stim_instant_plus);

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_itd_tompres_start(void) {
 lights_dimm();
 counter0=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_itd_tompres_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_itd_tompres_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_itd_tompres_resume(void) {
 lights_dimm();
 counter0=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_itd_tompres(void) {
 //int dummy_counter=0;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */

  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_itd_tompres_pause(); /* legitimate pause */
	     break;

   case 'A': /* Left - Adapter-Probe Temporal Offset -> 100ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L100;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset100;
	     para_itd_tompres_laterality=0;
             break;
   case 'B': /* Left - Adapter-Probe Temporal Offset -> 200ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L200;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset200;
	     para_itd_tompres_laterality=0;
             break;
   case 'C': /* Left - Adapter-Probe Temporal Offset -> 300ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L300;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset300;
	     para_itd_tompres_laterality=0;
             break;
   case 'D': /* Left - Adapter-Probe Temporal Offset -> 400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L400;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset400;
	     para_itd_tompres_laterality=0;
             break;
   case 'E': /* Left - Adapter-Probe Temporal Offset -> 500ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L500;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset500;
	     para_itd_tompres_laterality=0;
             break;
   case 'F': /* Left - Adapter-Probe Temporal Offset -> 600ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L600;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset600;
	     para_itd_tompres_laterality=0;
             break;
   case 'G': /* Left - Adapter-Probe Temporal Offset -> 800ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L800;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset800;
	     para_itd_tompres_laterality=0;
             break;
   case 'H': /* Left - Adapter-Probe Temporal Offset -> 1400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L1400;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset1400;
	     para_itd_tompres_laterality=0;
             break;

   case 'I': /* Right - Adapter-Probe Temporal Offset -> 100ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R100;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset100;
	     para_itd_tompres_laterality=1;
             break;
   case 'J': /* Right - Adapter-Probe Temporal Offset -> 200ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R200;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset200;
	     para_itd_tompres_laterality=1;
             break;
   case 'K': /* Right - Adapter-Probe Temporal Offset -> 300ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R300;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset300;
	     para_itd_tompres_laterality=1;
             break;
   case 'L': /* Right - Adapter-Probe Temporal Offset -> 400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R400;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset400;
	     para_itd_tompres_laterality=1;
             break;
   case 'M': /* Right - Adapter-Probe Temporal Offset -> 500ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R500;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset500;
	     para_itd_tompres_laterality=1;
             break;
   case 'N': /* Right - Adapter-Probe Temporal Offset -> 600ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R600;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset600;
	     para_itd_tompres_laterality=1;
             break;
   case 'O': /* Right - Adapter-Probe Temporal Offset -> 800ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R800;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset800;
	     para_itd_tompres_laterality=1;
             break;
   case 'P': /* Right - Adapter-Probe Temporal Offset -> 1400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R1400;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset1400;
	     para_itd_tompres_laterality=1;
	     break;

   case 'Q': /* Left - Adapter-Probe Temporal Offset -> 100ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L100X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset100;
	     para_itd_tompres_laterality=2;
             break;
   case 'R': /* Left - Adapter-Probe Temporal Offset -> 200ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L200X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset200;
	     para_itd_tompres_laterality=2;
             break;
   case 'S': /* Left - Adapter-Probe Temporal Offset -> 300ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L300X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset300;
	     para_itd_tompres_laterality=2;
             break;
   case 'T': /* Left - Adapter-Probe Temporal Offset -> 400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L400X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset400;
	     para_itd_tompres_laterality=2;
             break;
   case 'U': /* Left - Adapter-Probe Temporal Offset -> 500ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L500X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset500;
	     para_itd_tompres_laterality=2;
             break;
   case 'V': /* Left - Adapter-Probe Temporal Offset -> 600ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L600X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset600;
	     para_itd_tompres_laterality=2;
             break;
   case 'W': /* Left - Adapter-Probe Temporal Offset -> 800ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L800X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset800;
	     para_itd_tompres_laterality=2;
             break;
   case 'X': /* Left - Adapter-Probe Temporal Offset -> 1400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_L1400X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset1400;
	     para_itd_tompres_laterality=2;
             break;

   case 'Y': /* Right - Adapter-Probe Temporal Offset -> 100ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R100X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset100;
	     para_itd_tompres_laterality=3;
             break;
   case 'Z': /* Right - Adapter-Probe Temporal Offset -> 200ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R200X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset200;
	     para_itd_tompres_laterality=3;
             break;
   case 'a': /* Right - Adapter-Probe Temporal Offset -> 300ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R300X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset300;
	     para_itd_tompres_laterality=3;
             break;
   case 'b': /* Right - Adapter-Probe Temporal Offset -> 400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R400X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset400;
	     para_itd_tompres_laterality=3;
             break;
   case 'c': /* Right - Adapter-Probe Temporal Offset -> 500ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R500X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset500;
	     para_itd_tompres_laterality=3;
             break;
   case 'd': /* Right - Adapter-Probe Temporal Offset -> 600ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R600X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset600;
	     para_itd_tompres_laterality=3;
             break;
   case 'e': /* Right - Adapter-Probe Temporal Offset -> 800ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R800X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset800;
	     para_itd_tompres_laterality=3;
             break;
   case 'f': /* Right - Adapter-Probe Temporal Offset -> 1400ms */
	     para_itd_tompres_trigger=SEC_TOMPRES_R1400X;
             para_itd_tompres_ap_offset=para_itd_tompres_ap_offset1400;
	     para_itd_tompres_laterality=3;
   default:  break;
  }

  para_itd_tompres_soa_total=para_itd_tompres_ap_offset+para_itd_tompres_isi;

  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_itd_tompres_experiment_loop==0) para_itd_tompres_stop();
   //else para_itd_tompres_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_itd_tompres_stim_instant) && trigger_active &&
      (current_pattern_offset>0) )
  trigger_set(para_itd_tompres_trigger);

 /* Lead & Lag intervals */
 para_itd_tompres_adapter_region_lead=(
  (counter0 >= para_itd_tompres_stim_instant_minus) && \
  (counter0 <  para_itd_tompres_stim_instant_minus \
				            +para_itd_tompres_stim_period)) ? 1 : 0;
 para_itd_tompres_adapter_region_lag=(
  (counter0 >= para_itd_tompres_stim_instant_plus) && \
  (counter0 <  para_itd_tompres_stim_instant_plus \
				            +para_itd_tompres_stim_period)) ? 1 : 0;

 para_itd_tompres_probe_region_lead=(
  (counter0 >= para_itd_tompres_stim_instant_minus+para_itd_tompres_ap_offset) && \
  (counter0 <  para_itd_tompres_stim_instant_minus+para_itd_tompres_ap_offset \
                                            +para_itd_tompres_stim_period)) ? 1 : 0;
 para_itd_tompres_probe_region_lag=(
  (counter0 >= para_itd_tompres_stim_instant_plus+para_itd_tompres_ap_offset) && \
  (counter0 <  para_itd_tompres_stim_instant_plus+para_itd_tompres_ap_offset \
                                            +para_itd_tompres_stim_period)) ? 1 : 0;

 /* --------------------------------- */

 dac_0=dac_1=DACZERO;

 switch (para_itd_tompres_laterality) {
  case 0: /* RIGHT IPSILATERAL */
   if (para_itd_tompres_adapter_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_probe_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_adapter_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_probe_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   break;

  case 1: /* LEFT IPSILATERAL */
   if (para_itd_tompres_adapter_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_probe_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_adapter_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_probe_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   break;

  case 2: /* RIGHT TRANSLATERAL */
   if (para_itd_tompres_adapter_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_probe_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_adapter_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_probe_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   break;

  case 3: /* LEFT TRANSLATERAL */
   if (para_itd_tompres_adapter_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
   if (para_itd_tompres_probe_region_lead) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_minus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_adapter_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=DACZERO;
   }
   if (para_itd_tompres_probe_region_lag) {
    para_itd_tompres_stim_local_offset=counter0-para_itd_tompres_stim_instant_plus \
                                               -para_itd_tompres_ap_offset;
    if (para_itd_tompres_stim_local_offset%para_itd_tompres_click_period < \
                                                    para_itd_tompres_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=DACZERO;
   }
  default: break;
 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_itd_tompres_soa_total;
 //if (counter0==para_itd_tompres_soa_total) {
 //if (counter0==para_itd_tompres_isi) {
  //get_random_bytes(&para_itd_tompres_soa_rand,sizeof(int));
  //para_itd_tompres_soa_rand=0; //%=para_itd_tompres_soa_randmax;
  //para_itd_tompres_soa_total=para_itd_tompres_soa+para_itd_tompres_soa_rand;
  //rt_printk("%d %d %d\n",para_itd_tompres_soa,para_itd_tompres_soa_rand,
  // 		 	para_itd_tompres_soa_total);
  //counter0=0;
 //}
}
