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

/* PARADIGM 'Adapter-Probe' matched vs. unmatched
 *          'Ipsilateral vs. Contralateral'
 *          'Tones vs click trains'

 * Tones are 500Hz and 1150Hz

    counter0: Counter for single trial
    -- squareburst of 400ms stim duration

    -- multiple adapter support added. */

static int para_itd_pip_ctrain_experiment_loop=1;

static int para_itd_pip_ctrain_trigger,

	   para_itd_pip_ctrain_soa,
	   para_itd_pip_ctrain_isi,

	   para_itd_pip_ctrain_no_adapters, /* How many adapters before probe? */
	   para_itd_pip_ctrain_iai,         /* Inter-adapter-interval */

	   /* Stim atomic properties */
	   para_itd_pip_ctrain_click_period,para_itd_pip_ctrain_no_periods,
	   para_itd_pip_ctrain_hi_period,
	   para_itd_pip_ctrain_stim_period,
	   para_itd_pip_ctrain_stim_local_offset,

	   para_itd_pip_ctrain_lr_delta,

           para_itd_pip_ctrain_adapter_region_norm1,
           para_itd_pip_ctrain_adapter_region_norm2,
           para_itd_pip_ctrain_adapter_region_norm3,
           para_itd_pip_ctrain_probe_region_norm1,
           para_itd_pip_ctrain_probe_region_norm2,
           para_itd_pip_ctrain_probe_region_norm3,

	   para_itd_pip_ctrain_adapter_region_lead,
	   para_itd_pip_ctrain_probe_region_lead,
	   para_itd_pip_ctrain_adapter_region_lag,
	   para_itd_pip_ctrain_probe_region_lag,
	   para_itd_pip_ctrain_stim_instant,

	   para_itd_pip_ctrain_laterality,
	   para_itd_pip_ctrain_ap_offset,
	   para_itd_pip_ctrain_istone,

	   para_itd_pip_ctrain_stim_instant_minus,
	   para_itd_pip_ctrain_stim_instant_plus;

static double para_itd_pip_ctrain_theta,
              para_itd_pip_ctrain_theta2,
              para_itd_pip_ctrain_delta_theta50, /* For modulation */
              para_itd_pip_ctrain_delta_theta500,
	      para_itd_pip_ctrain_delta_theta1150;

static void para_itd_pip_ctrain_init(void) {
 current_pattern_offset=0;

 para_itd_pip_ctrain_soa=(4.00001)*AUDIO_RATE; /* 4.0 seconds */
 para_itd_pip_ctrain_ap_offset=(0.800001)*AUDIO_RATE; /* 800ms */
 para_itd_pip_ctrain_isi=para_itd_pip_ctrain_soa-para_itd_pip_ctrain_ap_offset;

 para_itd_pip_ctrain_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps */
 para_itd_pip_ctrain_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 700 steps */
 para_itd_pip_ctrain_no_periods=5;

 para_itd_pip_ctrain_lr_delta=(0.00061)*AUDIO_RATE; /*  L-R delta2: 30 steps */

 para_itd_pip_ctrain_stim_period=para_itd_pip_ctrain_click_period*para_itd_pip_ctrain_no_periods;


 //para_itd_pip_ctrain_stim_instant=para_itd_pip_ctrain_click_period/2;
 //para_itd_pip_ctrain_stim_instant=para_itd_pip_ctrain_soa/2;
 para_itd_pip_ctrain_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_itd_pip_ctrain_stim_instant_minus=para_itd_pip_ctrain_stim_instant-para_itd_pip_ctrain_lr_delta/2;
 para_itd_pip_ctrain_stim_instant_plus=para_itd_pip_ctrain_stim_instant+para_itd_pip_ctrain_lr_delta/2;

 para_itd_pip_ctrain_delta_theta50=50.0/(double)(AUDIO_RATE); /* f=50Hz */
 para_itd_pip_ctrain_delta_theta500=500.0/(double)(AUDIO_RATE); /* f=500Hz */
 para_itd_pip_ctrain_delta_theta1150=1150.0/(double)(AUDIO_RATE); /* f=1150Hz */

 para_itd_pip_ctrain_no_adapters=3;
 para_itd_pip_ctrain_iai=(0.200001)*AUDIO_RATE; /* Inter-adapter Interval: 200ms */;

 //para_itd_pip_ctrain_adapter_region_norm1=0;
 //para_itd_pip_ctrain_adapter_region_norm2=0;
 //para_itd_pip_ctrain_adapter_region_norm3=0;
 //para_itd_pip_ctrain_probe_region_norm1=0;
 //para_itd_pip_ctrain_probe_region_norm2=0;
 //para_itd_pip_ctrain_probe_region_norm3=0;

 rt_printk("%d %d %d %d %d %d %d\n", //para_itd_pip_ctrain_soa,
		         para_itd_pip_ctrain_isi,
		 	 para_itd_pip_ctrain_hi_period,
		 	 para_itd_pip_ctrain_click_period,
		 	 para_itd_pip_ctrain_lr_delta,
		 	 para_itd_pip_ctrain_stim_instant,
		 	 para_itd_pip_ctrain_stim_instant_minus,
		 	 para_itd_pip_ctrain_stim_instant_plus);

 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_itd_pip_ctrain_start(void) {
 lights_dimm();
 counter0=0; counter1=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_itd_pip_ctrain_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_itd_pip_ctrain_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_itd_pip_ctrain_resume(void) {
 lights_dimm();
 counter0=0; counter1=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_itd_pip_ctrain(void) {
 int dummy_counter=0;

 if (counter0==0) {
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */

  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_itd_pip_ctrain_pause(); /* legitimate pause */
	     break;

   case 'A': /* Tone Pip 500Hz -> 500Hz */
	     para_itd_pip_ctrain_trigger=SEC_PIP_TONE_LL;
	     para_itd_pip_ctrain_istone=1; para_itd_pip_ctrain_laterality=0;
             break;
   case 'B': /* Tone Pip 1150Hz -> 1150Hz */
	     para_itd_pip_ctrain_trigger=SEC_PIP_TONE_HH;
	     para_itd_pip_ctrain_istone=1; para_itd_pip_ctrain_laterality=1;
             break;
   case 'C': /* Tone Pip 500Hz -> 1150Hz */
	     para_itd_pip_ctrain_trigger=SEC_PIP_TONE_LH;
	     para_itd_pip_ctrain_istone=1; para_itd_pip_ctrain_laterality=2;
             break;
   case 'D': /* Tone Pip 1150Hz -> 500Hz */
	     para_itd_pip_ctrain_trigger=SEC_PIP_TONE_HL;
	     para_itd_pip_ctrain_istone=1; para_itd_pip_ctrain_laterality=3;
             break;
   case 'E': /* Click Train Pip Left -> Left */
	     para_itd_pip_ctrain_trigger=SEC_PIP_CTRAIN_LL;
	     para_itd_pip_ctrain_istone=0; para_itd_pip_ctrain_laterality=4;
             break;
   case 'F': /* Click Train Pip Right -> Right */
	     para_itd_pip_ctrain_trigger=SEC_PIP_CTRAIN_RR;
	     para_itd_pip_ctrain_istone=0; para_itd_pip_ctrain_laterality=5;
             break;
   case 'G': /* Click Train Pip Left -> Right */
	     para_itd_pip_ctrain_trigger=SEC_PIP_CTRAIN_LR;
	     para_itd_pip_ctrain_istone=0; para_itd_pip_ctrain_laterality=6;
             break;
   case 'H': /* Click Train Pip Right -> Left */
	     para_itd_pip_ctrain_trigger=SEC_PIP_CTRAIN_RL;
	     para_itd_pip_ctrain_istone=0; para_itd_pip_ctrain_laterality=7;
   default:  break;
  }


  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_itd_pip_ctrain_experiment_loop==0) para_itd_pip_ctrain_stop();
   //else para_itd_pip_ctrain_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_itd_pip_ctrain_stim_instant) && trigger_active &&
      (current_pattern_offset>0) )
  trigger_set(para_itd_pip_ctrain_trigger);

 /* Identify current temporal region */

 if (para_itd_pip_ctrain_istone) {

  if (counter0 >= para_itd_pip_ctrain_stim_instant && \
      counter0 <  para_itd_pip_ctrain_stim_instant+para_itd_pip_ctrain_ap_offset) {
   if ((counter0-para_itd_pip_ctrain_stim_instant)/para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_no_adapters) {
    if ((counter0-para_itd_pip_ctrain_stim_instant)%para_itd_pip_ctrain_iai == 0) {
     para_itd_pip_ctrain_theta=para_itd_pip_ctrain_theta2=0.0; counter1=0;
    } 
    if ((counter0-para_itd_pip_ctrain_stim_instant)%para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_stim_period) {
     /* Intro and outro cosine ramps */
     if (counter1 == para_itd_pip_ctrain_stim_period*(0.8001))
      para_itd_pip_ctrain_theta2=0.0;
     /* Intro and outro cosine ramps */
     para_itd_pip_ctrain_adapter_region_norm1= \
      (counter1 < para_itd_pip_ctrain_stim_period*(0.2001)) ? 1 : 0;
     para_itd_pip_ctrain_adapter_region_norm2= \
      (counter1 >= para_itd_pip_ctrain_stim_period*(0.2001) && \
       counter1 <  para_itd_pip_ctrain_stim_period*(0.8001)) ? 1 : 0;
     para_itd_pip_ctrain_adapter_region_norm3= \
      (counter1 >= para_itd_pip_ctrain_stim_period*(0.8001) && \
       counter1 <  para_itd_pip_ctrain_stim_period) ? 1 : 0;
     counter1++;
    } else {
     para_itd_pip_ctrain_adapter_region_norm1=0;
     para_itd_pip_ctrain_adapter_region_norm2=0;
     para_itd_pip_ctrain_adapter_region_norm3=0;
    }
   }
  }
  /* Probe region */
  if (counter0 == para_itd_pip_ctrain_stim_instant+para_itd_pip_ctrain_ap_offset) {
   counter1=0; /* reset adapter# for next stim */
  }
  if (counter0 >= para_itd_pip_ctrain_stim_instant+para_itd_pip_ctrain_ap_offset && \
      counter0 <  para_itd_pip_ctrain_stim_instant+para_itd_pip_ctrain_ap_offset \
                                                  +para_itd_pip_ctrain_stim_period) {
   /* Intro and outro cosine ramps */
   para_itd_pip_ctrain_probe_region_norm1= \
    (counter1 < para_itd_pip_ctrain_stim_period*(0.2001)) ? 1 : 0;
   para_itd_pip_ctrain_probe_region_norm2= \
    (counter1 >= para_itd_pip_ctrain_stim_period*(0.2001) && \
     counter1 <  para_itd_pip_ctrain_stim_period*(0.8001)) ? 1 : 0;
   para_itd_pip_ctrain_probe_region_norm3= \
    (counter1 >= para_itd_pip_ctrain_stim_period*(0.8001) && \
     counter1 <  para_itd_pip_ctrain_stim_period) ? 1 : 0;
    counter1++;
  } else {
   para_itd_pip_ctrain_probe_region_norm1=0;
   para_itd_pip_ctrain_probe_region_norm2=0;
   para_itd_pip_ctrain_probe_region_norm3=0;
  }

 } else { /* is lateral click train */

  /* Adapter region lag */
  if (counter0 >= para_itd_pip_ctrain_stim_instant_plus && \
      counter0 <  para_itd_pip_ctrain_stim_instant_plus+para_itd_pip_ctrain_ap_offset) {
   if ((counter0-para_itd_pip_ctrain_stim_instant_plus)/para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_no_adapters) {
    if ((counter0-para_itd_pip_ctrain_stim_instant_plus)%para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_stim_period) {
     para_itd_pip_ctrain_adapter_region_lag=1;
    } else {
     para_itd_pip_ctrain_adapter_region_lag=0;
    }
   }
  }

  /* Probe region lag */
  if (counter0 >= para_itd_pip_ctrain_stim_instant_plus+para_itd_pip_ctrain_ap_offset && \
      counter0 <  para_itd_pip_ctrain_stim_instant_plus+para_itd_pip_ctrain_ap_offset \
                                                  +para_itd_pip_ctrain_stim_period) {
   para_itd_pip_ctrain_probe_region_lag=1;
  } else {
   para_itd_pip_ctrain_probe_region_lag=0;
  }

  /* Adapter region lead */
  if (counter0 >= para_itd_pip_ctrain_stim_instant_minus && \
      counter0 <  para_itd_pip_ctrain_stim_instant_minus+para_itd_pip_ctrain_ap_offset) {
   if ((counter0-para_itd_pip_ctrain_stim_instant_minus)/para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_no_adapters) {
    if ((counter0-para_itd_pip_ctrain_stim_instant_minus)%para_itd_pip_ctrain_iai \
 		                         < para_itd_pip_ctrain_stim_period) {
     para_itd_pip_ctrain_adapter_region_lead=1;
    } else {
     para_itd_pip_ctrain_adapter_region_lead=0;
    }
   }
  }

  /* Probe region lead */
  if (counter0 >= para_itd_pip_ctrain_stim_instant_minus+para_itd_pip_ctrain_ap_offset && \
      counter0 <  para_itd_pip_ctrain_stim_instant_minus+para_itd_pip_ctrain_ap_offset \
                                                  +para_itd_pip_ctrain_stim_period) {
   para_itd_pip_ctrain_probe_region_lead=1;
  } else {
   para_itd_pip_ctrain_probe_region_lead=0;
  }

 }

 /* ------ */

//   if (para_itd_pip_ctrain_adapter_region_lead) {
//    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus;
//    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
//                                                    para_itd_pip_ctrain_hi_period)
//     dac_0=AMP_OPPCHN;
//    else dac_0=0;
//   }
//   if (para_itd_pip_ctrain_probe_region_lead) {
//    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus \
//                                               -para_itd_pip_ctrain_ap_offset;
//    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
//                                                    para_itd_pip_ctrain_hi_period)
//     dac_0=AMP_OPPCHN;
 //   else dac_0=0;
 //  }

 dac_0=dac_1=0;

 switch (para_itd_pip_ctrain_laterality) {
  case 0: /* Center (0deg ITD) - 500Hz -> 500Hz  */
   if (para_itd_pip_ctrain_adapter_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_adapter_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
   }
   if (para_itd_pip_ctrain_adapter_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0)  ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
   }
   if (para_itd_pip_ctrain_probe_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0)  ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   break;

  case 1: /* Center (0deg ITD) - 1150Hz -> 1150Hz  */
   if (para_itd_pip_ctrain_adapter_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_adapter_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
   }
   if (para_itd_pip_ctrain_adapter_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0) ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
   }
   if (para_itd_pip_ctrain_probe_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0) ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   break;

  case 2: /* Center (0deg ITD) - 500Hz -> 1150Hz  */
   if (para_itd_pip_ctrain_adapter_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_adapter_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
   }
   if (para_itd_pip_ctrain_adapter_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0) ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
   }
   if (para_itd_pip_ctrain_probe_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0) ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   break;

  case 3: /* Center (0deg ITD) - 1150Hz -> 500Hz  */
   if (para_itd_pip_ctrain_adapter_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_adapter_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
   }
   if (para_itd_pip_ctrain_adapter_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0) ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta1150;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm1) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
                 (1-cos(2.0*M_PI*para_itd_pip_ctrain_theta2))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   if (para_itd_pip_ctrain_probe_region_norm2) {
     dac_0=dac_1=(short)((double)(AMP_OPPCHN)*sin(2.0*M_PI*para_itd_pip_ctrain_theta));
     para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
   }
   if (para_itd_pip_ctrain_probe_region_norm3) {
    dac_0=dac_1=(short)( (double)(AMP_OPPCHN)* \
		 sin(2.0*M_PI*para_itd_pip_ctrain_theta)* \
     (1-cos(2.0*M_PI*(para_itd_pip_ctrain_theta2+para_itd_pip_ctrain_delta_theta50/2.0)  ))/2.0 );
    para_itd_pip_ctrain_theta+=para_itd_pip_ctrain_delta_theta500;
    para_itd_pip_ctrain_theta2+=para_itd_pip_ctrain_delta_theta50;
   }
   break;

  case 4: /* Right IPSILATERAL */
   if (para_itd_pip_ctrain_adapter_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_probe_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_adapter_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_probe_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   break;

  case 5: /* LEFT IPSILATERAL */
   if (para_itd_pip_ctrain_adapter_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_probe_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_adapter_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_probe_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;

  case 6: /* RIGHT TRANSLATERAL */
   if (para_itd_pip_ctrain_adapter_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_probe_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_adapter_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_probe_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;

  case 7: /* LEFT TRANSLATERAL */
   if (para_itd_pip_ctrain_adapter_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_itd_pip_ctrain_probe_region_lead) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_minus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_adapter_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_itd_pip_ctrain_probe_region_lag) {
    para_itd_pip_ctrain_stim_local_offset=counter0-para_itd_pip_ctrain_stim_instant_plus \
                                               -para_itd_pip_ctrain_ap_offset;
    if (para_itd_pip_ctrain_stim_local_offset%para_itd_pip_ctrain_click_period < \
                                                    para_itd_pip_ctrain_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
  default: break;
 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_itd_pip_ctrain_soa;
}
