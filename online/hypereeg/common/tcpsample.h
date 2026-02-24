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
#include "sample.h"
#include "le_helper.h"
#include "globals.h"

struct TcpSample {
 static constexpr quint32 MAGIC=0x54534D50; // 'TSMP'
 quint64 offset=0,timestampMs=0; // wall-clock at creation
 std::vector<Sample> amp;
 float audioN[AUDIO_N];
 unsigned int ampCount=0,chnCount=0;
 unsigned int trigger=0;
 quint32 userEvent=0;
 quint32 audioTrigEvent=0;
 int16_t audioNR[AUDIO_N];

 TcpSample(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t aCount,size_t cCount) {
  ampCount=(unsigned)aCount; chnCount=(unsigned)cCount; offset=0; timestampMs=0; trigger=0;
  amp.resize(ampCount); for (auto &s:amp) s.init(chnCount);
  for (size_t i=0;i<AUDIO_N;++i) { audioN[i]=0.f; audioNR[i]=0; }
 }

 void initSizeOnly(size_t aCount,size_t cCount) {
  ampCount=(unsigned)aCount; chnCount=(unsigned)cCount; amp.resize(ampCount);
  for (auto& s:amp) s.initSizeOnly(chnCount);
  // audioN contents to be overwritten later
  trigger=0; offset=0; timestampMs=0;
 }

 static inline int wireSizeBytes(int ampCount,int chnCount) {
  // header: MAGIC(4)+offset(8)+timestamp(8)+trigger(4)+ampCount(4)=28
  // amp: ampCount*chnCount*float(4)
  // audio: AUDIO_N*float(4)
  return 28+(ampCount*chnCount*4)+(AUDIO_N*4);
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
  // amps
  for (unsigned a=0;a<ampCount;++a) {
   // assume amp[a].data size == chnCount
   const auto& v=amp[a].data;
   // You can keep this assert during stabilization
   // Q_ASSERT(v.size() == chnCount);
   for (unsigned c=0;c<chnCount;++c) {
    wr_f32_le(p,v[c]);
   }
  }
  // audio
  for (int i=0;i<AUDIO_N;++i) wr_f32_le(p,audioN[i]);
  if (written) *written=need;
  return true;
 }

 QByteArray serializeFast() const {
  const int need=wireSizeBytes(int(ampCount),int(chnCount));
  QByteArray ba; ba.resize(need);
  int w=0; const bool ok=serializeRaw(ba.data(),ba.size(),&w);
  if (!ok || w!=need) return QByteArray();
  return ba;
 }

 QByteArray serialize() const { QByteArray ba;
  QDataStream out(&ba,QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);
  out << static_cast<quint32>(MAGIC);
  out << static_cast<quint64>(offset) << static_cast<quint64>(timestampMs);
  out << static_cast<quint32>(trigger) << static_cast<quint32>(amp.size());
  for (const Sample& s:amp) { s.serialize(out); }
  for (size_t audIndex=0;audIndex<AUDIO_N;audIndex++) { out << static_cast<float>(audioN[audIndex]); }
  return ba;
 }

 // Deterministic size for given shape (matches deserializeRaw layout)
 static inline int serializedSizeFor(size_t aCount,size_t cCount) {
  return 4            // MAGIC
       + 8+8          // offset, timestampMs
       + 4+4          // trigger, ampCount
       + int(aCount)*int(cCount)*4
       + AUDIO_N*4;
 }

 // Current object's size (uses current ampCount/chnCount)
 inline int serializedSize() const {
  // NOTE: amp.size() is the wire ampCount
  return serializedSizeFor(amp.size(),chnCount);
 }

 // Write into preallocated buffer. Returns bytes written, or 0 on error.
 int serializeTo(char* dst,int cap) const {
  if (!dst || cap<=0) return 0;
  const quint32 aCount=quint32(amp.size());
  const int need=serializedSizeFor(aCount,chnCount);
  if (cap<need) return 0;
  char* p=dst;
  wr_u32_le(p,MAGIC);
  wr_u64_le(p,quint64(offset));
  wr_u64_le(p,quint64(timestampMs));
  wr_u32_le(p,quint32(trigger));
  wr_u32_le(p,aCount);
  // amp floats (same order as Sample::serialize(QDataStream): for each Sample, for each float)
  for (quint32 a=0;a<aCount;++a) {
   // Be defensive if someone forgot initSizeOnly/init:
   const auto& v=amp[int(a)].data;
   const int want=int(chnCount);
   const int have=int(v.size());
   const int n=(have<want) ? have:want;
   for (int i=0;i<n;++i) wr_f32_le(p,v[size_t(i)]);
   for (int i=n;i<want;++i) wr_f32_le(p,0.0f); // pad if undersized
  }
  // audio
  for (int i=0;i<AUDIO_N;++i) wr_f32_le(p,audioN[i]);
  return int(p-dst); // should equal need
 }

 bool deserializeRaw(const char* data,int len,int cCount,quint32 expectedAmpCount=0) {
  if (!data || len<=0) return false;

  const char* p=data; const char* end=data+len;

  auto need=[&](int n) -> bool { return (p + n)<=end; };

  if (!need(4)) return false; // MAGIC
  const quint32 magic=rd_u32_le(p); p+=4; if (magic != MAGIC) return false;

  if (!need(8)) return false; // offset
  offset=rd_u64_le(p); p+=8;

  if (!need(8)) return false; // timestampMs
  timestampMs=rd_u64_le(p); p+=8;

  if (!need(4)) return false; // trigger
  trigger=rd_u32_le(p); p+=4;

  if (!need(4)) return false; // ampCount
  const quint32 aCount=rd_u32_le(p); p+=4;

  if (expectedAmpCount!=0 && aCount!=expectedAmpCount) return false;

  ampCount=aCount; chnCount=(unsigned)cCount; amp.clear(); amp.resize(ampCount);
  for (quint32 a=0;a<ampCount;++a) { amp[int(a)].init(chnCount);
   int consumed=0;
   if (!amp[int(a)].deserialize(p,int(end-p),chnCount,&consumed)) return false;
   p+=consumed;
  }

  for (size_t i=0;i<AUDIO_N;++i) { // audioN
   if (!need(4)) return false;
   audioN[i]=rd_f32_le(p); p+=4;
  }

  // Strict framing check (keep during stabilization)
  if (p!=end) {
   qWarning() << "[TcpSample] framing mismatch, remaining bytes =" << (end-p)
              << "len =" << len << "ampCount =" << ampCount << "chnCount =" << chnCount;
   return false;
  }

  return true;
 }

 bool deserialize(const QByteArray& ba,int chnCount,quint32 expectedAmpCount=0) {
  return deserializeRaw(ba.constData(),ba.size(),chnCount,expectedAmpCount);
 }
};
