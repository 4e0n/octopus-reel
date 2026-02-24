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

#include <QtGlobal>
#include <QDateTime>
#include <limits>

struct Health1Hz {
 qint64 lastMs=0;

 // fill stats
 quint64 fillMin=std::numeric_limits<quint64>::max();
 quint64 fillMax=0;
 quint64 fillSum=0;
 quint64 fillN  =0;

 // optional counters
 quint64 lateWake=0; // how many times we resynced / detected late
 quint64 outerRx =0; // RX frames seen in last second
 quint64 drop    =0; // drop count in last second
 quint64 maxCompQ=0; // max compQueue observed in last second

 void observeFill(quint64 fill) {
  if (fill<fillMin) fillMin=fill;
  if (fill>fillMax) fillMax=fill;
  fillSum+=fill;
  fillN++;
 }
 void observeCompQ(quint64 q) {
  if (q>maxCompQ) maxCompQ=q;
 }

 template<class LoggerFn>
  void maybeLog(const char* tag,LoggerFn logger) {
   const qint64 ms=QDateTime::currentMSecsSinceEpoch();
   if (lastMs==0) lastMs=ms;
   if (ms-lastMs<1000) return;
   const double fillAvg=(fillN>0) ? double(fillSum)/double(fillN):0.0;
   logger(tag,
          fillMin==std::numeric_limits<quint64>::max() ? 0:fillMin,
          fillAvg,
          fillMax,
          lateWake,
          outerRx,
          drop,
          maxCompQ);
   // reset window
   lastMs=ms;
   fillMin=std::numeric_limits<quint64>::max();
   fillMax=0;
   fillSum=0;
   fillN=0;
   lateWake=0;
   outerRx=0;
   drop=0;
   maxCompQ=0;
  }
};
