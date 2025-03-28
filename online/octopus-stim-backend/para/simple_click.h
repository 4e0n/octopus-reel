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

/* PARADIGM 'CLICK'
    Simple click paradigm. Duration is 1msec. ISI is 1 second. */

#define SEC_CLICK		10	// "Click"

static int para_click_duration;

static void para_click_init(void) {
 counter0=0; para_click_duration=50;
}

static void para_click(void) {
 if (counter0==0) {
  /* output norm to range between +-20dB (2047/sqrt(10)) */
  dac_0=dac_1=AMP_H20;
  if (trigger_active) trigger_set(SEC_CLICK);
 }
 else if (counter0==para_click_duration) dac_0=dac_1=0;  
 counter0++; counter0%=AUDIO_RATE; /* 1 second */
}
