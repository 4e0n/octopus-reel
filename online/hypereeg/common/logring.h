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

#include <QDateTime>

static inline void log_ring_1hz(const char* tag,quint64 head,quint64 tail,
                                quint64& lastHead,quint64& lastTail,qint64& lastMs) {
 const qint64 now=QDateTime::currentMSecsSinceEpoch();
 if (lastMs==0) lastMs=now;
 if (now-lastMs>=1000) {
  const quint64 fill=head-tail;
  const quint64 dH=head-lastHead; // push in last second
  const quint64 dT=tail-lastTail; // pop in last second
  qInfo().noquote() << QString("[%1] fill=%2  +%3/s  -%4/s  head=%5 tail=%6")
                        .arg(tag)
                        .arg(fill)
                        .arg(dH)
                        .arg(dT)
                        .arg((qulonglong)head)
                        .arg((qulonglong)tail);
  lastHead=head;
  lastTail=tail;
  lastMs=now;
 }
}
