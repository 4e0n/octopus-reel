
static int test_calibration_phase1;

static void test_calibration_init(void) {
 counter0=0; theta=0.;
 test_calibration_phase1=1200*DA_SYNTH_RATE;  /* End of DC   -> 20 min */
}

/* DAC's DC level itself is inherently..       */
/* subtructed in this calibration process!     */
/* if our amplifier didn't have any hipass flt */

static void test_calibration(void) {
 if (counter0 < test_calibration_phase1) dac_3=(short)(0.);
 else if (counter0 >= test_calibration_phase1) {
  dac_3=(short)(256.*sin(theta*M_PI/180.));
  /* Supposed to be 12Hz.. existing at some place of about
     the center of EEG amplifier's bandwidth.. */
  theta+=12.*360./(float)(DA_SYNTH_RATE); if (theta>=360.) theta-=360.;
 } dac_0=dac_1=dac_2=0; counter0++;
}
