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
#include <QtGlobal>
#include <functional>

class FramedStreamReader {
 public:
  // Feed newly received bytes, parse as many complete packets as possible.
  // For each complete payload, call onPayload(payloadPtr, payloadLen).
  void feed(const QByteArray &newBytes,const std::function<void(const char*,int)> &onPayload) {
   inbuf.append(newBytes);
   while (inbuf.size()>=4) {
    const uchar *p0=reinterpret_cast<const uchar*>(inbuf.constData());
    const quint32 Lo=(quint32)p0[0]
                    |((quint32)p0[1]<<8)
                    |((quint32)p0[2]<<16)
                    |((quint32)p0[3]<<24);
    if (Lo>4*1024*1024) { inbuf.clear(); return; } // corruption guard
    // Drop one byte and resync (or clear buffer; your choice)
    if (Lo>maxPayloadBytes) { inbuf.remove(0,1); continue; }
    if (inbuf.size()<4+(int)Lo) break;
    const char *pay=inbuf.constData()+4;
    const int payLen=(int)Lo;
    onPayload(pay,payLen);
    inbuf.remove(0,4+payLen);
   }
  }

  void clear() { inbuf.clear(); }
  void setMaxPayloadBytes(quint32 v) { maxPayloadBytes=v; }

 private:
  QByteArray inbuf;
  quint32 maxPayloadBytes=16*1024*1024; // 16MB safety default
};
