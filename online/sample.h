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

#ifndef _SAMPLE_H
#define _SAMPLE_H

#include "acqglobals.h"

typedef struct _sample {
 float marker;
 float data[PHYS_CHN_COUNT];
 float dataF[PHYS_CHN_COUNT];
 float curCM[PHYS_CHN_COUNT];
 float sum0[PHYS_CHN_COUNT]; // avg of peri convN samples
 float sum1[PHYS_CHN_COUNT]; // avg of previous convL samples
 unsigned int trigger;
 unsigned int offset;
} sample;

/*
typedef struct _sample {
 size_t size;
 float marker;
 float *data,*dataF;
 float *sum0,*sum1; // avg of 0: peri convN, 1: of prev. convL samples
 unsigned int trigger,offset;

 _sample(size_t s) : size(s) {
  data(new float[s]);
  dataF(new float[s]);
  sum0(new float[s]);
  sum1(new float[s]);
 }

 ~MyStruct() {
  delete[] data; delete[] dataF; delete[] sum0; delete[] sum1;
 }

 // Rule of three/five: optional depending on copying needs
 _sample(const _sample&) = delete;
 _sample& operator=(const _sample&) = delete;
} sample;
*/

#endif
