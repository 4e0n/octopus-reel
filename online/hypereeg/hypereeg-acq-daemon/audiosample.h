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

#ifndef _AUDIOSAMPLE_H
#define _AUDIOSAMPLE_H

#include <vector>
#include <QDataStream>

struct AudioSample {
 std::vector<float> data;  // Raw audio data.
 std::vector<float> dataF; // (Probably) some realtime computed derivative.. maybe envelope data (Hilbert transform?)
 unsigned int offset=0;

 void init(size_t setCount) { // setCount is the ratio of sample rates for Audio/EEG.
  offset=0;
  data.assign(setCount, 0.0f);
  dataF.assign(setCount, 0.0f);
 }

 AudioSample(size_t chnCount=0) { init(chnCount); }
 
 void serialize(QDataStream &out) const {
  for (float f:data) out<<f;
  for (float f:dataF) out<<f;
 }

 bool deserialize(QDataStream &in,size_t setCount) {
  data.resize(setCount); dataF.resize(setCount);
  for (float &f:data) in>>f;
  for (float &f:dataF) in>>f;
  return true;
 }
};

#endif
