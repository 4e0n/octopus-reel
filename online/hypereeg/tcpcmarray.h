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

#ifndef _TCPCMARRAY_H
#define _TCPCMARRAY_H

#include <vector>
#include <QByteArray>
#include <QDataStream>

struct TcpCMArray {
 std::vector<std::vector<uint8_t>> cmLevel;  // 0..255 -> red..yellow..green

 static constexpr quint32 MAGIC=0x434D4152;  // 'CMAR'

 void init(size_t ampCount,size_t chnCount) {
  cmLevel.resize(ampCount); for (auto &v:cmLevel) v.assign(chnCount,0);
 }

 TcpCMArray(size_t ampCount=0,size_t chnCount=0) { init(ampCount,chnCount); }

 QByteArray serialize() const {
  QByteArray ba; QDataStream out(&ba, QIODevice::WriteOnly);
  out.setByteOrder(QDataStream::LittleEndian);
  out << MAGIC << static_cast<quint32>(cmLevel.size());
  for (const auto& v:cmLevel) for (uint8_t c:v) out<<c;
  return ba;
 }

 bool deserialize(const QByteArray &ba,size_t chnCount) {
  QDataStream in(ba);
  in.setByteOrder(QDataStream::LittleEndian);
  quint32 magic,ampCount; in>>magic; if (magic!=MAGIC) return false;
  in >> ampCount; cmLevel.resize(ampCount);
  for (auto& amp:cmLevel) amp.resize(chnCount); 
  for (auto& amp:cmLevel) for (auto& chnCM:amp) in>>chnCM;
  return true;
 }
};

#endif
