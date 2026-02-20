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
#include <QMutexLocker>
#include <QDebug>
#include <atomic>
#ifdef __linux__
#include <pthread.h>
#endif
#include "confparam.h"
#include "../common/logring.h"
#include "../common/rt_bootstrap.h"
#include "../common/timespec.h"

class TcpThread : public QThread {
 Q_OBJECT
 public:
  TcpThread(ConfParam *c,QObject *parent=nullptr):QThread(parent),conf(c) {
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&conf->tcpBuffer; conf->tcpBufTail=0;
  }

  void run() override {
#ifdef __linux__
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),3); // choose a different core than CompThread if possible
   set_thread_rt(pthread_self(),SCHED_FIFO,55);
#endif

   ts::deadline_t nextDeadline=ts::now();
   nextDeadline=ts::add_ns(nextDeadline,5*1000*1000); // +5ms

   const int N=conf->eegSamplesInTick;
   const quint64 target=quint64(2*N);
   const quint64 minAvailToSend=target;

   const int64_t basePeriod_ns=(int64_t(N)*1000000000LL)/int64_t(conf->eegRate); // physics cadence

   const int sz=conf->frameBytesOut;
   if (sz<=0) {
    qWarning() << "[PP:SEND] frameBytesOut not set";
    return;
   }

   const int expectSz=TcpSamplePP::serializedSizeFor(conf->ampCount,conf->physChnCount);
   if (sz!=expectSz) {
    qWarning() << "[PP:SEND] frameBytesOut mismatch sz=" << sz << "expect=" << expectSz;
    // You can choose: return; or keep going if you want “best effort”.
    // I recommend return during stabilization.
    return;
   }

   payload.resize(N*sz);
   const quint32 Lo=quint32(payload.size());
   out.resize(4+int(Lo));

   out[0]=char((Lo    )&0xff);
   out[1]=char((Lo>> 8)&0xff);
   out[2]=char((Lo>>16)&0xff);
   out[3]=char((Lo>>24)&0xff);

   // PI tuning (same convention as node-acq)
   const int64_t Kp_ns_per_sample=std::max<int64_t>(2000,1000000/target/2);
   const int64_t Ki_ns_per_sample=std::max<int64_t>(1,Kp_ns_per_sample/200);
   const int64_t corrClamp_ns=basePeriod_ns/4;
   const int64_t integClamp=10*target;

   auto clamp_i64=[](int64_t v,int64_t lo,int64_t hi)->int64_t { return (v<lo)?lo:(v>hi)?hi:v; };

   int64_t integ=0;

   // Wait for at least one sample
   while (!isInterruptionRequested()) {
    QMutexLocker lk(&conf->mutex);
    if (conf->tcpBufHead-conf->tcpBufTail) break;
    lk.unlock();
    msleep(1);
   }

   while (!isInterruptionRequested()) {
    ts::sleep_until_abs(nextDeadline);
    // Optional “late wake” resync
    const auto nowts=ts::now();
    const int64_t nowNs=ts::timespec_to_ns(nowts);
    const int64_t nextNs=ts::timespec_to_ns(nextDeadline);
    if (nowNs>nextNs+5*basePeriod_ns) { nextDeadline=nowts; integ=0; }

    quint64 avail=0;
    {
      QMutexLocker lk(&conf->mutex);
      avail=conf->tcpBufHead-conf->tcpBufTail;
    }

    const int64_t err=int64_t(avail)-int64_t(target);
    integ=clamp_i64(integ+err,-integClamp,integClamp);

    int64_t corrNs=Kp_ns_per_sample*err+Ki_ns_per_sample*integ;
    corrNs=clamp_i64(corrNs,-corrClamp_ns,corrClamp_ns);

    // sender => shorten period when backlog grows
    int64_t periodNs=basePeriod_ns-corrNs;
    periodNs=clamp_i64(periodNs,basePeriod_ns/2,basePeriod_ns*2);

    nextDeadline=ts::add_ns(nextDeadline,periodNs);

    if (avail<minAvailToSend) { integ=0; continue; }

    quint64 tail0=0,headSnap=0,tailAfter=0;
    {
      QMutexLocker lk(&conf->mutex);
      avail=conf->tcpBufHead-conf->tcpBufTail;
      if (avail<quint64(N)) { integ=0; continue; }
      tail0=conf->tcpBufTail;     // tail BEFORE
      headSnap=conf->tcpBufHead;
      tailAfter=tail0+quint64(N); // tail AFTER
      conf->tcpBufTail=tailAfter;
      conf->spaceReady.wakeOne();
    }

    // Serialize without holding conf->mutex
    char* dst=payload.data();
    for (int i=0;i<N;++i) {
     const TcpSamplePP &s=(*tcpBuffer)[(tail0+quint64(i))%tcpBufSize];
     const int wrote=s.serializeTo(dst+i*sz,sz);
     if (wrote!=sz) {
      qWarning() << "[PP:SEND] serializeTo failed wrote=" << wrote << " expected=" << sz;
     }
    }

    // Outer frame=[uint32 Lo][payload]
    memcpy(out.data()+4,payload.constData(), Lo);

    static quint64 lastH=0,lastT=0; static qint64 lastMs=0;
    log_ring_1hz("PP:SEND",headSnap,tailAfter,lastH,lastT,lastMs);

    emit sendPacketSignal(out);
   }
  }

 signals:
  void sendPacketSignal(const QByteArray &packet);

 private:
  ConfParam *conf=nullptr;
  QVector<TcpSamplePP> *tcpBuffer=nullptr;
  unsigned int tcpBufSize=0;

  QByteArray payload,out;
};
