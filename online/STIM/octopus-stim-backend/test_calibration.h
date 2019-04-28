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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

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
