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

#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QDateTime>
#include <cmath>
#include "confparam.h"
#include "../common/globals.h"
#include "../common/tcpsample_pp.h"

class TcpThread : public QThread {
 Q_OBJECT
 public:
  TcpThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c;
   tcpEEG=TcpSamplePP(conf->ampCount,conf->physChnCount);
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&conf->tcpBuffer;
   eegChunk.resize(conf->eegSamplesInTick);
   conf->tcpBufTail=0;
  }

  void run() override {
   const int N=conf->eegSamplesInTick;
   const quint64 slack=2*N; // keep tail ~2*N behind head when backlog exists
   const quint64 wakeMs=conf->eegProbeMsecs;

   while (!(conf->tcpBufHead-conf->tcpBufTail)) msleep(1); // Wait/ensure for at least one available sample

   // determine fixed serialized size once
   TcpSamplePP tmp(conf->ampCount,conf->physChnCount); tmp=(*tcpBuffer)[conf->tcpBufTail%tcpBufSize];
   const int sz=tmp.serialize().size();

   QByteArray packet; packet.reserve(4+N*sz);

   quint64 lastSend=QDateTime::currentMSecsSinceEpoch();

   while(!isInterruptionRequested()) { quint64 avail;
    {
     QMutexLocker locker(&conf->mutex);
     avail=conf->tcpBufHead-conf->tcpBufTail;
    }
    if (avail<slack) { msleep(1); continue; } // Not following from distant enough

    const quint64 now=QDateTime::currentMSecsSinceEpoch();
    if ((now-lastSend)<wakeMs) { msleep(1); continue; }

    // Copy N samples, advance tail
    {
     QMutexLocker locker(&conf->mutex);
     if ((conf->tcpBufHead-conf->tcpBufTail)<(quint64)N) continue;
     const quint64 tail=conf->tcpBufTail;
     for (int i=0;i<N;++i) eegChunk[i]=(*tcpBuffer)[(tail+i)%tcpBufSize];

//static qint64 t1=0;
//qint64 noww=QDateTime::currentMSecsSinceEpoch();
//if (noww-t1>1000) { t1=noww;
//  qDebug() << "[CONS] head" << conf->tcpBufHead
//           << "tail0" << tail
//           << "avail" << (conf->tcpEEGHead - conf->tcpBufTail)
//           << "off0"  << eegChunk[0].offset
//           << "eeg0"  << eegChunk[0].amp[0].dataBP[0]
//           << "aud0"  << eegChunk[0].audioN[0];
//}

     conf->tcpBufTail+=N;
    }

//static qint64 t1=0;
//const quint64 noww=QDateTime::currentMSecsSinceEpoch();
//if (noww-t1>1000) { t1=noww;
//  const auto &s = eegChunk[0];
//  qDebug() << "[CONS] off" << s.offset << "mag" << QString::number(s.MAGIC,16)
//           << "eeg0" << s.amp[0].dataBP[0]
//           << "aud0" << s.audioN[0];
//}

    for (int i=0;i<N;i++) {
     const QByteArray one=eegChunk[i].serialize();
     const quint32 L=(quint32)one.size();

     QByteArray packet;
     packet.reserve(4+one.size());
     packet.append(char((L    )&0xff));
     packet.append(char((L>> 8)&0xff));
     packet.append(char((L>>16)&0xff));
     packet.append(char((L>>24)&0xff));
     packet.append(one);
     emit sendPacketSignal(packet);
    }

    lastSend=now;
   }
  }

 signals:
  void sendPacketSignal(const QByteArray &packet);

 private:
  ConfParam *conf;

  TcpSamplePP tcpEEG;
  QVector<TcpSamplePP> *tcpBuffer; unsigned int tcpBufSize;

  std::vector<TcpSamplePP> eegChunk;
};
