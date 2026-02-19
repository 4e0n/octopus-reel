/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

#pragma once

#include <QtGlobal>
#include <QDataStream>
#include <vector>
#include <cstring>
#include "le_helper.h"

struct Sample {
 std::vector<float> data; // Raw amplifier data (wire format)
 unsigned int trigger=0,offset=0; // NOT on wire

 void init(size_t chnCount) { trigger=0; offset=0; data.assign(chnCount,0.0f); }
 void initSizeOnly(size_t chnCount) { trigger=0; offset=0; data.resize(chnCount); }

 Sample(size_t chnCount=0) { init(chnCount); }

 void serialize(QDataStream &out) const { for (float f:data) out<<f; }

 bool deserialize(QDataStream &in,size_t chnCount) {
  data.resize(chnCount);
  for (float &f:data) in >> f;
  return true;
 }

 inline void copyFrom(const Sample& src) {
  // assume sizes are stable; if not, fix once and keep it that way
  Q_ASSERT(data.size()==src.data.size());
//  if (data.size()!=src.data.size()) data.resize(src.data.size());
  std::memcpy(data.data(),src.data.data(),src.data.size()*sizeof(float));
  trigger=src.trigger; offset=src.offset;
 }

 // NEW: pointer-based deserialize matching serialize()
 bool deserialize(const char* src,int len,int chnCount,int* consumed) {
  if (!src || len<0 || chnCount<0) return false;
  const int needBytes=chnCount*4;
  if (len<needBytes) return false;

  data.resize(chnCount);

  // Read chnCount floats, little-endian IEEE754
  const char* p=src;
  for (int i=0;i<chnCount;++i) {
   quint32 w=rd_u32_le(p);
   float f; std::memcpy(&f,&w,4); data[i]=f;
   p+=4;
  }

  if (consumed) *consumed=needBytes;
  return true;
 }
};
