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
#include <cmath>
#include "globals.h"  // rd_u32_le()
#include "le_helper.h"  // rd_u32_le()

struct SamplePP {
 std::vector<float> data;    // Raw
 std::vector<float> dataBP;  // Bandpass (2-40 Hz)
 std::vector<float> dataN;   // Notch-indicator / notch-filtered
#ifdef EEGBANDSCOMP
 std::vector<float> dataD; // [2-4]Hz IIR filtered Delta data.
 std::vector<float> dataT; // [4-8]Hz IIR filtered Theta data.
 std::vector<float> dataA; // [8-14]Hz IIR filtered Alpha data.
 std::vector<float> dataB; // [14-28]Hz IIR filtered Beta data.
 std::vector<float> dataG; // [28-40]Hz IIR filtered Gamma data.
			   //
 std::vector<float> gfp; // [6]:BP,D,T,A,B,G
 std::vector<float> gfpL;
 std::vector<float> gfpR;
#endif
 unsigned int trigger=0, offset=0; // NOT on wire

 //std::vector<float> meta;

 void init(size_t chnCount) {
  trigger=0; offset=0;
  data.assign(chnCount,0.0f);
  dataBP.assign(chnCount,0.0f);
  dataN.assign(chnCount,0.0f);
#ifdef EEGBANDSCOMP
  dataD.assign(chnCount,0.0f);
  dataT.assign(chnCount,0.0f); dataA.assign(chnCount,0.0f); dataB.assign(chnCount,0.0f);
  dataG.assign(chnCount,0.0f);

  gfp.assign(6,0.0f); gfpL.assign(6,0.0f); gfpR.assign(6,0.0f);
#endif
 }

 void initSizeOnly(size_t chnCount) {
  trigger=0; offset=0;
  data.resize(chnCount);
  dataBP.resize(chnCount); dataN.resize(chnCount); 
#ifdef EEGBANDSCOMP
  dataD.resize(chnCount,0.0f);
  dataT.resize(chnCount,0.0f); dataA.resize(chnCount,0.0f); dataB.resize(chnCount,0.0f);
  dataG.resize(chnCount,0.0f);

  gfp.resize(GFP_N); gfpL.resize(GFP_N); gfpR.resize(GFP_N);
#endif
 }

 SamplePP(size_t chnCount=0) { init(chnCount); }

 // Wire format: data, dataBP, dataN (all float32 LE)
 void serialize(QDataStream &out) const {
  for (float f:data)   out<<f;
  for (float f:dataBP) out<<f;
  for (float f:dataN)  out<<f;
#ifdef EEGBANDSCOMP
  for (float f:dataD) out<<f;
  for (float f:dataT) out<<f;
  for (float f:dataA) out<<f;
  for (float f:dataB) out<<f;
  for (float f:dataG) out<<f;

  for (float f:gfp)  out<<f;
  for (float f:gfpL) out<<f;
  for (float f:gfpR) out<<f;
#endif
 }

 bool deserialize(QDataStream &in, size_t chnCount) {
  data.resize(chnCount); dataBP.resize(chnCount); dataN.resize(chnCount);
#ifdef EEGBANDSCOMP
  dataD.resize(chnCount);
  dataT.resize(chnCount); dataA.resize(chnCount); dataB.resize(chnCount);
  dataG.resize(chnCount);

  gfp.resize(GFP_N); gfpL.resize(GFP_N); gfpR.resize(GFP_N);
#endif

  for (float &f:data)   in>>f;
  for (float &f:dataBP) in>>f;
  for (float &f:dataN)  in>>f;
#ifdef EEGBANDSCOMP
  for (float &f:dataD)  in>>f;
  for (float &f:dataT)  in>>f;
  for (float &f:dataA)  in>>f;
  for (float &f:dataB)  in>>f;
  for (float &f:dataG)  in>>f;

  gfp.resize(GFP_N); gfpL.resize(GFP_N); gfpR.resize(GFP_N);
  for (float &f:gfp)  in>>f;
  for (float &f:gfpL) in>>f;
  for (float &f:gfpR) in>>f;
#endif

  return true;
 }

 // pointer-based deserialize
 bool deserialize(const char* src,int len,int chnCount,int* consumed) {
  if (!src || len<0 || chnCount<0) return false;

#ifdef EEGBANDSCOMP
  const int vecCount=8;
#else
  const int vecCount=3;
#endif
  const int needBytes=int((size_t(chnCount)*size_t(vecCount)+size_t(3*GFP_N))*sizeof(float));

  //const int needBytes=chnCount*3*4; // data+dataBP+dataN
  //const int needBytes=chnCount*8*4; // data+dataBP+dataN+dataD+dataT+...
  if (len<needBytes) return false;

  data.resize(chnCount); dataBP.resize(chnCount); dataN.resize(chnCount);
#ifdef EEGBANDSCOMP
  dataD.resize(chnCount);
  dataT.resize(chnCount); dataA.resize(chnCount); dataB.resize(chnCount);
  dataG.resize(chnCount);
#endif

  const char* p=src;

  auto rd_f32=[&](float &dst) { quint32 w=rd_u32_le(p); std::memcpy(&dst,&w,4); p+=4; };

  for (int i=0;i<chnCount;++i) rd_f32(data[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataBP[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataN[i]);
#ifdef EEGBANDSCOMP
  for (int i=0;i<chnCount;++i) rd_f32(dataD[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataT[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataA[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataB[i]);
  for (int i=0;i<chnCount;++i) rd_f32(dataG[i]);

  for (int i=0;i<GFP_N;++i) rd_f32(gfp[i]);
  for (int i=0;i<GFP_N;++i) rd_f32(gfpL[i]);
  for (int i=0;i<GFP_N;++i) rd_f32(gfpR[i]);
#endif

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
#ifdef EEGBANDSCOMP
  Q_ASSERT(dataD.size()==src.dataD.size());
  Q_ASSERT(dataT.size()==src.dataT.size());
  Q_ASSERT(dataA.size()==src.dataA.size());
  Q_ASSERT(dataB.size()==src.dataB.size());
  Q_ASSERT(dataG.size()==src.dataG.size());
  std::memcpy(dataD.data(),src.dataD.data(),src.dataD.size()*sizeof(float));
  std::memcpy(dataT.data(),src.dataT.data(),src.dataT.size()*sizeof(float));
  std::memcpy(dataA.data(),src.dataA.data(),src.dataA.size()*sizeof(float));
  std::memcpy(dataB.data(),src.dataB.data(),src.dataB.size()*sizeof(float));
  std::memcpy(dataG.data(),src.dataG.data(),src.dataG.size()*sizeof(float));

  Q_ASSERT(gfp.size()==src.gfp.size()); Q_ASSERT(gfpL.size()==src.gfpL.size()); Q_ASSERT(gfpR.size()==src.gfpR.size());
  std::memcpy(gfp.data(),src.gfp.data(),src.gfp.size()*sizeof(float));
  std::memcpy(gfpL.data(),src.gfpL.data(),src.gfpL.size()*sizeof(float));
  std::memcpy(gfpR.data(),src.gfpR.data(),src.gfpR.size()*sizeof(float));
#endif

  trigger=src.trigger; offset=src.offset;
 }

 enum GfpBandIndex { GFP_BP=0,GFP_D=1,GFP_T=2,GFP_A=3,GFP_B=4,GFP_G=5,GFP_N=6 };

 static inline float computeGfpFromVector(const std::vector<float>& v,const std::vector<int>& indices) {
  double sum=0.0,sumsq=0.0; unsigned int n=0;

  for (int idx:indices) { const float x=v[size_t(idx)]; sum+=x; sumsq+=double(x)*double(x); ++n; }
  if (n<2) return 0.0f;

  const double mean=sum/double(n); double var=(sumsq/double(n))-mean*mean;
  if (var<0.0) var=0.0;

  return float(std::sqrt(var));
 }

 void computeGFPs(const std::vector<int>& allIdx,const std::vector<int>& leftIdx,const std::vector<int>& rightIdx) {
  gfp.resize(GFP_N); gfpL.resize(GFP_N); gfpR.resize(GFP_N);

  gfp[GFP_BP]=computeGfpFromVector(dataBP,allIdx);
  gfpL[GFP_BP]=computeGfpFromVector(dataBP,leftIdx); gfpR[GFP_BP]=computeGfpFromVector(dataBP,rightIdx);

#ifdef EEGBANDSCOMP
  gfp[GFP_D]=computeGfpFromVector(dataD,allIdx);
  gfpL[GFP_D]=computeGfpFromVector(dataD,leftIdx); gfpR[GFP_D]=computeGfpFromVector(dataD,rightIdx);
  gfp[GFP_T]=computeGfpFromVector(dataT,allIdx);
  gfpL[GFP_T]=computeGfpFromVector(dataT,leftIdx); gfpR[GFP_T]=computeGfpFromVector(dataT,rightIdx);
  gfp[GFP_A]=computeGfpFromVector(dataA,allIdx);
  gfpL[GFP_A]=computeGfpFromVector(dataA,leftIdx); gfpR[GFP_A]=computeGfpFromVector(dataA,rightIdx);
  gfp[GFP_B]=computeGfpFromVector(dataB,allIdx);
  gfpL[GFP_B]=computeGfpFromVector(dataB,leftIdx); gfpR[GFP_B]=computeGfpFromVector(dataB,rightIdx);
  gfp[GFP_G]=computeGfpFromVector(dataG,allIdx);
  gfpL[GFP_G]=computeGfpFromVector(dataG,leftIdx); gfpR[GFP_G]=computeGfpFromVector(dataG,rightIdx);
#else
  for (int i=GFP_D;i<GFP_N;++i) { gfp[i]=0.0f; gfpL[i]=0.0f; gfpR[i]=0.0f; }
#endif
 }

};
