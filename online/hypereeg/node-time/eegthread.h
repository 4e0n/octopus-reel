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
#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QStaticText>
#include <QVector>
#include <QElapsedTimer>
#include <QDateTime>
#include <thread>
#include <cmath>
#include "confparam.h"
#include "../common/sample.h"
#include "../common/octo_omp.h"
#include "../common/logring.h"

// Compute one envelope value from N(=48) normalized samples in [-1,1]
inline float audioEnvFromN_RMS(const float *audN) {
 double acc=0.0;
 for (int i=0;i<48;++i) acc+=double(audN[i])*double(audN[i]);
 return float(std::sqrt(acc/48.0));
}

inline float audioEnvFromN_MAV(const float *audN) {
 double acc=0.0;
 for (int i=0;i<48;++i) acc+=std::abs(double(audN[i]));
 return float(acc/48.0);
}

class EEGThread:public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,QImage *sb=nullptr,QObject *parent=nullptr):QThread(parent) {
   conf=c; ampNo=a; sweepBuffer=sb; threadActive=true;

   physChnCount=conf->physChnCount;
   //colCount=std::ceil((float)physChnCount/(float)(33.));
   //chnPerCol=std::ceil((float)(physChnCount)/(float)(colCount));
   chnPerCol=66;
   colCount=std::ceil((float)(physChnCount)/(float)(chnPerCol));

   chnY=(float)(conf->sweepFrameH-conf->audWaveH)/(float)(chnPerCol); // reserved vertical pixel count per channel

   prevY.resize(physChnCount);
   for (unsigned chnIdx=0;chnIdx<physChnCount;++chnIdx) {
    const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
    prevY[int(chnIdx)]=baseY;
   }
   prevYA=conf->sweepFrameH-conf->audWaveH/2;

   int ww=(int)((float)(conf->sweepFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); wX.append(colIdx*ww+1); }
   for (unsigned int colIdx=0;colIdx<colCount-1;colIdx++) wn.append((colIdx+1)*ww-1);
   wn.append(conf->sweepFrameW-1);

   tcpBuffer=&conf->tcpBuffer; tcpBufSize=conf->tcpBufSize; scrAvailSmp=conf->scrAvailableSamples;

   evtFont=QFont("Helvetica",18,QFont::Bold);

   resetScrollBuffer();
  }

  void resetScrollBuffer() {
   sweepBuffer->fill(Qt::white);
   for (unsigned chnIdx=0;chnIdx<physChnCount;++chnIdx) {
    const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
    prevY[int(chnIdx)]=baseY;
   }
   prevYA=conf->sweepFrameH-conf->audWaveH/2;
   // reset cursors
   for (int colIdx=0;colIdx<wX.size();++colIdx) wX[colIdx]=w0[colIdx];
  }

  void updateBufferTick(quint64 tailSnap,unsigned NSnap) {
   const unsigned N=NSnap; const quint64 tail=tailSnap;
   const int speedIdx=conf->eegSweepSpeedIdx;

   auto shouldDrawIndex=[&](unsigned i) -> bool {
    switch (speedIdx) {
      case 0: return (i+1==N);           // only last
      case 1: return (i+1==N)||(i+2==N); // last two: ...18,19
      case 2: return (i%4)==3;           // 3,7,11,15,19
      case 3: return (i%2)==1;           // 1,3,...,19
      case 4: default: return true;      // all samples
    }
   };

   bool pendingTrig=false; bool pendingOpEvt=false; bool pendingSubEvt=false;

   sweepPainter.begin(sweepBuffer);
   sweepPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

   // We will draw one column per selected i, advancing wX each time
   for (unsigned i=0;i<N;++i) {
    const TcpSamplePP &s=(*tcpBuffer)[(tail+i)%tcpBufSize];

    if (s.trigger>0) { pendingTrig=true; trigVal=s.trigger; trigX=wX[0]+24; trigRepeat=MARKER_REPEAT_COUNT; }
    if (s.opEvt>0) { pendingOpEvt=true; opVal=s.opEvt; opX=wX[0]+24; opRepeat=MARKER_REPEAT_COUNT; }
    if (s.subEvt>0) { pendingSubEvt=true; subVal=s.subEvt; subX=wX[0]+24; subRepeat=MARKER_REPEAT_COUNT; }

    //if (s.opEvt!=0) qDebug() << "CHUNK" << i << s.offset << s.opEvt;

    if (!shouldDrawIndex(i)) continue;

    // Clear cursor lines once per column (per colIdx)
    for (unsigned colIdx=0;colIdx<colCount;++colIdx) {
     if (wX[colIdx]<=wn[colIdx]) {
      sweepPainter.setPen(Qt::white);
      sweepPainter.drawLine(wX[colIdx],0,wX[colIdx],conf->sweepFrameH-1);
      if (wX[colIdx]<wn[colIdx]-1) {
       sweepPainter.setPen(Qt::black);
       sweepPainter.drawLine(wX[colIdx]+1,0,wX[colIdx]+1,conf->sweepFrameH-1);
      }
     }
    }

    // EEG channels
    sweepPainter.setPen(Qt::black);
    for (unsigned chnIdx=0;chnIdx<physChnCount;++chnIdx) {
     const unsigned colIdx=chnIdx/chnPerCol;
     const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
#ifdef EEGBANDSCOMP
     float x;
     switch (conf->eegBand) {
      case 0:
      default:x=s.amp[ampNo].dataBP[chnIdx]; break; // (2-40)
      case 1: x=s.amp[ampNo].dataD[chnIdx];  break; // (2-4)
      case 2: x=s.amp[ampNo].dataT[chnIdx];  break; // (4-8)
      case 3: x=s.amp[ampNo].dataA[chnIdx];  break; // (8-12)
      case 4: x=s.amp[ampNo].dataB[chnIdx];  break; // (12-28)
      case 5: x=s.amp[ampNo].dataG[chnIdx];  break; // (28-40)
     }
#else
     float x=s.amp[ampNo].dataBP[chnIdx]; // for now (2-40)
#endif
     const int y=baseY-int(x*chnY*conf->eegAmpX[ampNo]);
     // draw line from previous to current at this column
     const int x0=wX[colIdx]-1;
     const int x1=wX[colIdx];
     if (x0>=w0[colIdx]) sweepPainter.drawLine(x0,prevY[chnIdx],x1,y);
     prevY[chnIdx]=y;
    }

    // Audio envelope
    const int baseYA=conf->sweepFrameH-conf->audWaveH/2;
    const float env=audioEnvFromN_RMS(s.audioN);
    const int yA=baseYA-int(env*conf->audWaveH*20);
    sweepPainter.setPen(Qt::red);
    for (unsigned colIdx=0;colIdx<colCount;++colIdx) {
     const int x0=wX[colIdx]-1;
     const int x1=wX[colIdx];
     if (x0>=w0[colIdx]) sweepPainter.drawLine(x0,prevYA,x1,yA);
    }
    prevYA=yA;

    // Event / trigger markers
    const int markerTextY=conf->sweepFrameH-6; // just above audio region
    const int markerLineY0=0;
    //const int markerLineY1=conf->sweepFrameH-conf->audWaveH-2; // stop before audio region
    const int markerLineY1=conf->sweepFrameH-1; // whole vertical line

    if (pendingTrig) {
     for (unsigned colIdx=0;colIdx<colCount;++colIdx) { const int x=wX[colIdx];
      sweepPainter.setPen(Qt::red);
      sweepPainter.drawLine(x,markerLineY0,x,markerLineY1);
     }
     pendingTrig=false;
    }
    if (pendingOpEvt) {
     for (unsigned colIdx=0;colIdx<colCount;++colIdx) { const int x=wX[colIdx];
      sweepPainter.setPen(Qt::blue);
      sweepPainter.drawLine(x,markerLineY0,x,markerLineY1);
     }
     pendingOpEvt=false;
    }
    if (pendingSubEvt) {
     for (unsigned colIdx=0;colIdx<colCount;++colIdx) { const int x=wX[colIdx];
      sweepPainter.setPen(Qt::darkGreen);
      sweepPainter.drawLine(x,markerLineY0,x,markerLineY1);
     }
     pendingSubEvt=false;
    }

    if (trigRepeat>0 && trigX>=0) {
     drawRotatedMarker(sweepPainter,trigX,markerTextY,QString("Trigger #%1").arg(trigVal),Qt::red);
     --trigRepeat;
    }
    if (opRepeat>0 && opX>=0) {
     drawRotatedMarker(sweepPainter,opX,markerTextY-18,QString("Operator Event #%1").arg(opVal),Qt::blue);
     --opRepeat;
    }
    if (subRepeat>0 && subX>=0) {
     drawRotatedMarker(sweepPainter,subX,markerTextY-36,QString("Subject Event #%1").arg(subVal),Qt::darkGreen);
     --subRepeat;
    }

    // -- One column drawn, advance x one column
    for (int colIdx=0;colIdx<wX.size();++colIdx) {
     wX[colIdx]++;
     if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx];
    }
   }
   sweepPainter.end();
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
   while (threadActive) {
    quint64 tailSnap; unsigned NSnap;
    {
      QMutexLocker locker(&conf->mutex);

      // Sleep until producer signals
      while (!conf->eegSweepPending[ampNo] && !conf->quitPending && !isInterruptionRequested()) {
       conf->eegSweepWait.wait(&conf->mutex);
      }

      if (conf->quitPending || isInterruptionRequested()) { threadActive=false; break; }
 
      tailSnap=conf->tcpBufTail;
      unsigned int n=conf->scrUpdateSamples; // minimum per tick (e.g. 20)
      const unsigned int cap=conf->scrMaxUpdateSamples;
      // dynamic catch-up: consume more if backlog is large
      const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
      if (avail>quint64(n)) {
       const quint64 want=avail; // “drain all” desire
       const quint64 capped=(want>cap) ? cap:want;
       n=(unsigned int)(capped);
       if (n<conf->scrUpdateSamples) n=conf->scrUpdateSamples;
      }
      NSnap=conf->tickSamples; // shared decision from producer

      conf->eegSweepPending[ampNo]=false; conf->eegSweepUpdating--;
      const bool last=(conf->eegSweepUpdating==0);

      if (last) conf->tcpBufTail+=NSnap;
    }
    QElapsedTimer tDraw; tDraw.start();

    updateBufferTick(tailSnap,NSnap); // no conf->mutex held

    const quint64 ms=quint64(tDraw.elapsed());
    conf->consDrawMsAcc.fetch_add(ms,std::memory_order_relaxed);
    conf->consTicks.fetch_add(1,std::memory_order_relaxed);
    conf->consSamples.fetch_add(NSnap,std::memory_order_relaxed);

    emit updateEEGFrame();

#ifdef PLL_VERBOSE
    // --- 1 Hz consumer log (print only from ampNo==0 to avoid spam) ---
    if (ampNo==0) {
     static qint64 lastMs=0;
     static quint64 lastTicks=0,lastMsAcc=0,lastCons=0,lastWake=0,lastSkip=0;

     const qint64 nowMs=QDateTime::currentMSecsSinceEpoch();
     if (lastMs==0) lastMs=nowMs;

     if (nowMs-lastMs>=1000) {
      lastMs=nowMs;

      const quint64 ticks=conf->consTicks.load(std::memory_order_relaxed);
      const quint64 msAcc=conf->consDrawMsAcc.load(std::memory_order_relaxed);
      const quint64 cons=conf->consSamples.load(std::memory_order_relaxed);

      const quint64 wake=conf->wakeIssued.load(std::memory_order_relaxed);
      const quint64 skip=conf->wakeGateSkip.load(std::memory_order_relaxed);

      const quint64 d_ticks=ticks-lastTicks; lastTicks=ticks;
      const quint64 d_msAcc=msAcc-lastMsAcc; lastMsAcc=msAcc;
      const quint64 d_cons=cons-lastCons;    lastCons=cons;

      const quint64 d_wake=wake-lastWake; lastWake=wake;
      const quint64 d_skip=skip-lastSkip; lastSkip=skip;

      double avgMs=0.0;
      if (d_ticks>0) avgMs=double(d_msAcc)/double(d_ticks);

      quint64 hSnap=0,tSnap=0;
      {
        QMutexLocker locker(&conf->mutex);
        hSnap=conf->tcpBufHead; tSnap=conf->tcpBufTail;
      }
      const quint64 used=hSnap-tSnap;
      const double fillPct=conf->tcpBufSize ? (100.0*double(used)/double(conf->tcpBufSize)):0.0;

      qInfo().noquote()
        << QString("[TIME:CONS] ticks=%1/s avgDraw=%2 ms | cons=%3 smp/s | wake=%4/s gateSkip=%5/s | ring=%6/%7 (%8%) | Ticksamples=%9")
            .arg((qulonglong)d_ticks)
            .arg(avgMs,0,'f',2)
            .arg((qulonglong)d_cons)
            .arg((qulonglong)d_wake)
            .arg((qulonglong)d_skip)
            .arg((qulonglong)used)
            .arg((qulonglong)conf->tcpBufSize)
            .arg(fillPct,0,'f',1)
            .arg(conf->tickSamples);
     }
    }
#endif
   }
   qInfo("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  void drawRotatedMarker(QPainter &p,int x,int y,const QString &text,const QColor &color) {
   p.save();
    p.translate(x,y); p.rotate(-90.0); p.setPen(color); p.setFont(evtFont); p.drawText(0,0,text);
   p.restore();
  }

  ConfParam *conf; unsigned int ampNo; QImage *sweepBuffer; QVector<int> w0,wn,wX;
  unsigned int physChnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<TcpSamplePP> *tcpBuffer; unsigned int tcpBufSize,scrAvailSmp;
  bool threadActive;

  QFont evtFont;
  QPainter sweepPainter; QVector<QStaticText> chnTextCache;

  QVector<int> prevY; // Previous y pixel of EEG channels
  int prevYA=0;       // Previous y pixel of Audio channel

  int trigX=-1,opX=-1,subX=-1; unsigned trigVal=0,opVal=0,subVal=0; int trigRepeat=0,opRepeat=0,subRepeat=0;
  static constexpr int MARKER_REPEAT_COUNT=30;
};
