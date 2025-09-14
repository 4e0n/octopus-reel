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

#pragma once

#include <vector>
#include <QByteArray>
#include <QDataStream>
#include "sample.h"

struct TcpSample {
 static constexpr quint32 MAGIC=0x54534D50; // 'TSMP'
 quint64 offset=0,timestampMs=0; // wall-clock at creation
 std::vector<Sample> amp;
 float audio48[48];
 unsigned int trigger=0;

 TcpSample(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t ampCount,size_t chnCount) {
  amp.resize(ampCount); for (Sample &s:amp) s.init(chnCount); offset=0; timestampMs=0; trigger=0;
 }

 QByteArray serialize() const { QByteArray ba;
  QDataStream out(&ba,QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out << static_cast<quint32>(MAGIC);
  out << static_cast<quint64>(offset) << static_cast<quint64>(timestampMs);
  out << static_cast<quint32>(trigger) << static_cast<quint32>(amp.size());
  for (const Sample& s:amp) { s.serialize(out); }
  for (size_t audIndex=0;audIndex<48;audIndex++) { out << static_cast<float>(audio48[audIndex]); }
  return ba;
 }

 bool deserialize(const QByteArray &ba,size_t chnCount) {
  QDataStream in(ba);
  in.setByteOrder(QDataStream::LittleEndian);
  quint32 magic=0,ampCount=0;
  in>>magic; if (magic!=MAGIC) { qDebug() << "<TCPSample> MAGIC mismatch!!!"; return false; }
  in >> offset >> timestampMs >> trigger >> ampCount;
  amp.resize(ampCount);
  for (Sample &s:amp) { s.init(chnCount);
   if (!s.deserialize(in,chnCount)) { qDebug() << "<TCPSample> Sample desrialization error!!"; return false; }
  }
  for (size_t audIndex=0;audIndex<48;audIndex++) { float v=0.f;
   in >> v;
   audio48[audIndex]=v;
  }
  return true;
 }
};
