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
#include <QByteArray>
#include <QDebug>

#ifdef __linux__
#include <pthread.h>
#endif

#include "confparam.h"
#include "../common/tcpsample.h"
#include "../common/timespec.h"
#include "../common/tcp_sendloop.h"

#ifdef EEMAGINE
#include "../common/rt_bootstrap.h"
#endif

class TcpThread:public QThread {
 Q_OBJECT
 public:
  explicit TcpThread(ConfParam *c,QObject *parent=nullptr):QThread(parent),conf(c) {
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&conf->tcpBuffer; conf->tcpBufTail=0;
  }
 protected:
  void run() override {
#ifdef __linux__
#ifdef EEMAGINE
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),2);
   set_thread_rt(pthread_self(),SCHED_FIFO,60);
#endif
#endif
   const int N=conf->eegSamplesInTick;
   const int64_t basePeriod_ns=(int64_t(N)*1000000000LL)/int64_t(conf->eegRate);

   const int sz=TcpSample::serializedSizeFor(conf->ampCount,conf->physChnCount);

   sendloop::Params p;
   p.N=N;
   p.basePeriod_ns=basePeriod_ns;
   p.target=uint64_t(2*N);
   p.minAvailToSend=uint64_t(N+N/2);

   p.Kp_ns_per_sample=std::max<int64_t>(2000,1000000LL/int64_t(p.target)/2);
   p.Ki_ns_per_sample=std::max<int64_t>(1,p.Kp_ns_per_sample/200);
   p.corrClamp_ns=basePeriod_ns/4;
   p.integClamp=10*int64_t(p.target);

   // Wait for at least one sample
   while (!isInterruptionRequested()) {
    QMutexLocker lk(&conf->mutex);
    if (conf->tcpBufHead-conf->tcpBufTail) break;
    lk.unlock();
    msleep(1);
   }

   auto shouldStop=[this]() -> bool { return isInterruptionRequested(); };

   auto availFn=[this]() -> uint64_t {
    QMutexLocker lk(&conf->mutex);
    return uint64_t(conf->tcpBufHead-conf->tcpBufTail);
   };

   auto serializeAndAdvance=[this,N,sz](char* dst) -> bool {
    QMutexLocker locker(&conf->mutex);
    const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
    if (avail<quint64(N)) return false;
    const quint64 tail0=conf->tcpBufTail;
    for (int i=0;i<N;++i) {
     const TcpSample& s=(*tcpBuffer)[(tail0+quint64(i))%tcpBufSize];
     const int wrote=s.serializeTo(dst+i*sz,sz);
     if (wrote!=sz) {
      qWarning() << "<TcpThread>[ACQ] serializeTo failed wrote=" << wrote << " expected=" << sz;
     }
    }
    conf->tcpBufTail+=quint64(N);
    return true;
   };

   auto sendFn=[this](const char* outData,int outLen) {
    emit sendPacketSignal(QByteArray(outData,outLen));
   };

   sendloop::DebugFn dbgFn=nullptr;

#ifdef PLL_VERBOSE
   dbgFn=[](uint64_t avail,uint64_t target,int64_t err,int64_t corrNs,int64_t periodNs,int64_t baseNs) {
    const double fill=(target>0) ? double(avail)/double(target):0.0;
    qInfo().noquote() << QString("<TcpPLL> avail=%1 target=%2 fill=%3 err=%4 corrNs=%5 periodNs=%6 baseNs=%7")
                         .arg(qulonglong(avail))
                         .arg(qulonglong(target))
                         .arg(fill,0,'f',2)
                         .arg(qlonglong(err))
                         .arg(qlonglong(corrNs))
                         .arg(qlonglong(periodNs))
                         .arg(qlonglong(baseNs));
   };
#endif
   sendloop::run_paced_sender(p,sz,shouldStop,availFn,serializeAndAdvance,sendFn,dbgFn);
  }

signals:
  void sendPacketSignal(const QByteArray &packet);

private:
  ConfParam *conf=nullptr;
  QVector<TcpSample> *tcpBuffer=nullptr;
  unsigned int tcpBufSize=0;
};
