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

/* SQUARE WAVE TEST
    0.5second duration, 1 second repeat rate..
    ..to individually test averaging and jitter characteristics. */

static int test_square_duration;

static void test_square_init(void) {
 counter0=0; test_square_duration=12500;
}

static void test_square(void) { 
 if (counter0==0) {
  dac_3=256; if (trig_active) outb(0x80|SEC_SQUARE_WAVE,STIM_PARBASE);
 } else if (counter0==TRIG_HI_STEPS) outb(0x00,STIM_PARBASE); /* pull down.. */
 else if (counter0==test_square_duration) dac_3=0;
 dac_0=dac_1=dac_2=0;
 counter0++; counter0%=50000; /* after 1 sec. */
}
