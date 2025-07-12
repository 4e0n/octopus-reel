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

#include <QByteArray>
#include <QDataStream>

#include "sample.h"

typedef struct _TcpSample {
 std::vector<Sample> amp;
 unsigned int trigger=0;

 //_TcpSample()=default;
 //_TcpSample(size_t ampCount=0,size_t chnCount=0) : amp(ampCount,Sample(chnCount)),trigger(0) {}
 _TcpSample(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 void init(size_t ampCount, size_t chnCount) {
  amp.resize(ampCount);
  for (Sample &s : amp) s.init(chnCount);
  trigger=0;
 }

 QByteArray serialize() const {
  QByteArray ba;
  QDataStream out(&ba, QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out << static_cast<quint32>(trigger);
  out << static_cast<quint32>(amp.size());
  for (const Sample& s : amp) {
   auto writeFloatVec=[&](const std::vector<float>& v) { for (float f:v) out<<f; };
   out << s.marker;
   writeFloatVec(s.data);
   writeFloatVec(s.dataF);
//   writeFloatVec(s.dataZ);
//   writeFloatVec(s.dataCAR);
//   writeFloatVec(s.dataCM);
  }
  return ba;
 }
} TcpSample;

#endif
