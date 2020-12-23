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

/* STEADY SQUARE WAVE TEST
    Total period is 10 seconds.. identical with the one in calibration signal */

static int test_steadysquare_duration;

static void test_steadysquare_init(void) {
 counter0=0; test_steadysquare_duration=5*AUDIO_RATE; /* 5 seconds hi */
}

static void test_steadysquare(void) { 
 if (counter0==0) dac_0=0; 
 else if (counter0==test_steadysquare_duration) {
  dac_0=4096; if (trigger_active) trigger_set(SEC_STEADY_WAVE);
 }
 counter0++; counter0%=10*AUDIO_RATE; /*  Total period is 10 seconds */
}
