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
#include <atomic>
#ifdef __linux__
#include <pthread.h>
#endif
#include <cmath>
#include <chrono>
#include "confparam.h"
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/logring.h"
#ifdef EEMAGINE
#include "../common/rt_bootstrap.h"
#endif
#include "../common/timespec.h"

class TcpThread : public QThread {
 Q_OBJECT
 public:
  TcpThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c;
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&conf->tcpBuffer; conf->tcpBufTail=0;
  }

  void run() override {
#ifdef __linux__
#ifdef EEMAGINE
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),2); // Pin tcpsend to core 2
   set_thread_rt(pthread_self(),SCHED_FIFO,60); // Give (lesser) RT priority
#endif
#endif 

   ts::deadline_t nextDeadline=ts::now();
   // optional: start a bit in the future so first iteration has time to fill backlog
   nextDeadline=ts::add_ns(nextDeadline,5*1000*1000); // +5ms

   const int N=conf->eegSamplesInTick;
   const quint64 target=quint64(2*N);
   const quint64 minAvailToSend=target; // only send if we have at least target

   // Base cadence from physics (NOT from probe msec)
   const int64_t basePeriod_ns=(int64_t(N)*1000000000LL)/int64_t(conf->eegRate);

   const int sz=TcpSample::serializedSizeFor(conf->ampCount,conf->physChnCount);
   payload.resize(N*sz);
   const quint32 Lo=quint32(payload.size());
   out.resize(4+int(Lo));

   // --- PI tuning (start conservative) ---
   // Units: err is "samples". corrNs is "nanoseconds".
   // So Kp and Ki are "ns per sample" and "ns per sample per tick".
   const int64_t Kp_ns_per_sample=std::max<int64_t>(2000,1000000/target/2); // 10 µs per sample of error for N=50
   const int64_t Ki_ns_per_sample=std::max<int64_t>(1,Kp_ns_per_sample/200); // 0.05 µs per sample per tick
   const int64_t corrClamp_ns=basePeriod_ns/4; // max +/-25% correction
   const int64_t integClamp=10*target;         // samples (anti-windup)

   int64_t integ=0;

   auto clamp_i64=[](int64_t v,int64_t lo,int64_t hi) -> int64_t {
    return (v<lo) ? lo:(v>hi)?hi:v;
   };

   // Wait for at least one sample
   while (!isInterruptionRequested()) {
    QMutexLocker locker(&conf->mutex);
    if (conf->tcpBufHead-conf->tcpBufTail) break;
    locker.unlock();
    msleep(1);
   }

   while (!isInterruptionRequested()) {
    ts::sleep_until_abs(nextDeadline);
    // Optional “late wake” resync
    const auto nowts=ts::now();
    const int64_t nowNs=ts::timespec_to_ns(nowts);
    const int64_t nextNs=ts::timespec_to_ns(nextDeadline);
    // If we woke up VERY late, resync phase
    if (nowNs>nextNs+5*basePeriod_ns) { nextDeadline=nowts; integ=0; }
    //if (timespec_to_ns(nowts)>timespec_to_ns(nextDeadline)+5*basePeriod_ns) {
    // nextDeadline=nowts; // resync if we fell behind badly
    //}

    quint64 avail=0;
    {
      QMutexLocker locker(&conf->mutex);
      avail=conf->tcpBufHead-conf->tcpBufTail;
    }

    // PLL error
    const int64_t err=int64_t(avail)-int64_t(target);

    // PI update (anti-windup clamp)
    integ=clamp_i64(integ+err,-integClamp,integClamp);

    int64_t corrNs=Kp_ns_per_sample*err+Ki_ns_per_sample*integ;
    corrNs=clamp_i64(corrNs,-corrClamp_ns,corrClamp_ns);

    // schedule next deadline:
    // if err>0 (too much backlog), corrNs>0 => shorten period => earlier deadline
    int64_t periodNs=basePeriod_ns-corrNs;
    periodNs=clamp_i64(periodNs,basePeriod_ns/2,basePeriod_ns*2); // [0.5x .. 2x]
    nextDeadline=ts::add_ns(nextDeadline,periodNs);

    // Only integrate if we actually have some data (or only when avail>=N).
    //if (avail>=quint64(N)) { integ=clamp_i64(integ+err,-integClamp,integClamp); }
    // More conservative -- prevent wind-up while starved -- only send if enough samples exista 
    if (avail<minAvailToSend) { integ=0; continue; }
    {
      QMutexLocker locker(&conf->mutex);
      avail=conf->tcpBufHead-conf->tcpBufTail;
      if (avail<quint64(N)) { integ=0; continue; }
      const quint64 tail0=conf->tcpBufTail;
      char* dst=payload.data();
      for (int i=0;i<N;++i) {
       const TcpSample& s=(*tcpBuffer)[(tail0+quint64(i))%tcpBufSize];
       const int wrote=s.serializeTo(dst+i*sz,sz);
       if (wrote!=sz) {
        qWarning() << "[ACQ] serializeTo failed wrote=" << wrote << " expected=" << sz;
        // optional: std::memset(dst+i*sz,0,sz);
       }
      }
      conf->tcpBufTail+=quint64(N);
      static quint64 lastH=0,lastT=0; static qint64 lastMs=0;
      log_ring_1hz("ACQ:SEND",conf->tcpBufHead,conf->tcpBufTail,lastH,lastT,lastMs);
    } // unlock ASAP

    // Build outer frame (no mutex)
    out[0]=char((Lo    )&0xff);
    out[1]=char((Lo>> 8)&0xff);
    out[2]=char((Lo>>16)&0xff);
    out[3]=char((Lo>>24)&0xff);
    memcpy(out.data()+4,payload.constData(),Lo);

    emit sendPacketSignal(out);
   }
  }

 signals:
  void sendPacketSignal(const QByteArray &packet);

 private:
  ConfParam *conf;
  //TcpSample tcpEEG;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize;

  QByteArray payload,out;
};
