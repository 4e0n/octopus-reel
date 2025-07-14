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

typedef struct _Sample {
 float marker=0.;
 std::vector<float> data;  // [0.1-100]Hz IIR filtered amplifier data.
 std::vector<float> dataF; // 30Q @ 50Hz IIR notch filtered version of data
 unsigned int trigger=0,offset=0;

 void init(size_t chnCount) {
  marker=0.; trigger=0;
  data.assign(chnCount, 0.0f);
  dataF.assign(chnCount, 0.0f);
 }

 _Sample(size_t chnCount=0) { init(chnCount); }
 
} Sample;

#endif
