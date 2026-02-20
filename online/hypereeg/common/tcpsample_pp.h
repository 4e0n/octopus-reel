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

#include <QByteArray>
#include <QDataStream>
#include <vector>
#include <cstring>
#include "sample_pp.h"
#include "tcpsample.h"
#include "le_helper.h"
#include "globals.h"

struct TcpSamplePP {
 static constexpr quint32 MAGIC=0x54534D50; // 'TSMP'
 quint64 offset=0,timestampMs=0; // wall-clock at creation
 std::vector<SamplePP> amp;
 float audioN[AUDIO_N];
 unsigned int ampCount=0,chnCount=0;
 unsigned int trigger=0;

 TcpSamplePP(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t aCount,size_t cCount) {
  ampCount=(unsigned)aCount; chnCount=(unsigned)cCount; offset=0; timestampMs=0; trigger=0;
  amp.resize(ampCount); for (auto &s:amp) s.init(chnCount);
  for (size_t i=0;i<AUDIO_N;++i) audioN[i]=0.f;
 }

 void initSizeOnly(size_t aCount,size_t cCount) {
  ampCount=(unsigned)aCount; chnCount=(unsigned)cCount; amp.resize(ampCount);
  for (auto& s:amp) s.initSizeOnly(chnCount);
  // audioN contents to be overwritten later
  trigger=0; offset=0; timestampMs=0;
 }
 
 static inline int serializedSizeFor(size_t aCount,size_t cCount) {
  // header: MAGIC(4)+offset(8)+timestamp(8)+trigger(4)+ampCount(4)=28
  // per-amp: (data+dataBP+dataN)=3*chnCount float32
  // audio: AUDIO_N float32
  return 28+int(aCount)*int(cCount)*3*4+AUDIO_N*4;
 }

 static inline int wireSizeBytes(int ampCount,int chnCount) {
  return serializedSizeFor(size_t(ampCount), size_t(chnCount));
 }

 bool serializeRaw(char* dst,int cap,int* written=nullptr) const {
  const int need=wireSizeBytes(int(ampCount),int(chnCount));
  if (!dst || cap<need) return false;
  char* p=dst;
  wr_u32_le(p,MAGIC);
  wr_u64_le(p,offset);
  wr_u64_le(p,timestampMs);
  wr_u32_le(p,quint32(trigger));
  wr_u32_le(p,quint32(ampCount));
  // amps: data, dataBP, dataN
  for (unsigned a=0;a<ampCount;++a) {
   const auto& v=amp[a].data;
   const auto& vBP=amp[a].dataBP;
   const auto& vN=amp[a].dataN;
   // Assumption: sizes are fixed/stable (preferred in runtime)
   // Keep asserts during stabilization:
   // Q_ASSERT(v.size()==chnCount);
   // Q_ASSERT(vBP.size()==chnCount);
   // Q_ASSERT(vN.size()==chnCount);
   for (unsigned c=0;c<chnCount;++c) wr_f32_le(p,v[c]);
   for (unsigned c=0;c<chnCount;++c) wr_f32_le(p,vBP[c]);
   for (unsigned c=0;c<chnCount;++c) wr_f32_le(p,vN[c]);
  }
  // audio
  for (int i=0;i<AUDIO_N;++i) wr_f32_le(p,audioN[i]);
  if (written) *written=need;
  return true;
 }

 QByteArray serializeFast() const {
  const int need=serializedSizeFor(size_t(ampCount),size_t(chnCount));
  QByteArray ba; ba.resize(need);
  const int w=serializeTo(ba.data(),ba.size());
  if (w!=need) return QByteArray();
  return ba;
 }

 QByteArray serialize() const { QByteArray ba;
  QDataStream out(&ba,QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);
  out << static_cast<quint32>(MAGIC);
  out << static_cast<quint64>(offset) << static_cast<quint64>(timestampMs);
  out << static_cast<quint32>(trigger) << static_cast<quint32>(amp.size());
  for (const SamplePP& s:amp) { s.serialize(out); }
  for (size_t audIndex=0;audIndex<AUDIO_N;audIndex++) { out << static_cast<float>(audioN[audIndex]); }
  return ba;
 }

 inline int serializedSize() const {
  return serializedSizeFor(amp.size(),chnCount);
 }

 int serializeTo(char* dst,int cap) const {
  if (!dst||cap<=0) return 0;

  const quint32 aCount=quint32(ampCount);
  Q_ASSERT(amp.size()==size_t(ampCount));
  const int need=serializedSizeFor(aCount,chnCount);
  if (cap<need) return 0;

  char* p=dst;
  wr_u32_le(p,MAGIC);
  wr_u64_le(p,quint64(offset));
  wr_u64_le(p,quint64(timestampMs));
  wr_u32_le(p,quint32(trigger));
  wr_u32_le(p,aCount);

  // amps: for each amp => data,dataBP,dataN
  for (quint32 a=0;a<aCount;++a) {
   const auto& v=amp[int(a)].data;
   const auto& vBP=amp[int(a)].dataBP;
   const auto& vN=amp[int(a)].dataN;

   const int want=int(chnCount);

   // defensive if someone forgot initSizeOnly (pad with zeros)
   const int n=(int(v.size())<want)     ? int(v.size())   : want;
   const int nBP=(int(vBP.size())<want) ? int(vBP.size()) : want;
   const int nN=(int(vN.size())<want)   ? int(vN.size())  : want;

   for (int i=0;i<n;++i) wr_f32_le(p,v[size_t(i)]);
   for (int i=n;i<want;++i) wr_f32_le(p,0.0f);

   for (int i=0;i<nBP;++i) wr_f32_le(p,vBP[size_t(i)]);
   for (int i=nBP;i<want;++i) wr_f32_le(p,0.0f);

   for (int i=0;i<nN;++i) wr_f32_le(p,vN[size_t(i)]);
   for (int i=nN;i<want;++i) wr_f32_le(p,0.0f);
  }

  for (int i=0;i<AUDIO_N;++i) wr_f32_le(p, audioN[i]);

  return int(p-dst); // should equal need
 }

 // Ensure internal storage exists and is correctly sized; not to be cleared every time.
 void ensureShape(size_t aCount,size_t cCount) {
  if (ampCount==aCount && chnCount==cCount && amp.size()==aCount) {
   if (!amp.empty() && amp[0].data.size()==cCount && amp[0].dataBP.size()==cCount && amp[0].dataN.size()==cCount) {
    return;
   }
  }

  ampCount=unsigned(aCount); chnCount=unsigned(cCount); amp.resize(ampCount);
  for (unsigned a=0;a<ampCount;++a) {
   // Prefer initSizeOnly style (no fill) if you add it.
   // With current SamplePP::init, this will fill zeros once on shape change only.
   amp[a].initSizeOnly(chnCount);
  }
 }

 bool deserializeRaw(const char* data,int len,int cCount,quint32 expectedAmpCount=0) {
  if (!data || len<=0 || cCount<=0) return false;
  const char* p=data; const char* end=data+len;
  auto need=[&](int n) -> bool { return (p+n)<=end; };
  if (!need(4)) return false; // MAGIC
  const quint32 magic=rd_u32_le(p); p+=4;
  if (magic!=MAGIC) return false;
  if (!need(8)) return false; // offset
  offset=rd_u64_le(p); p+=8;
  if (!need(8)) return false; // timestampMs
  timestampMs=rd_u64_le(p); p+=8;
  if (!need(4)) return false; // trigger
  trigger=rd_u32_le(p); p+=4;
  if (!need(4)) return false; // ampCount
  const quint32 aCount=rd_u32_le(p); p+=4;
  if (expectedAmpCount!=0 && aCount!=expectedAmpCount) return false;
  // Fast framing check: expected total size must match len
  const int expect=serializedSizeFor(size_t(aCount),size_t(cCount));
  if (len!=expect) {
   qWarning() << "[TcpSamplePP] framing size mismatch len=" << len << "expect=" << expect
              << "aCount=" << aCount << "cCount=" << cCount;
   return false;
  }
  // Ensure buffers exist (only realloc when shape changes)
  ensureShape(size_t(aCount),size_t(cCount));
  // Decode floats directly into existing vectors (no resizes)
  for (unsigned a=0;a<ampCount;++a) {
   auto &s=amp[a];
   float* d=s.data.data();
   float* dBP=s.dataBP.data();
   float* dN=s.dataN.data();
   for (unsigned c=0;c<chnCount;++c) { d[c]=rd_f32_le(p); p+=4; }
   for (unsigned c=0;c<chnCount;++c) { dBP[c]=rd_f32_le(p); p+=4; }
   for (unsigned c=0;c<chnCount;++c) { dN[c]=rd_f32_le(p); p+=4; }
  }
  for (int i=0;i<AUDIO_N;++i) { audioN[i]=rd_f32_le(p); p+=4; } // audioN
  // At this point p should equal end due to size check
  return true;
 }

 bool deserialize(const QByteArray& ba,int chnCount,quint32 expectedAmpCount=0) {
  return deserializeRaw(ba.constData(),ba.size(),chnCount,expectedAmpCount);
 }

 void fromTcpSample(const TcpSample &s,size_t cCount) {
  offset=s.offset; timestampMs=s.timestampMs; trigger=s.trigger;
  for (size_t i=0;i<AUDIO_N;++i) audioN[i]=s.audioN[i]; // Audio

  ampCount=unsigned(s.amp.size()); chnCount=unsigned(cCount);
  ensureShape(ampCount,chnCount);
  for (unsigned a=0;a<ampCount;++a) { // Copy raw EEG data
   auto &dst=amp[a].data;
   const auto &src=s.amp[a].data;
   Q_ASSERT(dst.size()==chnCount);
   Q_ASSERT(src.size()==chnCount);
   std::memcpy(dst.data(),src.data(),chnCount*sizeof(float));
   // *** do NOT clear dataN/dataBP here.
   // The filter loop will be overwriting for every channel/every sample.
  }
 }
};
