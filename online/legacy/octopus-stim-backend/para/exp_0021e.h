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

#define PARA_0021E_L680_L680	1	// PARA0021 L680->L680 (Adapter->Probe)
#define PARA_0021E_L680_L680DA	2	// PARA0021 L680->L680 (Adapter->Probe) Double Adapter
#define PARA_0021E_L680_R680	3	// PARA0021 L680->R680 (Adapter->Probe)
#define PARA_0021E_L680_R680DA	4	// PARA0021 L680->R680 (Adapter->Probe) Double Adapter
#define PARA_0021E_R680_R680	5	// PARA0021 R680->R680 (Adapter->Probe)
#define PARA_0021E_R680_R680DA	6	// PARA0021 R680->R680 (Adapter->Probe) Double Adapter
#define PARA_0021E_R680_L680	7	// PARA0021 R680->L680 (Adapter->Probe)
#define PARA_0021E_R680_L680DA	8	// PARA0021 R680->L680 (Adapter->Probe) Double Adapter

#define PARA_0021E_L160_L160	9	// PARA0021 L160->L160 (Adapter->Probe)
#define PARA_0021E_L160_L160DA	10	// PARA0021 L160->L160 (Adapter->Probe) Double Adapter
#define PARA_0021E_L160_R160	11	// PARA0021 L160->R160 (Adapter->Probe)
#define PARA_0021E_L160_R160DA	12	// PARA0021 L160->R160 (Adapter->Probe) Double Adapter
#define PARA_0021E_R160_R160	13	// PARA0021 R160->R160 (Adapter->Probe)
#define PARA_0021E_R160_R160DA	14	// PARA0021 R160->R160 (Adapter->Probe) Double Adapter
#define PARA_0021E_R160_L160	15	// PARA0021 R160->L160 (Adapter->Probe)
#define PARA_0021E_R160_L160DA	16	// PARA0021 R160->L160 (Adapter->Probe) Double Adapter

#define PARA_0021E_C_C		17	// PARA0021 C->C (Adapter->Probe)
#define PARA_0021E_C_CDA	18	// PARA0021 C->C (Adapter->Probe) Double Adapter

#define PARA_0021E_L360_L360	19	// PARA0021 L360->L360 (Adapter->Probe)
#define PARA_0021E_L360_L360DA	20	// PARA0021 L360->L360 (Adapter->Probe) Double Adapter
#define PARA_0021E_L360_R360	21	// PARA0021 L360->R360 (Adapter->Probe)
#define PARA_0021E_L360_R360DA	22	// PARA0021 L360->R360 (Adapter->Probe) Double Adapter
#define PARA_0021E_R360_R360	23	// PARA0021 R360->R360 (Adapter->Probe)
#define PARA_0021E_R360_R360DA	24	// PARA0021 R360->R360 (Adapter->Probe) Double Adapter
#define PARA_0021E_R360_L360	25	// PARA0021 R360->L360 (Adapter->Probe)
#define PARA_0021E_R360_L360DA	26	// PARA0021 R360->L360 (Adapter->Probe) Double Adapter

#define PARA_0021E_L220_L220	27	// PARA0021 L220->L220 (Adapter->Probe)
#define PARA_0021E_L220_L220DA	28	// PARA0021 L220->L220 (Adapter->Probe) Double Adapter
#define PARA_0021E_L220_R220	29	// PARA0021 L220->R220 (Adapter->Probe)
#define PARA_0021E_L220_R220DA	30	// PARA0021 L220->R220 (Adapter->Probe) Double Adapter
#define PARA_0021E_R220_R220	31	// PARA0021 R220->R220 (Adapter->Probe)
#define PARA_0021E_R220_R220DA	32	// PARA0021 R220->R220 (Adapter->Probe) Double Adapter
#define PARA_0021E_R220_L220	33	// PARA0021 R220->L220 (Adapter->Probe)
#define PARA_0021E_R220_L220DA	34	// PARA0021 R220->L220 (Adapter->Probe) Double Adapter

static int para_0021e_experiment_loop=1;

static int para_0021e_trigger,para_0021e_soa,
	   para_0021e_iai,         /* Inter-adapter-interval */

	   /* Stim atomic properties */
	   para_0021e_click_period,para_0021e_no_periods_probe,
	   para_0021e_hi_period,
	   para_0021e_adapter_total_dur_base, /* 750ms */
	   para_0021e_adapter_period,para_0021e_probe_period,
	   para_0021e_stim_local_offset,

	   //para_0021e_lr_delta,
	   para_0021e_lr_delta160,para_0021e_lr_delta220,para_0021e_lr_delta360,para_0021e_lr_delta680,

	   para_0021e_adapter_region_center,
	   para_0021e_adapter_region_lag,para_0021e_adapter_region_lead,
	   para_0021e_probe_region_center,
	   para_0021e_probe_region_lag,para_0021e_probe_region_lead,
	   para_0021e_stim_instant,para_0021e_double_adapter,

	   para_0021e_adapter_type,para_0021e_probe_type,
	   para_0021e_ap_offset,

	   para_0021e_stim_instant_center,para_0021e_stim_instant_minus,para_0021e_stim_instant_plus,
	   para_0021e_stim_instant_minus160,para_0021e_stim_instant_minus220,
	   para_0021e_stim_instant_minus360,para_0021e_stim_instant_minus680,
	   para_0021e_stim_instant_plus160,para_0021e_stim_instant_plus220,
	   para_0021e_stim_instant_plus360,para_0021e_stim_instant_plus680;

static void para_0021e_init(void) {
 current_pattern_offset=0;

 para_0021e_soa=(3.50001)*AUDIO_RATE; /* 4.0 seconds */
 para_0021e_ap_offset=(0.400001)*AUDIO_RATE; /* DISCRETE 0.2 */
 para_0021e_hi_period=(0.00051)*AUDIO_RATE; /* 500 us - 25 steps */
 para_0021e_click_period=(0.01001)*AUDIO_RATE; /* 10 ms - 700 steps */
 /* --------- */
 para_0021e_no_periods_probe=5; /* 5*10 ms */

 para_0021e_adapter_total_dur_base=para_0021e_ap_offset; /* 400ms */
 para_0021e_probe_period=para_0021e_click_period*para_0021e_no_periods_probe; /* Probe Click Pip duration -> 50ms */
 para_0021e_adapter_period=para_0021e_probe_period; /* Adapter Click Pip duration -> 50ms */

 para_0021e_iai=(0.200001)*AUDIO_RATE; /* Inter-adapter Interval: 200ms (for multiple adapters within the adapter region) */;
 /* --------- */

 para_0021e_lr_delta160=(0.000161)*AUDIO_RATE; /*  L-R delta: 160us -  8 steps */
 para_0021e_lr_delta220=(0.000221)*AUDIO_RATE; /*  L-R delta: 220us - 11 steps */
 para_0021e_lr_delta360=(0.000361)*AUDIO_RATE; /*  L-R delta: 360us - 18 steps */
 para_0021e_lr_delta680=(0.000681)*AUDIO_RATE; /*  L-R delta: 680us - 34 steps */

 para_0021e_stim_instant=(0.200001)*AUDIO_RATE; /* 200ms (arbitrary at beginning) */;
 para_0021e_stim_instant_center=para_0021e_stim_instant;
 para_0021e_stim_instant_minus160=para_0021e_stim_instant-para_0021e_lr_delta160/2;
 para_0021e_stim_instant_plus160=para_0021e_stim_instant+para_0021e_lr_delta160/2;
 para_0021e_stim_instant_minus220=para_0021e_stim_instant-para_0021e_lr_delta220/2;
 para_0021e_stim_instant_plus220=para_0021e_stim_instant+para_0021e_lr_delta220/2;
 para_0021e_stim_instant_minus360=para_0021e_stim_instant-para_0021e_lr_delta360/2;
 para_0021e_stim_instant_plus360=para_0021e_stim_instant+para_0021e_lr_delta360/2;
 para_0021e_stim_instant_minus680=para_0021e_stim_instant-para_0021e_lr_delta680/2;
 para_0021e_stim_instant_plus680=para_0021e_stim_instant+para_0021e_lr_delta680/2;

 rt_printk("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                         para_0021e_adapter_total_dur_base,
		 	 para_0021e_hi_period,
		 	 para_0021e_click_period,
		 	 para_0021e_lr_delta160,
		 	 para_0021e_lr_delta220,
		 	 para_0021e_lr_delta360,
		 	 para_0021e_lr_delta680,
		 	 para_0021e_stim_instant,
		 	 para_0021e_stim_instant_center,
		 	 para_0021e_stim_instant_minus160,
		 	 para_0021e_stim_instant_minus220,
		 	 para_0021e_stim_instant_minus360,
		 	 para_0021e_stim_instant_minus680,
		 	 para_0021e_stim_instant_plus160,
		 	 para_0021e_stim_instant_plus220,
		 	 para_0021e_stim_instant_plus360,
		 	 para_0021e_stim_instant_plus680);
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim initialized.\n");
}

static void para_0021e_start(void) {
 lights_dimm();
 counter0=0; counter1=0; trigger_active=1; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim started.\n");
}

static void para_0021e_stop(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim stopped.\n");
}

static void para_0021e_pause(void) {
 audio_active=0;
 lights_on();
 rt_printk("octopus-stim-backend.o: Stim paused.\n");
}

static void para_0021e_resume(void) {
 lights_dimm();
 counter0=0; counter1=0; audio_active=1;
 rt_printk("octopus-stim-backend.o: Stim resumed.\n");
}

static void para_0021e(void) {
 //int dummy_counter=0;

 if (counter0==0) {
refetch:
  current_pattern_data=patt_buf[current_pattern_offset]; /* fetch new.. */
  switch (current_pattern_data) {
   case '@': /* Interblock pause */
             para_0021e_pause(); /* legitimate pause */
	     break;
   /* Click Train L680 -> L680 */
   case 'A': para_0021e_trigger=PARA_0021E_L680_L680;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=0;
             break;
   case 'B': para_0021e_trigger=PARA_0021E_L680_L680DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train L680 -> R680 */
   case 'C': para_0021e_trigger=PARA_0021E_L680_R680;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=0;
             break;
   case 'D': para_0021e_trigger=PARA_0021E_L680_R680DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R680 -> R680 */
   case 'E': para_0021e_trigger=PARA_0021E_R680_R680;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=0;
             break;
   case 'F': para_0021e_trigger=PARA_0021E_R680_R680DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R680 -> L680 */
   case 'G': para_0021e_trigger=PARA_0021E_R680_L680;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=0;
             break;
   case 'H': para_0021e_trigger=PARA_0021E_R680_L680DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus680;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus680;
	     para_0021e_double_adapter=1;
             break;

   /* Click Train L160 -> L160 */
   case 'I': para_0021e_trigger=PARA_0021E_L160_L160;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=0;
             break;
   case 'J': para_0021e_trigger=PARA_0021E_L160_L160DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train L160 -> R160 */
   case 'K': para_0021e_trigger=PARA_0021E_L160_R160;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=0;
             break;
   case 'L': para_0021e_trigger=PARA_0021E_L160_R160DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R160 -> R160 */
   case 'M': para_0021e_trigger=PARA_0021E_R160_R160;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=0;
             break;
   case 'N': para_0021e_trigger=PARA_0021E_R160_R160DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R160 -> L160 */
   case 'O': para_0021e_trigger=PARA_0021E_R160_L160;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=0;
             break;
   case 'P': para_0021e_trigger=PARA_0021E_R160_L160DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus160;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus160;
	     para_0021e_double_adapter=1;
             break;

   /* Click Train C -> C */
   case 'Q': para_0021e_trigger=PARA_0021E_C_C;
	     para_0021e_adapter_type=0; para_0021e_probe_type=0;
	     para_0021e_double_adapter=0;
             break;
   case 'R': para_0021e_trigger=PARA_0021E_C_CDA;
	     para_0021e_adapter_type=0; para_0021e_probe_type=0;
	     para_0021e_double_adapter=1;
	     break;

   /* Click Train L360 -> L360 */
   case 'S': para_0021e_trigger=PARA_0021E_L360_L360;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=0;
             break;
   case 'T': para_0021e_trigger=PARA_0021E_L360_L360DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train L360 -> R360 */
   case 'U': para_0021e_trigger=PARA_0021E_L360_R360;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=0;
             break;
   case 'V': para_0021e_trigger=PARA_0021E_L360_R360DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R360 -> R360 */
   case 'W': para_0021e_trigger=PARA_0021E_R360_R360;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=0;
             break;
   case 'X': para_0021e_trigger=PARA_0021E_R360_R360DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R360 -> L360 */
   case 'Y': para_0021e_trigger=PARA_0021E_R360_L360;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=0;
             break;
   case 'Z': para_0021e_trigger=PARA_0021E_R360_L360DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus360;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus360;
	     para_0021e_double_adapter=1;
             break;

   /* Click Train L220 -> L220 */
   case '1': para_0021e_trigger=PARA_0021E_L220_L220;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=0;
             break;
   case '2': para_0021e_trigger=PARA_0021E_L220_L220DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train L220 -> R220 */
   case '3': para_0021e_trigger=PARA_0021E_L220_R220;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=0;
             break;
   case '4': para_0021e_trigger=PARA_0021E_L220_R220DA;
	     para_0021e_adapter_type=1; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R220 -> R220 */
   case '5': para_0021e_trigger=PARA_0021E_R220_R220;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=0;
             break;
   case '6': para_0021e_trigger=PARA_0021E_R220_R220DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=2;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=1;
             break;
   /* Click Train R220 -> L220 */
   case '7': para_0021e_trigger=PARA_0021E_R220_L220;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=0;
             break;
   case '8': para_0021e_trigger=PARA_0021E_R220_L220DA;
	     para_0021e_adapter_type=2; para_0021e_probe_type=1;
	     para_0021e_stim_instant_minus=para_0021e_stim_instant_minus220;
	     para_0021e_stim_instant_plus=para_0021e_stim_instant_plus220;
	     para_0021e_double_adapter=1;
             break;

   default:  current_pattern_offset++; /* If invalid then bypass */
             if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
              if (para_0021e_experiment_loop==0) para_0021e_stop();
              current_pattern_offset=0;
             }
             goto refetch;
	     //break;
  }

  current_pattern_offset++;
  if (current_pattern_offset==pattern_size) { /* roll-back or stop? */
   if (para_0021e_experiment_loop==0) para_0021e_stop();
   //else para_0021e_pause();
   current_pattern_offset=0;
  }
 }

 /* ------------------------------------------------------------------- */
 /* Trigger */
 if ( (counter0==para_0021e_stim_instant+para_0021e_ap_offset) && trigger_active &&
      (current_pattern_offset>0) )
  trigger_set(para_0021e_trigger);
 /* ------------------------------------------------------------------- */

 /* Adapter region center */
 if (counter0 >= para_0021e_stim_instant_center && \
     counter0 <  para_0021e_stim_instant_center+para_0021e_adapter_total_dur_base) {
  if (para_0021e_double_adapter) {
   if ((counter0-para_0021e_stim_instant_center)%para_0021e_iai < para_0021e_adapter_period)
    para_0021e_adapter_region_center=1;
   else  para_0021e_adapter_region_center=0;
  } else {
   if ((counter0-para_0021e_stim_instant_center) < para_0021e_adapter_period)
    para_0021e_adapter_region_center=1;
   else  para_0021e_adapter_region_center=0;
  }
 } else {
  para_0021e_adapter_region_center=0;
 }

 /* Adapter region (left) lead */
 if (counter0 >= para_0021e_stim_instant_minus && \
     counter0 <  para_0021e_stim_instant_minus+para_0021e_adapter_total_dur_base) {
  if (para_0021e_double_adapter) {
   if ((counter0-para_0021e_stim_instant_minus)%para_0021e_iai < para_0021e_adapter_period)
    para_0021e_adapter_region_lead=1;
   else para_0021e_adapter_region_lead=0;
  } else {
   if ((counter0-para_0021e_stim_instant_minus) < para_0021e_adapter_period)
    para_0021e_adapter_region_lead=1;
   else para_0021e_adapter_region_lead=0;
  }
 } else {
  para_0021e_adapter_region_lead=0;
 }

 /* Adapter region (left) lag */
 if (counter0 >= para_0021e_stim_instant_plus && \
     counter0 <  para_0021e_stim_instant_plus+para_0021e_adapter_total_dur_base) {
  if (para_0021e_double_adapter) {
   if ((counter0-para_0021e_stim_instant_plus)%para_0021e_iai < para_0021e_adapter_period)
    para_0021e_adapter_region_lag=1;
   else para_0021e_adapter_region_lag=0;
  } else {
   if ((counter0-para_0021e_stim_instant_plus) < para_0021e_adapter_period)
    para_0021e_adapter_region_lag=1;
   else para_0021e_adapter_region_lag=0;
  }
 } else {
  para_0021e_adapter_region_lag=0;
 }

 /* Probe region center */
 if (counter0 >= para_0021e_stim_instant_center+para_0021e_ap_offset && \
     counter0 <  para_0021e_stim_instant_center+para_0021e_ap_offset+para_0021e_probe_period) {
  para_0021e_probe_region_center=1;
 } else {
  para_0021e_probe_region_center=0;
 }

 /* Probe region (left) lead */
 if (counter0 >= para_0021e_stim_instant_minus+para_0021e_ap_offset && \
     counter0 <  para_0021e_stim_instant_minus+para_0021e_ap_offset+para_0021e_probe_period) {
  para_0021e_probe_region_lead=1;
 } else {
  para_0021e_probe_region_lead=0;
 }

 /* Probe region (left) lag */
 if (counter0 >= para_0021e_stim_instant_plus+para_0021e_ap_offset && \
     counter0 <  para_0021e_stim_instant_plus+para_0021e_ap_offset+para_0021e_probe_period) {
  para_0021e_probe_region_lag=1;
 } else {
  para_0021e_probe_region_lag=0;
 }

 /* ------ */

 dac_0=dac_1=0;

 switch (para_0021e_adapter_type) {
  case 0: /* Center */
   if (para_0021e_adapter_region_center) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant%para_0021e_probe_period;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=dac_1=AMP_OPPCHN;
    else dac_0=dac_1=0;
   }
   break;
  case 1: /* Left Lead */
   if (para_0021e_adapter_region_lead) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_minus;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_0021e_adapter_region_lag) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_plus;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;
  case 2: /* Right Lead*/
   if (para_0021e_adapter_region_lead) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_minus;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_0021e_adapter_region_lag) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_plus;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   break;
 }

 switch (para_0021e_probe_type) {
  case 0: /* Center */
   if (para_0021e_probe_region_center) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant-para_0021e_ap_offset;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=dac_1=AMP_OPPCHN;
    else dac_0=dac_1=0;
   }
   break;
  case 1: /* Left Lead (L680,L360,L220,L160) */
   if (para_0021e_probe_region_lead) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_minus-para_0021e_ap_offset;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
   if (para_0021e_probe_region_lag) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_plus-para_0021e_ap_offset;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   break;
  case 2: /* Right Lead (R680,R360,R220,R160) */
   if (para_0021e_probe_region_lead) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_minus-para_0021e_ap_offset;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_1=AMP_OPPCHN;
    else dac_1=0;
   }
   if (para_0021e_probe_region_lag) {
    para_0021e_stim_local_offset=counter0-para_0021e_stim_instant_plus-para_0021e_ap_offset;
    if (para_0021e_stim_local_offset%para_0021e_click_period < para_0021e_hi_period)
     dac_0=AMP_OPPCHN;
    else dac_0=0;
   }
  default: break;
 }

 /* ------------------------------------------------------------------- */

 counter0++;
 counter0%=para_0021e_soa;
}
