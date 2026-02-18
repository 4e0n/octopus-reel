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
#include "../common/logring.h"

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
   //TcpSamplePP tmp(conf->ampCount,conf->physChnCount); tmp=(*tcpBuffer)[conf->tcpBufTail%tcpBufSize];
   const int sz=conf->frameBytesOut;

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
    quint64 hSnap=0,tSnap=0;
    {
     QMutexLocker locker(&conf->mutex);
     if ((conf->tcpBufHead-conf->tcpBufTail)<(quint64)N) continue;
     const quint64 tail=conf->tcpBufTail;
     for (int i=0;i<N;++i) eegChunk[i]=(*tcpBuffer)[(tail+i)%tcpBufSize];

     hSnap=conf->tcpBufHead; tSnap=conf->tcpBufTail;

     conf->tcpBufTail+=N;
     conf->spaceReady.wakeOne();
    }

    static quint64 lastH=0,lastT=0;
    static qint64  lastMs=0;
    log_ring_1hz("PP:SEND",hSnap,tSnap,lastH,lastT,lastMs);

    // Build ONE payload: N fixed-size frames back-to-back (no inner lengths)
    payload.resize(N*sz); // allocate exact size once
    char *dst=payload.data(); int off=0;
    for (int i=0;i<N;++i) {
     const QByteArray one=eegChunk[i].serialize(); // must be sz bytes
     // (optional safety)
     if (one.size()!=sz) {
      qWarning() << "[ACQ] serialize size changed!" << one.size() << "expected" << sz;
      continue;
     }
     memcpy(dst+off,one.constData(),sz);
     off+=sz;
    }

    // Outer frame=[uint32 Lo][payload]
    const quint32 Lo=(quint32)payload.size();
    out.resize(4+(int)Lo);
    out[0]=char((Lo    )&0xff);
    out[1]=char((Lo>> 8)&0xff);
    out[2]=char((Lo>>16)&0xff);
    out[3]=char((Lo>>24)&0xff);
    memcpy(out.data()+4,payload.constData(),Lo);

    emit sendPacketSignal(out);

    lastSend=now;
   }
  }

 signals:
  void sendPacketSignal(const QByteArray &packet);

 private:
  ConfParam *conf;

  QByteArray payload,out;

  TcpSamplePP tcpEEG;
  QVector<TcpSamplePP> *tcpBuffer; unsigned int tcpBufSize;

  std::vector<TcpSamplePP> eegChunk;
};
