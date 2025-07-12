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
 std::vector<float> data;
 std::vector<float> dataF;
// std::vector<float> curCM;
// std::vector<float> sum0; // peri-convN samples
// std::vector<float> com0; // peri-convN 50Hz common mode noise square
// std::vector<float> sum1; // previous convL samples
 unsigned int trigger=0;
 unsigned int offset;

 void init(size_t chnCount) {
  marker=0.; trigger=0;
  data.assign(chnCount, 0.0f);
  dataF.assign(chnCount, 0.0f);
  //dataZ.assign(chnCount, 0.0f);
  //dataCAR.assign(chnCount, 0.0f);
  //dataCM.assign(chnCount, 0.0f);
 }

 //_Sample(size_t chnCount=0) : data(chnCount,0),dataF(chnCount,0) {}
 _Sample(size_t chnCount=0) { init(chnCount); }
 
} Sample;

#endif
