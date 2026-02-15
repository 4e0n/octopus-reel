
#pragma once

#include <QThread>
#include <QMutexLocker>
#include <QDateTime>
#include "confparam.h"

class CompThread : public QThread {
 Q_OBJECT
 public:
  explicit CompThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent),conf(c) {}

  void run() override {
   const int eegCh=int(conf->chnCount-2);
   while (!isInterruptionRequested() && !conf->compStop.load()) {
    QByteArray block;

    // Pop one block (no ring mutex involved)
    {
      QMutexLocker lk(&conf->compMutex);
      if (conf->compQueue.isEmpty()) {
       conf->compReady.wait(&conf->compMutex,10);
       continue;
      }
      block=std::move(conf->compQueue.dequeue());
      conf->compSpace.wakeOne();
    }

    TcpSample tcpS;
    if (!tcpS.deserialize(block,conf->chnCount)) continue;

    // Build PP without serialize/deserialize round trip (you will implement this)
    TcpSamplePP tcpSPP(conf->ampCount,conf->physChnCount);
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
       for (int i=0;i<eSz;i++)
        sum+=tcpSPP.amp[ampIdx].dataN[ conf->chnInfo[chnIdx].interElec[i] ];
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

      conf->tcpBuffer[conf->tcpBufHead%conf->tcpBufSize]=std::move(tcpSPP);
      conf->tcpBufHead++;

      const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
      if (avail>=(quint64)conf->eegSamplesInTick) conf->dataReady.wakeOne();
     }
    }
  }

 private:
  ConfParam *conf;
};
