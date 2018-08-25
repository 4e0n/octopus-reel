/* PARADIGM 'CLICK'
    Simple click paradigm. Duration is 1msec. ISI is 1 second. */

static int para_click_duration;

static void para_click_init(void) {
 counter0=0; para_click_duration=50;
}

static void para_click(void) {
 if (counter0==0) {
  dac_0=dac_1=DA_NORM; /* output norm to range between +-20dB (2047/sqrt(10)) */
  if (trig_active) outb(0x80|SEC_CLICK,0x378);
 } else if (counter0==TRIG_HI_STEPS) outb(0x00,0x378); /* pull down */
 else if (counter0==para_click_duration) dac_0=dac_1=0;  
 counter0++; counter0%=DA_SYNTH_RATE; /* 1 second */
 dac_2=dac_3=0;
}
