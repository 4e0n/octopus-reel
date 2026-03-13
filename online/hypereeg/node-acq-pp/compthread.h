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
#include <QElapsedTimer>
#include "confparam.h"
#include "../common/logring.h"
#include "../common/rt_bootstrap.h"

#ifdef _OPENMP
#include <omp.h>
#endif

class CompThread : public QThread {
 Q_OBJECT
 public:
  explicit CompThread(ConfParam *c, QObject *parent=nullptr):QThread(parent),conf(c) {}

  void run() override {
#ifdef __linux__
   lock_memory_or_warn();
   // N150 = 4 cores. If RX is on 0 and SEND on 1, park COMP on 2 or 3.
   // Pick ONE core for the QThread itself (OMP workers are separate threads).
   pin_thread_to_cpu(pthread_self(),2);
   set_thread_rt(pthread_self(),SCHED_FIFO,70); // let COMP win if needed
#endif

#ifdef _OPENMP
   omp_set_dynamic(0);
   // On a 4-core machine, start conservative
   // 2 is usually safer; 3 really dedicate cores to compute
   omp_set_num_threads(2);
#endif

   const int eegCh=int(conf->chnCount-2);
   TcpSample tcpS(conf->ampCount,conf->physChnCount);
   TcpSamplePP tcpSPP(conf->ampCount,conf->physChnCount);

   // Simple guard: when output ring is "near full", pause briefly.
   // This prevents burning CPU while SEND drains.
   const quint64 outNearFullThreshold=(conf->tcpBufSize > quint64(conf->eegSamplesInTick))
        ? (conf->tcpBufSize-quint64(conf->eegSamplesInTick)):(conf->tcpBufSize);

   while (!isInterruptionRequested() && !conf->compStop.load()) {
    // 1) Wait for work (blocking)
    ConfParam::CompBlock cb;
    {
      QMutexLocker lk(&conf->compMutex);
      while (conf->compQueue.isEmpty() && !isInterruptionRequested() && !conf->compStop.load()) {
       conf->compReady.wait(&conf->compMutex); // no timeout = no polling
      }
      if (isInterruptionRequested()||conf->compStop.load()) break;
      if (conf->compQueue.isEmpty()) continue; // defensive
      cb=std::move(conf->compQueue.dequeue());
      conf->compSpace.wakeOne();
      // If compSpace is maintained somewhere, it is awaken here.
    }
    // 2) Validate block framing
    const int sz=conf->frameBytesIn;
    const int totalBytes=cb.buf.size()-cb.off;
    if (totalBytes<=0) continue;
    if ((totalBytes%sz)!=0) {
     qWarning() << "[PP:COMP] block remainder not multiple of frameBytesIn:"
                << "remBytes=" << totalBytes << "sz=" << sz;
     continue;
    }
    const char* base=cb.buf.constData()+cb.off;
    const int frames=totalBytes/sz;
    // 3) Process frames; if output ring is blocked, apply backpressure
    for (int i=0;i<frames;++i) {
     // --- backpressure before heavy compute ---
     // If SEND is falling behind, don't keep computing into a full ring.
     // This keeps compQueue from exploding due to ringFull requeue loops.
     for (;;) {
      quint64 availOut=0;
      {
        QMutexLocker lk(&conf->mutex);
        availOut=conf->tcpBufHead-conf->tcpBufTail;
      }
      if (availOut<outNearFullThreshold) break;
      // Near full: let SEND catch up a bit.
      // 1ms is small but stops hot spinning.
      QThread::usleep(1000);
      if (isInterruptionRequested()||conf->compStop.load()) break;
     }
     if (isInterruptionRequested()||conf->compStop.load()) break;
     const char* one=base+i*sz;
     if (!tcpS.deserializeRaw(one,sz,conf->chnCount,conf->ampCount)) continue;
     // Compute for ONE sample (outside ring lock)
     tcpSPP.fromTcpSample(tcpS,conf->chnCount);

     // ==============================================================================================================
     // ==============================================================================================================
     // NODE-COMP STYLE GENERIC COMPUTATION TO BE IMPLEMENTED HERE!!
     // ==============================================================================================================
     // ==============================================================================================================

     // OpenMP: parallelize over amps only (lower overhead than nested)
     // If ampCount is 1, omp adds overhead -> guard it.
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if(conf->ampCount>=2)
#endif

     // Apply pre-filter before neighbor-interpolation
     for (int ampIdx=0;ampIdx<int(conf->ampCount);++ampIdx) for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
      const float x=tcpSPP.amp[ampIdx].data[chnIdx];
      // notch-filtered version is master
      auto &notch=conf->filterListN[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataN[chnIdx]=notch.filterSample(x);
     }

     // Channels' interpolation on Notch filtered version
     for (int ampIdx=0;ampIdx<int(conf->ampCount);++ampIdx) {
      for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
       const unsigned interMode=conf->chnInfo[chnIdx].interMode[ampIdx];
       float xN=tcpSPP.amp[ampIdx].dataN[chnIdx];
       // Default is 1
       if (interMode==2) { // INTERPOLATE
        const int eSz=conf->chnInfo[chnIdx].interElec.size();
	if (eSz>0) {
         float sum=0.f; for (int ei=0;ei<eSz;++ei) sum+=tcpSPP.amp[ampIdx].dataN[ conf->chnInfo[chnIdx].interElec[ei] ];
	 xN=sum/float(eSz);
         //tcpSPP.amp[ampIdx].dataN[chnIdx]=sum/float(eSz);
        }
       } else if (interMode==0) { // OFF
        xN=0.f;
        //tcpSPP.amp[ampIdx].dataN[chnIdx]=0.f;
       }
       // Now filter further the interpolated version
       auto &bp=conf->filterListBP[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataBP[chnIdx]=bp.filterSample(xN);
#ifdef EEGBANDSCOMP
       auto &delta=conf->filterListD[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataD[chnIdx]=delta.filterSample(xN);
       auto &theta=conf->filterListT[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataT[chnIdx]=theta.filterSample(xN);
       auto &alpha=conf->filterListA[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataA[chnIdx]=alpha.filterSample(xN);
       auto &beta=conf->filterListB[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataB[chnIdx]=beta.filterSample(xN);
       auto &gamma=conf->filterListG[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataG[chnIdx]=gamma.filterSample(xN);
#endif
      }
     }

     // ==============================================================================================================
     // ==============================================================================================================
     // END OF COMPUTATION
     // ==============================================================================================================
     // ==============================================================================================================

     // Push ONE sample into ring
     bool ringFull=false;
     {
       QMutexLocker locker(&conf->mutex);
       if ((conf->tcpBufHead-conf->tcpBufTail) >= conf->tcpBufSize) {
        ringFull=true;
       } else {
        auto &slot=conf->tcpBuffer[conf->tcpBufHead%conf->tcpBufSize];
        slot=tcpSPP;
        conf->tcpBufHead++;
       }
     }
     if (ringFull) {
      // Requeue remaining bytes (including current frame and beyond)
      cb.off+=i*sz;
      if (cb.off<cb.buf.size()) {
       QMutexLocker lk(&conf->compMutex);
       while (conf->compQueue.size()>=conf->compQueueMax) conf->compQueue.dequeue();
       conf->compQueue.prepend(std::move(cb));
       conf->compReady.wakeOne();
      }
      break; // leave frame loop, go wait/drain again
     }
    }
   }
  }
 private:
  ConfParam *conf;
};
