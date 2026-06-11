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
#include <cmath>
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

   const unsigned int ampCount=conf->ampCount;
   const unsigned int refChnCount=conf->refChnCount; const unsigned int bipChnCount=conf->bipChnCount;
   const unsigned int metaChnCount=conf->metaChnCount;
   const unsigned int chnCount=conf->physChnCount; const unsigned int physChnCount=conf->physChnCount;

   const auto& refChns=conf->refChns; const auto& bipChns=conf->bipChns; const auto& metaChns=conf->metaChns;
   auto& filterListN=conf->filterListN; auto& filterListBP=conf->filterListBP;
#ifdef EEGBANDSCOMP
   auto& filterListD=conf->filterListD; auto& filterListT=conf->filterListT;
   auto& filterListA=conf->filterListA; auto& filterListB=conf->filterListB;
   auto& filterListG=conf->filterListG;
#endif

   if (conf->cmWindowSamples==0||conf->cmUpdateStepSamples==0) {
    qCritical() << "[PP:COMP] Invalid CM configuration.";
    return;
   }

#ifdef EEGBANDSCOMP
   if (conf->powerWindowSamples==0 || conf->powerUpdateStepSamples==0) {
    qCritical() << "[PP:COMP] Invalid power sliding-window configuration.";
    return;
   }

   const unsigned int powerChnCount=conf->powerChnCount;
   const unsigned int gfpAllOutIdx=conf->grandChnCount;
   const unsigned int gfpLeftOutIdx=conf->grandChnCount+1;
   const unsigned int gfpRightOutIdx=conf->grandChnCount+2;

   QVector<QVector<QVector<QVector<float>>>> powerRing; // [amp][chn][band][ring]
   QVector<QVector<QVector<double>>> powerRunningSumSq; // [amp][chn][band]
   powerRing.resize(ampCount);
   powerRunningSumSq.resize(ampCount);
   for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
    powerRing[ampIdx].resize(powerChnCount);
    powerRunningSumSq[ampIdx].resize(powerChnCount);
    for (unsigned int chnIdx=0;chnIdx<powerChnCount;++chnIdx) {
     powerRing[ampIdx][chnIdx].resize(ConfParam::POWER_BAND_COUNT);
     powerRunningSumSq[ampIdx][chnIdx].resize(ConfParam::POWER_BAND_COUNT);
     for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
      powerRing[ampIdx][chnIdx][b].resize(conf->powerWindowSamples);
      for (unsigned int k=0;k<conf->powerWindowSamples;++k)
       powerRing[ampIdx][chnIdx][b][int(k)]=0.0f;
      powerRunningSumSq[ampIdx][chnIdx][b]=0.0;
     }
    }
   }

   unsigned int powerRingPos=0;
   unsigned int powerValidSamples=0;
   quint64 powerSinceLastPublish=0;
#endif

   // Initialize CMlevels ringbuffer
   const unsigned int cmRingSize=conf->cmWindowSamples;
   QVector<QVector<QVector<float>>> cmDiffRing;
   cmDiffRing.resize(ampCount);
   for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
    cmDiffRing[ampIdx].resize(chnCount);
    for (unsigned int chnIdx=0;chnIdx<chnCount;++chnIdx) {
     cmDiffRing[ampIdx][chnIdx].resize(cmRingSize);
     for (unsigned int i=0;i<cmRingSize;i++) cmDiffRing[ampIdx][chnIdx][i]=0.0f;
    }
   }
   unsigned int cmRingPos=0; unsigned int cmValidSamples=0; quint64 cmSinceLastCompute=0;

#ifdef EEGBANDSCOMP
   QVector<QVector<QVector<double>>> powerSumSq; // [amp][grandChn][band]
   powerSumSq.resize(ampCount);
   for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
    powerSumSq[ampIdx].resize(powerChnCount);
    for (unsigned int chnIdx=0;chnIdx<powerChnCount;++chnIdx) {
     powerSumSq[ampIdx][chnIdx].resize(ConfParam::POWER_BAND_COUNT);
     for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b)
      powerSumSq[ampIdx][chnIdx][b]=0.0;
    }
   }
#endif

   // Initialize main stream processing
   TcpSample tcpS(ampCount,physChnCount); // Ref+Bip Channels
   TcpSamplePP tcpSPP(ampCount,conf->grandChnCount); // Ref+Bip+Meta+GFP Channels

   // Simple guard: when output ring is "near full", pause briefly.
   // This prevents burning CPU while SEND drains.
   const quint64 outNearFullThreshold=(conf->tcpBufSize > quint64(conf->eegSamplesInTick)) ?
                                       (conf->tcpBufSize-quint64(conf->eegSamplesInTick)):(conf->tcpBufSize);

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
     if (!tcpS.deserializeRaw(one,sz,physChnCount,ampCount)) continue;
     // Compute for ONE sample (outside ring lock)
     tcpSPP.fromTcpSample(tcpS,physChnCount);

     // ==============================================================================================================
     // ==============================================================================================================
     // NODE-COMP STYLE GENERIC COMPUTATION TO BE IMPLEMENTED HERE!!
     // ==============================================================================================================
     // ==============================================================================================================

     // OpenMP: parallelize over amps only (lower overhead than nested)
     // If ampCount is 1, omp adds overhead -> guard it.
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if(ampCount>=2)
#endif
     // Apply pre-filter (and the notch magnitudes) before neighbor-interpolation
     for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) for (unsigned int chnIdx=0; chnIdx<chnCount; ++chnIdx) {
      const float x=tcpSPP.amp[ampIdx].data[chnIdx];
      // notch-filtered version is master
      auto &notch=filterListN[ampIdx][chnIdx];
      const float xN=notch.filterSample(x); tcpSPP.amp[ampIdx].dataN[chnIdx]=xN;
      // CM rolling buffer stores the removed CM (mostly mains) component
      cmDiffRing[ampIdx][chnIdx][cmRingPos]=x-xN;
     }
     // Update CM index
     cmRingPos++; if (cmRingPos>=cmRingSize) cmRingPos=0;
     if (cmValidSamples<cmRingSize) cmValidSamples++;
     cmSinceLastCompute++;

     // Recompute actual CM values
     if (cmValidSamples>=conf->cmWindowSamples && cmSinceLastCompute>=conf->cmUpdateStepSamples) {
      QVector<QVector<float>> newCM; newCM.resize(ampCount);
      for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
       newCM[ampIdx].resize(chnCount);
       for (unsigned int chnIdx=0;chnIdx<chnCount;++chnIdx) {
        double acc=0.0;
        for (unsigned int k=0;k<cmRingSize;++k) {
         const float d=cmDiffRing[ampIdx][chnIdx][k]; acc+=double(d)*double(d);
        }
        newCM[ampIdx][chnIdx]=std::sqrt(acc/double(conf->cmWindowSamples));
       }
      }

      {
        QMutexLocker lk(&conf->cmMutex);
        conf->latestCMLevels=std::move(newCM);
        conf->cmLevelsValid=true;
      }
      cmSinceLastCompute=0;
     }

     // -----------------------------------------------------------------------------------------

     // Channels' interpolation on Notch filtered version
     for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
      for (unsigned int chnIdx=0;chnIdx<refChnCount;++chnIdx) { // Should be refChnCount as it is only EEG

       // 1. Referential EEG interpolation + filtering
       const unsigned interMode=refChns[chnIdx].interMode[ampIdx];
       float xN=tcpSPP.amp[ampIdx].dataN[chnIdx];
       // Default is 1
       if (interMode==2) { // INTERPOLATE
        const int eSz=refChns[chnIdx].interElec.size();
	if (eSz>0) {
         float sum=0.f; for (int ei=0;ei<eSz;++ei) sum+=tcpSPP.amp[ampIdx].dataN[ refChns[chnIdx].interElec[ei] ];
	 xN=sum/float(eSz);
        }
       } else if (interMode==0) { // OFF
        xN=0.f;
       }
       // Now filter further the interpolated version
       auto &bp=filterListBP[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataBP[chnIdx]=bp.filterSample(xN);
#ifdef EEGBANDSCOMP
       auto &delta=filterListD[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataD[chnIdx]=delta.filterSample(xN);
       auto &theta=filterListT[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataT[chnIdx]=theta.filterSample(xN);
       auto &alpha=filterListA[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataA[chnIdx]=alpha.filterSample(xN);
       auto &beta=filterListB[ampIdx][chnIdx];  tcpSPP.amp[ampIdx].dataB[chnIdx]=beta.filterSample(xN);
       auto &gamma=filterListG[ampIdx][chnIdx]; tcpSPP.amp[ampIdx].dataG[chnIdx]=gamma.filterSample(xN);
#endif
      }

      // 2. GFP computation from already-interpolated, already-bandpassed ref EEG
#ifdef EEGBANDSCOMP
      tcpSPP.amp[ampIdx].computeGFPs(conf->gfpAllIdx,conf->gfpLeftIdx,conf->gfpRightIdx);
#endif

      // 3. Bipolar channels
      for (unsigned int chnIdx=0;chnIdx<bipChnCount;++chnIdx) {
       const unsigned interMode=bipChns[chnIdx].interMode[ampIdx];
       float xN=tcpSPP.amp[ampIdx].dataN[refChnCount+chnIdx];
       // Default is 1
       if (interMode==2) { // INTERPOLATE
        const int eSz=bipChns[chnIdx].interElec.size();
	if (eSz>0) {
         float sum=0.f; for (int ei=0;ei<eSz;++ei) sum+=tcpSPP.amp[ampIdx].dataN[refChnCount + bipChns[chnIdx].interElec[ei] ];
	 xN=sum/float(eSz);
        }
       } else if (interMode==0) { // OFF
        xN=0.f;
       }
       // Now filter further the interpolated version
       auto &bp=filterListBP[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataBP[refChnCount+chnIdx]=bp.filterSample(xN);
#ifdef EEGBANDSCOMP
       auto &delta=filterListD[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataD[refChnCount+chnIdx]=delta.filterSample(xN);
       auto &theta=filterListT[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataT[refChnCount+chnIdx]=theta.filterSample(xN);
       auto &alpha=filterListA[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataA[refChnCount+chnIdx]=alpha.filterSample(xN);
       auto &beta=filterListB[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataB[refChnCount+chnIdx]=beta.filterSample(xN);
       auto &gamma=filterListG[ampIdx][refChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataG[refChnCount+chnIdx]=gamma.filterSample(xN);
#endif
      }

      for (unsigned int chnIdx=0;chnIdx<metaChnCount;++chnIdx) {
       const unsigned interMode=metaChns[chnIdx].interMode[ampIdx];
       float xN=tcpSPP.amp[ampIdx].dataN[physChnCount+chnIdx];
       // Default is 1
       if (interMode==2) { // INTERPOLATE
        const int eSz=metaChns[chnIdx].interElec.size();
	if (eSz>0) {
         float sum=0.f; for (int ei=0;ei<eSz;++ei) sum+=tcpSPP.amp[ampIdx].dataN[physChnCount + metaChns[chnIdx].interElec[ei] ];
	 xN=sum/float(eSz);
        }
       } else if (interMode==0) { // OFF
        xN=0.f;
       }
       // Now filter further the interpolated version
       auto &bp=filterListBP[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataBP[physChnCount+chnIdx]=bp.filterSample(xN);
#ifdef EEGBANDSCOMP
       auto &delta=filterListD[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataD[physChnCount+chnIdx]=delta.filterSample(xN);
       auto &theta=filterListT[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataT[physChnCount+chnIdx]=theta.filterSample(xN);
       auto &alpha=filterListA[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataA[physChnCount+chnIdx]=alpha.filterSample(xN);
       auto &beta=filterListB[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataB[physChnCount+chnIdx]=beta.filterSample(xN);
       auto &gamma=filterListG[ampIdx][physChnCount+chnIdx]; tcpSPP.amp[ampIdx].dataG[physChnCount+chnIdx]=gamma.filterSample(xN);
#endif
      }
     }

#ifdef EEGBANDSCOMP
     // -----------------------------------------------------------------------------------------
     // Sliding-window power accumulation for node-gui-glpower
     //
     // Window:
     //   conf->powerWindowSamples samples, e.g. 10000 = 10 s at 1000 Hz
     //
     // Publish:
     //   every conf->powerUpdateStepSamples samples, e.g. 500 = 2/sec
     //
     // Output:
     //   latestPowerRMS[amp][powerChn][band]
     //
     // powerChn layout:
     //   0 ... grandChnCount-1 : ordinary processed channels
     //   grandChnCount         : GFP-all
     //   grandChnCount+1       : GFP-left
     //   grandChnCount+2       : GFP-right

     auto pushPowerSq=[&](unsigned int ampIdx,unsigned int chnIdx,int bandIdx,double sq) {
      const float oldSq=powerRing[ampIdx][chnIdx][bandIdx][int(powerRingPos)];
      powerRing[ampIdx][chnIdx][bandIdx][int(powerRingPos)]=float(sq);
      powerRunningSumSq[ampIdx][chnIdx][bandIdx]+=sq;
      powerRunningSumSq[ampIdx][chnIdx][bandIdx]-=double(oldSq);
     };

     auto pushBandSet = [&](unsigned int ampIdx,unsigned int outIdx,
                            double xBP,double xD,double xT,double xA,double xB,double xG) {
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_OVERALL,xBP*xBP);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_DELTA,xD*xD);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_THETA,xT*xT);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_ALPHA,xA*xA);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_BETA,xB*xB);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_GAMMA,xG*xG);
     };

     auto pushGFPBandSet = [&](unsigned int ampIdx,unsigned int outIdx,const std::vector<int> &idxs) {
      if (idxs.empty()) {
       pushBandSet(ampIdx,outIdx,0,0,0,0,0,0);
       return;
      }
      double sBP=0.0,sD=0.0,sT=0.0,sA=0.0,sB=0.0,sG=0.0; int n=0;
      for (int ch:idxs) {
       if (ch<0 || ch>=int(conf->grandChnCount)) continue;
       const double xBP=double(tcpSPP.amp[ampIdx].dataBP[ch]);
       const double xD=double(tcpSPP.amp[ampIdx].dataD[ch]);
       const double xT=double(tcpSPP.amp[ampIdx].dataT[ch]);
       const double xA=double(tcpSPP.amp[ampIdx].dataA[ch]);
       const double xB=double(tcpSPP.amp[ampIdx].dataB[ch]);
       const double xG=double(tcpSPP.amp[ampIdx].dataG[ch]);
       sBP+=xBP*xBP; sD+=xD*xD; sT+=xT*xT; sA+=xA*xA; sB+=xB*xB; sG+=xG*xG;
       ++n;
      }
      if (n<=0) { pushBandSet(ampIdx,outIdx,0,0,0,0,0,0); return; }
      const double invN=1.0/double(n);
      // For GFP, store mean square directly.
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_OVERALL,sBP*invN);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_DELTA,sD*invN);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_THETA,sT*invN);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_ALPHA,sA*invN);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_BETA,sB*invN);
      pushPowerSq(ampIdx,outIdx,ConfParam::POWER_GAMMA,sG*invN);
     };
     for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
      for (unsigned int chnIdx=0;chnIdx<conf->grandChnCount;++chnIdx) {
       pushBandSet(ampIdx,chnIdx,
        double(tcpSPP.amp[ampIdx].dataBP[chnIdx]),
        double(tcpSPP.amp[ampIdx].dataD [chnIdx]),
        double(tcpSPP.amp[ampIdx].dataT [chnIdx]),
        double(tcpSPP.amp[ampIdx].dataA [chnIdx]),
        double(tcpSPP.amp[ampIdx].dataB [chnIdx]),
        double(tcpSPP.amp[ampIdx].dataG [chnIdx])
       );
      }
      pushGFPBandSet(ampIdx,gfpAllOutIdx,conf->gfpAllIdx);
      pushGFPBandSet(ampIdx,gfpLeftOutIdx,conf->gfpLeftIdx);
      pushGFPBandSet(ampIdx,gfpRightOutIdx,conf->gfpRightIdx);
     }
     powerRingPos++;
     if (powerRingPos>=conf->powerWindowSamples) powerRingPos=0;
     if (powerValidSamples<conf->powerWindowSamples) powerValidSamples++;
     powerSinceLastPublish++;
     if (powerValidSamples>=conf->powerWindowSamples && powerSinceLastPublish>=conf->powerUpdateStepSamples) {
      QVector<QVector<QVector<float>>> newPower; newPower.resize(ampCount);
      for (unsigned int ampIdx=0;ampIdx<ampCount;++ampIdx) {
       newPower[ampIdx].resize(powerChnCount);
       for (unsigned int chnIdx=0;chnIdx<powerChnCount;++chnIdx) {
        newPower[ampIdx][chnIdx].resize(ConfParam::POWER_BAND_COUNT);
        for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
         newPower[ampIdx][chnIdx][b]=float(std::sqrt(powerRunningSumSq[ampIdx][chnIdx][b]/double(powerValidSamples)));
        }
       }
      }

      {
       QMutexLocker lk(&conf->powerMutex);
       conf->latestPowerRMS=std::move(newPower);
       conf->powerValuesValid=true;
      }

      powerSinceLastPublish=0;
     }
#endif

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
