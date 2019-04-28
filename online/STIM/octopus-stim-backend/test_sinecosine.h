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
  theta=0.0; if (trig_active) outb(0x80|event0,STIM_PARBASE);
 } else if (counter1==TRIG_HI_STEPS) outb(0x00,STIM_PARBASE); /* TRIG down */

 if (event0==SEC_GEN_SINE) { /* Sine */
  if (theta<360.0) dac_3=(short)(256.0*sin(2.0*M_PI/360.0*theta)); else dac_3=0;
 } else if (event0==SEC_GEN_COSINE) { /* Cosine */
  if (theta<360.0) dac_3=(short)(256.0*cos(2.0*M_PI/360.0*theta)); else dac_3=0;
 }
 dac_0=dac_1=dac_2=0;
 counter1++; counter1%=50000; /* after 1 sec. */
 theta+=2.0*360.0/(double)(DA_SYNTH_RATE); /* f=2Hz */
}
