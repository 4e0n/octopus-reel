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

 TcpSample(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t aCount,size_t cCount) {
  ampCount=(unsigned)aCount; chnCount=(unsigned)cCount; offset=0; timestampMs=0; trigger=0;
  amp.resize(ampCount); for (auto &s:amp) s.init(chnCount);
  for (size_t i=0;i<AUDIO_N;++i) audioN[i]=0.f;
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

 bool deserializeRaw(const char* data, int len, int cCount,
                    quint32 expectedAmpCount = 0) {
  if (!data || len <= 0) return false;

  const char* p   = data;
  const char* end = data + len;

  auto need = [&](int n) -> bool { return (p + n) <= end; };

  // MAGIC
  if (!need(4)) return false;
  const quint32 magic = rd_u32_le(p); p += 4;
  if (magic != MAGIC) return false;

  // offset
  if (!need(8)) return false;
  offset = rd_u64_le(p); p += 8;

  // timestampMs
  if (!need(8)) return false;
  timestampMs = rd_u64_le(p); p += 8;

  // trigger
  if (!need(4)) return false;
  trigger = rd_u32_le(p); p += 4;

  // ampCount
  if (!need(4)) return false;
  const quint32 aCount = rd_u32_le(p); p += 4;

  if (expectedAmpCount != 0 && aCount != expectedAmpCount)
    return false;

  ampCount=aCount;
  chnCount=(unsigned)cCount;
  amp.clear();
  // amps (each is Sample of chnCount floats)
  amp.resize(ampCount);
  for (quint32 a=0;a<ampCount;++a) {
    amp[int(a)].init(chnCount);
    int consumed = 0;
    if (!amp[int(a)].deserialize(p, int(end - p), chnCount, &consumed))
      return false;
    p += consumed;
  }

  // audioN (AUDIO_N floats)
  for (size_t i = 0; i < AUDIO_N; ++i) {
    if (!need(4)) return false;
    audioN[i] = rd_f32_le(p);
    p += 4;
  }

  // Strict framing check (HIGHLY recommended while stabilizing)
  if (p != end) {
   qWarning() << "[TcpSample] framing mismatch, remaining bytes =" << (end - p)
              << "len =" << len << "ampCount =" << ampCount << "chnCount =" << chnCount;
   return false;
  }

  return true;
 }

 bool deserialize(const QByteArray& ba,int chnCount,quint32 expectedAmpCount=0) {
  return deserializeRaw(ba.constData(),ba.size(),chnCount,expectedAmpCount);
 }
};
