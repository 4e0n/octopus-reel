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

#ifndef GLOBALS
#define GLOBALS

//#define EEMAGINE

const unsigned int EE_AMPCOUNT=2;
const unsigned int EE_MAX_AMPCOUNT=8;

const int SAMPLE_RATE=1000;

const int EEG_PROBE_MSECS=100;
const int IMPEDANCE_PROBE_MSECS=100;

const int REF_CHN_COUNT=64; // 0
const int REF_CHN_MAXCOUNT=64;

const int BIP_CHN_COUNT=2; // 2
const int BIP_CHN_MAXCOUNT=24;

const int PHYS_CHN_COUNT=REF_CHN_COUNT+BIP_CHN_COUNT;
const int TOTAL_CHN_COUNT=PHYS_CHN_COUNT+2;

const float EE_REF_GAIN=1.;
const float EE_BIP_GAIN=4.;

const unsigned int CBUF_SIZE_IN_SECS=10;

#endif
