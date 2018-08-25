/* SQUARE WAVE TEST
    0.5second duration, 1 second repeat rate..
    ..to individually test averaging and jitter characteristics. */

static int test_square_duration;

static void test_square_init(void) {
 counter0=0; test_square_duration=12500;
}

static void test_square(void) { 
 if (counter0==0) {
  dac_3=256; if (trig_active) outb(0x80|SEC_SQUARE_WAVE,0x378);
 } else if (counter0==TRIG_HI_STEPS) outb(0x00,0x378); /* pull down.. */
 else if (counter0==test_square_duration) dac_3=0;
 dac_0=dac_1=dac_2=0;
 counter0++; counter0%=50000; /* after 1 sec. */
}
