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
 float sum0[PHYS_CHN_COUNT]; // avg of peri convN samples
 float sum1[PHYS_CHN_COUNT]; // avg of previous convL samples
 unsigned int trigger;
 unsigned int offset;
} sample;

#endif
