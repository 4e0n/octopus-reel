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
  if (counter0==0) { if (trigger_active) trigger_set(SEC_SQUARE_BURST); }
  //else if (counter0==TRIG_HI_STEPS) trigger_reset(); // pull down
  if ((counter0%para_squareburst_t2)<para_squareburst_t1) dac_0=dac_1=AMP_H20;
  else dac_0=dac_1=0;
 } else dac_0=dac_1=0;
 counter0++; counter0%=AUDIO_RATE; /* 1 second */
}
