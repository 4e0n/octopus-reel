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

/* PARADIGM 'Leveled-ITD'
 *          'Ipsilateral vs. Contralateral' Clicktrains

    counter0: Counter for single trial
    -- squareburst of 400ms stim duration */

static int para_0021b_experiment_loop=1;

static int para_0021b_trigger,para_0021b_soa,

	   /* Stim atomic properties */
	   para_0021b_click_period,para_0021b_no_periods_stim,
	   para_0021b_hi_period,para_0021b_stim_period,
	   para_0021b_stim_local_offset,

	   para_0021b_lr,para_0021b_lr_delta,
	   para_0021b_lr_delta100,para_0021b_lr_delta200,
	   para_0021b_lr_delta350,para_0021b_lr_delta600,

	   para_0021b_stim_region_center,
	   para_0021b_stim_region_lag,para_0021b_stim_region_lead,

	   para_0021b_stim_instant,para_0021b_stim_instant_center,
	   para_0021b_stim_instant_minus,
	   para_0021b_stim_instant_minus100,para_0021b_stim_instant_minus200,
	   para_0021b_stim_instant_minus350,para_0021b_stim_instant_minus600,
	   para_0021b_stim_instant_plus,
	   para_0021b_stim_instant_plus100,para_0021b_stim_instant_plus200,
	   para_0021b_stim_instant_plus350,para_0021b_stim_instant_plus600;

static void para_0021b_init(void) {
 current_pattern_offset=0;
 /* --------- */
 para_0021b_soa=(4.00001)*AUDIO_RATE; /* 4.0 seconds */
 para_0021b_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps */
 para_0021b_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 700 steps */
 /* --------- */
 para_0021b_no_periods_stim=5; /* 5*10 ms */
 para_0021b_stim_period=para_0021b_click_period*para_0021b_no_periods_stim; /* 50ms */
 /* --------- */
 para_0021b_lr_delta100=(0.000101)*AUDIO_RATE; /*  L-R delta: 200us -  5 steps */
 para_0021b_lr_delta200=(0.000201)*AUDIO_RATE; /*  L-R delta: 200us - 10 steps */
 para_0021b_lr_delta350=(0.000361)*AUDIO_RATE; /*  L-R delta: 340us - 18 steps */
 para_0021b_lr_delta600=(0.000601)*AUDIO_RATE; /*  L-R delta: 600us - 30 steps */

 para_0021b_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_0021b_stim_instant_center=para_0021b_stim_instant;
 para_0021b_stim_instant_minus100=para_0021b_stim_instant-para_0021b_lr_delta100/2;
 para_0021b_stim_instant_minus200=para_0021b_stim_instant-para_0021b_lr_delta200/2;
 para_0021b_stim_instant_minus350=para_0021b_stim_instant-para_0021b_lr_delta350/2;
 para_0021b_stim_instant_minus600=para_0021b_stim_instant-para_0021b_lr_delta600/2;
 para_0021b_stim_instant_plus100=para_0021b_stim_instant+para_0021b_lr_delta100/2;
 para_0021b_stim_instant_plus200=para_0021b_stim_instant+para_0021b_lr_delta200/2;
 para_0021b_stim_instant_plus350=para_0021b_stim_instant+para_0021b_lr_delta350/2;
 para_0021b_stim_instant_plus600=para_0021b_stim_instant+para_0021b_lr_delta600/2;

 rt_printk("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		 	 para_0021b_hi_period,
		 	 para_0021b_click_period,
		 	 para_0021b_lr_delta100,
		 	 para_0021b_lr_delta200,
		 	 para_0021b_lr_delta350,
		 	 para_0021b_lr_delta600,
		 	 para_0021b_stim_instant,
		 	 para_0021b_stim_instant_center,
		 	 para_0021b_stim_instant_minus100,
		 	 para_0021b_stim_instant_minus200,
		 	 para_0021b_stim_instant_minus350,
		 	 para_0021b_stim_instant_minus600,
		 	 para_0021b_stim_instant_plus100,
		 	 para_0021b_stim_instant_plus200,
		 	 para_0021b_stim_instant_plus350,
		 	 para_0021b_stim_instant_plus600);

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_0021b_start(void) {
 lights_dimm();
 counter0=0; counter1=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_0021b_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_0021b_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_0021b_resume(void) {
 lights_dimm();
 counter0=0; counter1=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_0021b(void) {
 int dummy_counter=0;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */

  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_0021b_pause(); /* legitimate pause */
	     break;
   case 'A': /* Click Train L600 */
	     para_0021b_trigger=PARA_0021B_L600; para_0021b_lr=1;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus600;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus600;
             break;
   case 'B': /* Click Train L350 */
	     para_0021b_trigger=PARA_0021B_L350; para_0021b_lr=1;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus350;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus350;
             break;
   case 'C': /* Click Train L200 */
	     para_0021b_trigger=PARA_0021B_L200; para_0021b_lr=1;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus200;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus200;
             break;
   case 'D': /* Click Train L100 */
	     para_0021b_trigger=PARA_0021B_L100; para_0021b_lr=1;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus100;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus100;
             break;
   case 'E': /* Click Train Center */
	     para_0021b_trigger=PARA_0021B_C; para_0021b_lr=0;
             break;
   case 'F': /* Click Train R100 */
	     para_0021b_trigger=PARA_0021B_R100; para_0021b_lr=2;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus100;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus100;
             break;
   case 'G': /* Click Train R200 */
	     para_0021b_trigger=PARA_0021B_R200; para_0021b_lr=2;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus200;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus200;
             break;
   case 'H': /* Click Train R350 */
	     para_0021b_trigger=PARA_0021B_R350; para_0021b_lr=2;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus350;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus350;
             break;
   case 'I': /* Click Train R600 */
	     para_0021b_trigger=PARA_0021B_R600; para_0021b_lr=2;
	     para_0021b_stim_instant_minus=para_0021b_stim_instant_minus600;
	     para_0021b_stim_instant_plus=para_0021b_stim_instant_plus600;
   default:  break;
  }

  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_0021b_experiment_loop==0) para_0021b_stop();
   //else para_0021b_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_0021b_stim_instant) && trigger_active &&
      (current_pattern_offset>0) )
  trigger_set(para_0021b_trigger);

 /* Probe region center */
 if (counter0 >= para_0021b_stim_instant_center && \
     counter0 <  para_0021b_stim_instant_center+para_0021b_stim_period) {
  para_0021b_stim_region_center=1;
 } else {
  para_0021b_stim_region_center=0;
 }

 /* Probe region lag */
 if (counter0 >= para_0021b_stim_instant_plus && \
     counter0 <  para_0021b_stim_instant_plus+para_0021b_stim_period) {
  para_0021b_stim_region_lag=1;
 } else {
  para_0021b_stim_region_lag=0;
 }

 /* Probe region lead */
 if (counter0 >= para_0021b_stim_instant_minus && \
     counter0 <  para_0021b_stim_instant_minus+para_0021b_stim_period) {
  para_0021b_stim_region_lead=1;
 } else {
  para_0021b_stim_region_lead=0;
 }

 /* ------ */

 dac_0=dac_1=0;

 switch (para_0021b_lr) {
  case 0: /* Center */
   if (para_0021b_stim_region_center) {
    para_0021b_stim_local_offset=counter0-para_0021b_stim_instant;
    if (para_0021b_stim_local_offset%para_0021b_click_period < para_0021b_hi_period)
     dac_0=dac_1=AMP_OPPCHN;
    else dac_0=dac_1=0;
   }
   break;
  case 1: /* L600,L350,L200,L100 */
   if (para_0021b_stim_region_lead) {
    para_0021b_stim_local_offset=counter0-para_0021b_stim_instant_minus;
    if (para_0021b_stim_local_offset%para_0021b_click_period < para_0021b_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_0021b_stim_region_lag) {
    para_0021b_stim_local_offset=counter0-para_0021b_stim_instant_plus;
    if (para_0021b_stim_local_offset%para_0021b_click_period < para_0021b_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;
  case 2: /* R100,R200,R350,R600 */
   if (para_0021b_stim_region_lead) {
    para_0021b_stim_local_offset=counter0-para_0021b_stim_instant_minus;
    if (para_0021b_stim_local_offset%para_0021b_click_period < para_0021b_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_0021b_stim_region_lag) {
    para_0021b_stim_local_offset=counter0-para_0021b_stim_instant_plus;
    if (para_0021b_stim_local_offset%para_0021b_click_period < para_0021b_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
  default: break;
 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_0021b_soa;
}
