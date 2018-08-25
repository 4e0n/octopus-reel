/* FULL PHASE SINE AND COSINE EVENTS
    10 seconds period square wave of 50% duty cycle w/ riseups of event#127.
     counter0: Alternating Amplitudes Counter
     counter1: Counter of subsequent sines or cosines */

/* Full phase sine and cosine evts to individually test chns and averaging. */
static void test_sinecosine_init(void) {
 event0=SEC_GEN_COSINE;
 counter0=counter1=0; theta=0.0; var0=256; /* First low amp.. */
}

static void test_sinecosine(void) { 
 if (counter0==0) {
  if (var0==256) var0=25; else if (var0==25) var0=256;
 }

 if (counter1==0) {
  if (event0==SEC_GEN_COSINE) event0=SEC_GEN_SINE;
  else if (event0==SEC_GEN_SINE) event0=SEC_GEN_COSINE;
  theta=0.0; if (trig_active) outb(0x80|event0,0x378);
 } else if (counter1==TRIG_HI_STEPS) outb(0x00,0x378); /* TRIG down */

 if (event0==SEC_GEN_SINE) { /* Sine */
  if (theta<360.0) dac_3=(short)(256.0*sin(2.0*M_PI/360.0*theta)); else dac_3=0;
 } else if (event0==SEC_GEN_COSINE) { /* Cosine */
  if (theta<360.0) dac_3=(short)(256.0*cos(2.0*M_PI/360.0*theta)); else dac_3=0;
 }
 dac_0=dac_1=dac_2=0;
 counter1++; counter1%=50000; /* after 1 sec. */
 theta+=2.0*360.0/(double)(DA_SYNTH_RATE); /* f=2Hz */
}
