/* PARADIGM 'SQUAREBURST'
    Simple squareburst paradigm. Duration is 1msec. ISI is 1 second. */

static int para_squareburst_duration,
           para_squareburst_t1,para_squareburst_t2,para_squareburst_t3;

static void para_squareburst_init(void) {
 counter0=0; para_squareburst_duration=50;
 para_squareburst_t1=25;   /*  0.5msec */
 para_squareburst_t2=500;  /* 10.0msec */
 para_squareburst_t3=2500; /* 50.0msec */
}

static void para_squareburst(void) {
 if (counter0<para_squareburst_t3) {
  if (counter0==0) { if (trig_active) outb(0x80|SEC_SQUARE_BURST,0x378); }
  else if (counter0==TRIG_HI_STEPS) outb(0x00,0x378); /* pull down */
  if ((counter0%para_squareburst_t2)<para_squareburst_t1) dac_0=dac_1=AMP_H20;
  else dac_0=dac_1=0;
 } else dac_0=dac_1=0;
 counter0++; counter0%=DA_SYNTH_RATE; /* 1 second */
 dac_2=dac_3=0;
}
