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

#ifndef _EEX_H
#define _EEX_H

#include "../acqglobals.h"

#ifdef EEMAGINE
using namespace eemagine::sdk;
#else
using namespace eesynth;
#endif

const unsigned int IIR_HIST_SIZE=4;

typedef struct _eex {
 unsigned int idx;
 amplifier *amp;
 std::vector<channel> chnList;
 stream *str;
 buffer buf;
 unsigned int t;
 unsigned int smpCount; // data count int buffer
 //unsigned int chnCount; // channel count int buffer -- redundant, to be asserted
 unsigned int baseSmpIdx; // Base Sample Index, to subtract from all upcoming basSmpIdx's
 unsigned int smpIdx; // Absolute Sample Index, as sent from the amplifier
 std::vector<float> imps;
 quint64 cBufIdx; //,cBufIdxP;
 std::vector<sample> cBuf;
 //std::vector<sample> cBufF;
 std::vector<std::array<double,IIR_HIST_SIZE> > fX,fY; // IIR filter History
} eex;

#endif
