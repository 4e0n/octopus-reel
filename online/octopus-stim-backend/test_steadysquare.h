/* STEADY SQUARE WAVE TEST
    Total period is 10 seconds.. identical with the one in calibration signal */

static int test_steadysquare_duration;

static void test_steadysquare_init(void) {
 counter0=0; test_steadysquare_duration=5*DA_SYNTH_RATE; /* 5 seconds hi */
}

static void test_steadysquare(void) { 
 if (counter0==0) dac_3=0; 
 else if (counter0==test_steadysquare_duration) {
  dac_3=250; if (trig_active) outb(0x80|SEC_STEADY_WAVE,0x378);
 } else if (counter0==test_steadysquare_duration+TRIG_HI_STEPS)
  outb(0x00,0x378); /* pull down */
 dac_0=dac_1=dac_2=0;
 counter0++; counter0%=10*DA_SYNTH_RATE; /*  Total period is 10 seconds */
}
