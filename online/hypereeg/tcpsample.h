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

#ifndef _TCPSAMPLE_H
#define _TCPSAMPLE_H

#include <vector>
#include <QByteArray>
#include <QDataStream>
#include "sample.h"

struct TcpSample {
 std::vector<Sample> amp; float audio48[48];
 quint64 offset=0; unsigned int trigger=0;
 quint64 timestampMs=0; // wall-clock at creation

 static constexpr quint32 MAGIC=0x54534D50; // 'TSMP'

 TcpSample(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t ampCount,size_t chnCount) {
  amp.resize(ampCount); for (Sample &s:amp) s.init(chnCount); trigger=0;
 }

 QByteArray serialize() const {
  QByteArray ba; QDataStream out(&ba, QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out << MAGIC;
  out << static_cast<quint64>(offset);
  out << static_cast<quint32>(trigger) << static_cast<quint32>(amp.size());
  out << static_cast<quint64>(timestampMs);
  for (const Sample& s:amp) {
   s.serialize(out);
  }
  for (int i=0;i<48;++i) {
   out << static_cast<float>(audio48[i]);
  }
  return ba;
 }

 bool deserialize(const QByteArray &ba,size_t chnCount) {
  QDataStream in(ba);
  in.setByteOrder(QDataStream::LittleEndian);
  quint32 magic=0,ampCount=0;
  in>>magic; if (magic!=MAGIC) return false;
  in >> offset;
  in >> trigger >> ampCount;
  in >> timestampMs;
  amp.resize(ampCount);
  for (Sample &s:amp) {
   s.init(chnCount); if (!s.deserialize(in,chnCount)) return false;
  }
  for (int i=0;i<48;++i) {
   float v=0.f;
   in >> v;
   audio48[i]=v;
  }
  return true;
 }
};

#endif
