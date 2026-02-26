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

#include <vector>
#include <QDataStream>
#include <cstring>      // memcpy
#include "le_helper.h"  // rd_u32_le()

struct SamplePP {
 std::vector<float> data;    // Raw
 std::vector<float> dataBP;  // Bandpass (2-40 Hz)
 std::vector<float> dataN;   // Notch-indicator / notch-filtered
 //std::vector<float> dataD; // [2-4]Hz IIR filtered Delta data.
 //std::vector<float> dataT; // [4-8]Hz IIR filtered Theta data.
 //std::vector<float> dataA; // [8-14]Hz IIR filtered Alpha data.
 //std::vector<float> dataB; // [14-28]Hz IIR filtered Beta data.
 //std::vector<float> dataG; // [28-40]Hz IIR filtered Gamma data.
 unsigned int trigger=0, offset=0; // NOT on wire

 void init(size_t chnCount) {
  trigger=0; offset=0;
  data.assign(chnCount,0.0f);
  dataBP.assign(chnCount,0.0f);
  dataN.assign(chnCount,0.0f);
  //dataD.assign(chnCount,0.0f); dataT.assign(chnCount,0.0f); dataA.assign(chnCount,0.0f);
  //dataB.assign(chnCount,0.0f); dataG.assign(chnCount,0.0f);
 }

 void initSizeOnly(size_t chnCount) {
  trigger=0; offset=0;
  data.resize(chnCount);
  dataBP.resize(chnCount); dataN.resize(chnCount);
  //dataD.resize(chnCount,0.0f); dataT.resize(chnCount,0.0f); dataA.resize(chnCount,0.0f);
  //dataB.resize(chnCount,0.0f); dataG.resize(chnCount,0.0f);
 }

 SamplePP(size_t chnCount=0) { init(chnCount); }

 // Wire format: data, dataBP, dataN (all float32 LE)
 void serialize(QDataStream &out) const {
  for (float f:data)   out<<f;
  for (float f:dataBP) out<<f;
  for (float f:dataN)  out<<f;
  //for (float f:dataD) out<<f;
  //for (float f:dataT) out<<f;
  //for (float f:dataA) out<<f;
  //for (float f:dataB) out<<f;
  //for (float f:dataG) out<<f;
 }

 bool deserialize(QDataStream &in, size_t chnCount) {
  data.resize(chnCount); dataBP.resize(chnCount); dataN.resize(chnCount);
  //dataD.resize(chnCount); dataT.resize(chnCount); dataA.resize(chnCount);
  //dataB.resize(chnCount); dataG.resize(chnCount);

  for (float &f:data)   in>>f;
  for (float &f:dataBP) in>>f;
  for (float &f:dataN)  in>>f;
  //for (float &f:dataD)  in>>f;
  //for (float &f:dataT)  in>>f;
  //for (float &f:dataA)  in>>f;
  //for (float &f:dataB)  in>>f;
  //for (float &f:dataG)  in>>f;
  return true;
 }

 // pointer-based deserialize
 bool deserialize(const char* src,int len,int chnCount,int* consumed) {
  if (!src || len<0 || chnCount<0) return false;

  const int needBytes=chnCount*3*4; // data+dataBP+dataN
  //const int needBytes=chnCount*8*4; // data+dataBP+dataN+dataD+dataT+...
  if (len<needBytes) return false;

  data.resize(chnCount); dataBP.resize(chnCount); dataN.resize(chnCount);
  //dataD.resize(chnCount); dataT.resize(chnCount); dataA.resize(chnCount);
  //dataB.resize(chnCount); dataG.resize(chnCount);

  const char* p=src;

  auto rd_f32=[&](float &dst) { quint32 w=rd_u32_le(p); std::memcpy(&dst,&w,4); p+=4; };

  for (int i=0;i<chnCount;++i) rd_f32(data[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataBP[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataN[i]);
  //for (int i=0;i<chnCount;++i) rd_f32(dataD[i]);
  //for (int i=0;i<chnCount;++i) rd_f32(dataT[i]);
  //for (int i=0;i<chnCount;++i) rd_f32(dataA[i]);
  //for (int i=0;i<chnCount;++i) rd_f32(dataB[i]);
  //for (int i=0;i<chnCount;++i) rd_f32(dataG[i]);

  if (consumed) *consumed=needBytes;
  return true;
 }

 inline void copyFrom(const SamplePP& src) {
  Q_ASSERT(data.size()==src.data.size());
  Q_ASSERT(dataBP.size()==src.dataBP.size());
  Q_ASSERT(dataN.size()==src.dataN.size());
  std::memcpy(data.data(),  src.data.data(),  src.data.size()*sizeof(float));
  std::memcpy(dataBP.data(),src.dataBP.data(),src.dataBP.size()*sizeof(float));
  std::memcpy(dataN.data(), src.dataN.data(), src.dataN.size()*sizeof(float));
  //std::memcpy(dataD.data(), src.dataD.data(), src.dataD.size()*sizeof(float));
  //std::memcpy(dataT.data(), src.dataT.data(), src.dataT.size()*sizeof(float));
  //std::memcpy(dataA.data(), src.dataA.data(), src.dataA.size()*sizeof(float));
  //std::memcpy(dataB.data(), src.dataB.data(), src.dataB.size()*sizeof(float));
  //std::memcpy(dataG.data(), src.dataG.data(), src.dataG.size()*sizeof(float));
  trigger=src.trigger; offset=src.offset;
 }
};
