
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
   while (!isInterruptionRequested() && !conf->compStop.load()) {
    QByteArray block;
    {
      QMutexLocker lk(&conf->compMutex);
      while (conf->compQueue.isEmpty()) conf->compReady.wait(&conf->compMutex);
      block = std::move(conf->compQueue.dequeue());
    }

    // block is OUTER PAYLOAD: [Li][one][Li][one]...
    int pos = 0;
    while (pos + 4 <= block.size()) {
     const uchar *p = reinterpret_cast<const uchar*>(block.constData() + pos);
     const quint32 Li = (quint32)p[0]
                        | ((quint32)p[1] << 8)
                        | ((quint32)p[2] << 16)
                        | ((quint32)p[3] << 24);
     pos += 4;

     if (pos + (int)Li > block.size()) {
      qWarning() << "[PP:COMP] inner frame truncated/corrupt:"
                 << "pos=" << pos << "Li=" << Li << "blockSz=" << block.size();
      break;
     }

     const QByteArray one = block.mid(pos, Li);
     pos += (int)Li;

     TcpSample tcpS;
     if (!tcpS.deserialize(one, conf->chnCount)) {
      // optional: log occasionally
      continue;
     }

     // ---- now your compute path for ONE sample ----
     // build TcpSamplePP, filter, push into ring, etc.
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

       static quint64 lastH=0, lastT=0;
       static qint64  lastMs=0;
       log_ring_1hz("PP:RING", conf->tcpBufHead, conf->tcpBufTail, lastH, lastT, lastMs);

       const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
       if (avail>=(quint64)conf->eegSamplesInTick) conf->dataReady.wakeOne();
     }
    }
   } 
  }

 private:
  ConfParam *conf;
};
