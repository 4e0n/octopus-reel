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
#include <QDateTime>
#include "confparam.h"
#include "../common/logring.h"
#include "../common/rt_bootstrap.h"
#include "../common/timespec.h"

class CompThread : public QThread {
 Q_OBJECT
 public:
  explicit CompThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent),conf(c) {}

  void run() override {
#ifdef __linux__
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),2); // Pin tcpsend to core 2
   set_thread_rt(pthread_self(),SCHED_FIFO,60); // Give (lesser) RT priority
#endif 

   ts::deadline_t nextDeadline=ts::now();
   // optional: start a bit in the future so first iteration has time to fill backlog
   nextDeadline=ts::add_ns(nextDeadline,5*1000*1000); // +5ms

   const int N=conf->eegSamplesInTick;
   const quint64 target=quint64(2*N);

   // physics cadence
   const int64_t basePeriod_ns=(int64_t(N)*1000000000LL)/int64_t(conf->eegRate);

   // PI tuning (same “units” as before)
   const int64_t Kp_ns_per_sample=std::max<int64_t>(2000,1000000/target/2);
   const int64_t Ki_ns_per_sample=std::max<int64_t>(1,Kp_ns_per_sample/200);
   const int64_t corrClamp_ns=basePeriod_ns/4;
   const int64_t integClamp=10*target;

   int64_t integ=0;

   auto clamp_i64=[](int64_t v,int64_t lo,int64_t hi)->int64_t {
    return (v<lo)?lo:(v>hi)?hi:v;
   };

   const int eegCh=int(conf->chnCount-2);
   TcpSample tcpS(conf->ampCount,conf->physChnCount);
   TcpSamplePP tcpSPP(conf->ampCount,conf->physChnCount);
   while (!isInterruptionRequested() && !conf->compStop.load()) {
    ts::sleep_until_abs(nextDeadline);
    // Optional “late wake” resync (Linux only, since we have monotonic timespec)
    const auto nowts=ts::now();
    const int64_t nowNs=ts::timespec_to_ns(nowts);
    const int64_t nextNs=ts::timespec_to_ns(nextDeadline);
    if (nowNs>nextNs+5*basePeriod_ns) { nextDeadline=nowts; integ=0; }

    quint64 availOut=0;
    {
      QMutexLocker lk(&conf->mutex);
      availOut=conf->tcpBufHead-conf->tcpBufTail;
    }

    const int64_t err=int64_t(availOut)-int64_t(target);
    integ=clamp_i64(integ+err,-integClamp,integClamp);

    int64_t corrNs = Kp_ns_per_sample*err + Ki_ns_per_sample*integ;
    corrNs=clamp_i64(corrNs,-corrClamp_ns,corrClamp_ns);

    // NOTE: consumer PLL => period=base+corr (not base-corr)
    int64_t periodNs=basePeriod_ns-corrNs;
    periodNs=clamp_i64(periodNs,basePeriod_ns/2,basePeriod_ns*2);

    nextDeadline=ts::add_ns(nextDeadline,periodNs);

    // If output ring is *very* full, let it drain (skip compute this tick)
    if (availOut>conf->tcpBufSize-quint64(N)) { integ=0; continue; }

    QByteArray block; ConfParam::CompBlock cb;
    {
      QMutexLocker lk(&conf->compMutex);
      // Wait (bounded) for work. This prevents systematic “tick skip” drift.
      while (conf->compQueue.isEmpty() && !isInterruptionRequested() && !conf->compStop.load()) {
       conf->compReady.wait(&conf->compMutex,2);
      }
      if (isInterruptionRequested()||conf->compStop.load()) break;
      if (conf->compQueue.isEmpty()) { integ=0; continue; } // timed out
      //block = std::move(conf->compQueue.dequeue());
      cb=std::move(conf->compQueue.dequeue()); 
      // optional if you ever use compSpace:
      // conf->compSpace.wakeOne();
    }

    // block a.k.a. OUTER PAYLOAD: [one][one][one]... (fixed size sz)
    const int sz=conf->frameBytesIn;
    const int totalBytes=cb.buf.size()-cb.off;
    if (totalBytes<=0) continue;
    if (totalBytes%sz != 0) {
     qWarning() << "[PP:COMP] block remainder not multiple of frameBytesIn:"
                << "remBytes=" << totalBytes << "sz=" << sz;
     continue;
    }

    const char* base=cb.buf.constData()+cb.off;
    const int frames=totalBytes/sz;
    for (int i=0;i<frames;++i) {
     const char* one=base+i*sz;
     if (!tcpS.deserializeRaw(one,sz,conf->chnCount,conf->ampCount)) continue;

     // -------------------------------------------------------------------------- 
     // Compute path for ONE sample:
     // build TcpSamplePP,filters,push into ring,...
     tcpSPP.fromTcpSample(tcpS,conf->chnCount);

     // Compute OUTSIDE ring lock
     #pragma omp parallel for schedule(static) if(eegCh>=8)
     for (unsigned ampIdx=0;ampIdx<conf->ampCount;++ampIdx) {
      for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
       auto &notch=conf->filterListN[ampIdx][chnIdx];
       auto &bp=conf->filterListBP[ampIdx][chnIdx];
 
       const float x=tcpSPP.amp[ampIdx].data[chnIdx];
       float xN=notch.filterSample(x);
       float xBP=bp.filterSample(xN);

       tcpSPP.amp[ampIdx].dataN[chnIdx]=xN;
       tcpSPP.amp[ampIdx].dataBP[chnIdx]=xBP;

       const unsigned interMode=conf->chnInfo[chnIdx].interMode[ampIdx];
       if (interMode==2) {
        float sum=0.f;
        const int eSz=conf->chnInfo[chnIdx].interElec.size();
        for (int ei=0;ei<eSz;ei++)
         sum+=tcpSPP.amp[ampIdx].dataN[ conf->chnInfo[chnIdx].interElec[ei] ];
        tcpSPP.amp[ampIdx].dataN[chnIdx]=(eSz>0)?(sum/float(eSz)):0.f;
       } else if (interMode==0) {
        tcpSPP.amp[ampIdx].dataN[chnIdx]=0.f;
       }
      }
     }
     // -------------------------------------------------------------------------- 

     // Push into ring (critical section should be minimized)
     bool ringFull=false;
     {
       QMutexLocker locker(&conf->mutex);
       if ((conf->tcpBufHead-conf->tcpBufTail)>=conf->tcpBufSize) {
        ringFull=true;
       } else {
        auto &slot=conf->tcpBuffer[conf->tcpBufHead%conf->tcpBufSize];
        slot=tcpSPP;
        conf->tcpBufHead++;
        // wake, telemetry, etc...
       }
     }

     if (ringFull) {
      // Zero-copy requeue: same QByteArray, just advance offset
      cb.off+=i*sz; // bytes we already consumed
      if (cb.off<cb.buf.size()) {
       QMutexLocker lk(&conf->compMutex);
       while (conf->compQueue.size()>=conf->compQueueMax) conf->compQueue.dequeue();
       conf->compQueue.prepend(std::move(cb));
       conf->compReady.wakeOne();
      }
      integ=0;
      break;
     }
    }
   }
  }

 private:
  ConfParam *conf;
};
