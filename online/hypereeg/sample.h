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

#include <vector>
#include <QDataStream>

struct Sample {
 std::vector<float> data;  // [0.1-100]Hz IIR filtered amplifier data.
 std::vector<float> dataF; // 30Q @ 50Hz IIR notch filtered version of data
 unsigned int trigger=0,offset=0;

 void init(size_t chnCount) {
  trigger=0; offset=0;
  data.assign(chnCount, 0.0f);
  dataF.assign(chnCount, 0.0f);
 }

 Sample(size_t chnCount=0) { init(chnCount); }
 
 void serialize(QDataStream &out) const {
  //out<<static_cast<quint32>(data.size());
  for (float f:data) out<<f;
  for (float f:dataF) out<<f;
 }

 bool deserialize(QDataStream &in,size_t chnCount) {
  //quint32 len;
  //if (in.atEnd()) return false;
  //in>>len;
  data.resize(chnCount); dataF.resize(chnCount);
  for (float &f:data) in>>f;
  for (float &f:dataF) in>>f;
  return true; //!in.atEnd();
 }
};

#endif
