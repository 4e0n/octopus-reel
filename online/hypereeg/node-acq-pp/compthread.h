
#pragma once

#include <QThread>
#include <QMutexLocker>
#include <QDateTime>
#include "confparam.h"
#include "../common/logring.h"

class CompThread : public QThread {
 Q_OBJECT
 public:
  explicit CompThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent),conf(c) {}

  void run() override {
   const int eegCh=int(conf->chnCount-2);
   TcpSample tcpS(conf->ampCount,conf->physChnCount);
   TcpSamplePP tcpSPP(conf->ampCount,conf->physChnCount);
   while (!isInterruptionRequested() && !conf->compStop.load()) {
    QByteArray block;
    {
      QMutexLocker lk(&conf->compMutex);

      while (conf->compQueue.isEmpty() && !isInterruptionRequested() && !conf->compStop.load()) {
       conf->compReady.wait(&conf->compMutex, 50);
      }
      if (isInterruptionRequested() || conf->compStop.load()) break;

      block = std::move(conf->compQueue.dequeue());
    }

    // block is OUTER PAYLOAD: [one][one][one]... (fixed size sz)
    const int sz = conf->frameBytesIn;
    if (sz <= 0) { qWarning() << "[PP:COMP] frameBytesIn not set"; continue; }
    if (block.size() % sz != 0) {
     qWarning() << "[PP:COMP] block size not multiple of frameBytesIn:"
                << "blockSz=" << block.size() << "frameBytesIn=" << sz;
     continue;
    }

    const char* base = block.constData();
    const int frames = block.size() / sz;

    for (int i = 0; i < frames; ++i) {
     const char* one = base + i * sz;

     if (!tcpS.deserializeRaw(one,sz,conf->chnCount,conf->ampCount)) continue;

     // compute -> push ring
     // ---- compute path for ONE sample ----
     // build TcpSamplePP, filter, push into ring, etc.
     // Build PP without serialize/deserialize round trip (you will implement this)
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

     // Push into ring (SHORT critical section)
     {
       QMutexLocker locker(&conf->mutex);

       // pick policy:
       // (A) NO-DROP: block comp thread until space exists
       while ((conf->tcpBufHead-conf->tcpBufTail)>=conf->tcpBufSize) {
        conf->spaceReady.wait(&conf->mutex,5);
       }

       // (B) LIVE-FIRST: drop oldest if ring full
       // while ((conf->tcpBufHead - conf->tcpBufTail) >= conf->tcpBufSize) conf->tcpBufTail++;

       //conf->tcpBuffer[conf->tcpBufHead%conf->tcpBufSize]=std::move(tcpSPP);
       auto &slot=conf->tcpBuffer[conf->tcpBufHead%conf->tcpBufSize];
       slot = tcpSPP; // or slot.copyFrom(tcpSPP)

       conf->tcpBufHead++;

       static quint64 lastH=0, lastT=0;
       static qint64  lastMs=0;
       log_ring_1hz("PP:RING", conf->tcpBufHead, conf->tcpBufTail, lastH, lastT, lastMs);

       //const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
       //if (avail>=(quint64)conf->eegSamplesInTick) conf->dataReady.wakeOne();

       const quint64 availBefore=(conf->tcpBufHead-1)-conf->tcpBufTail;
       const quint64 availAfter=conf->tcpBufHead-conf->tcpBufTail;
       if (availBefore<(quint64)conf->eegSamplesInTick && availAfter>=(quint64)conf->eegSamplesInTick)
        conf->dataReady.wakeOne();
     }
    }
   } 
  }

 private:
  ConfParam *conf;
};
