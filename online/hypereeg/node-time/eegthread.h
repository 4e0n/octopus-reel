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
#include <thread>
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
   conf=c; ampNo=a; sweepBuffer=sb; trigger=0; threadActive=true;

   chnCount=conf->chnInfo.size();
   //colCount=std::ceil((float)chnCount/(float)(33.));
   //chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   chnPerCol=66;
   colCount=std::ceil((float)(chnCount)/(float)(chnPerCol));

   prevY.resize(chnCount);
   for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
    const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
    prevY[int(chnIdx)]=baseY;
   }
   prevYA=conf->sweepFrameH-conf->audWaveH/2;

   int ww=(int)((float)(conf->sweepFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); wX.append(colIdx*ww+1); }
   for (unsigned int colIdx=0;colIdx<colCount-1;colIdx++) wn.append((colIdx+1)*ww-1);
   wn.append(conf->sweepFrameW-1);

   tcpBuffer=&conf->tcpBuffer; tcpBufSize=conf->tcpBufSize; scrAvailSmp=conf->scrAvailableSamples;
   chnY=(float)(conf->sweepFrameH-conf->audWaveH)/(float)(chnPerCol); // reserved vertical pixel count per channel

   evtFont=QFont("Helvetica",14,QFont::Bold);
   rTransform.rotate(-90);

   rBuffer=QImage(120,24,QImage::Format_RGB32); rBuffer.fill(Qt::transparent);
   rBufferC=QImage(); // cached - empty until first use

   resetScrollBuffer();
  }

  void resetScrollBuffer() {
   sweepBuffer->fill(Qt::white);
   for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
    const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
    prevY[int(chnIdx)]=baseY;
   }
   prevYA=conf->sweepFrameH-conf->audWaveH/2;
   // reset cursors
   for (int colIdx=0;colIdx<wX.size();++colIdx) wX[colIdx]=w0[colIdx];
  }

  inline bool shouldDrawIdx(unsigned i,unsigned N,int speedIdx) {
   switch (speedIdx) {
    case 0: return (i+1==N);           // 1 column
    case 1: return (i+1==N)||(i+2==N); // 2 columns
    case 2: return (i%4)==3;           // 5 columns
    case 3: return (i%2)==1;           // 10 columns
    case 4:
    default: return true;              // 20 columns
   }
  }

  void updateBufferTick(quint64 baseTail,unsigned N,int speedIdx,float ampX) {
//   auto shouldDrawIndex=[&](unsigned i) -> bool {
//    switch (speedIdx) {
//      case 0: return (i+1==N);           // only last
//      case 1: return (i+1==N)||(i+2==N); // last two: ...18,19
//      case 2: return (i%4)==3;           // 3,7,11,15,19
//      case 3: return (i%2)==1;           // 1,3,...,19
//      case 4: default: return true;      // all samples
//    }
//   };

   sweepPainter.begin(sweepBuffer);
   sweepPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

   // We will draw one column per selected i, advancing wX each time
   for (unsigned i=0;i<N;++i) {
    if (!shouldDrawIdx(i,N,speedIdx)) continue;
    const TcpSamplePP &s=(*tcpBuffer)[(baseTail+i)%tcpBufSize];

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
    sweepPainter.setPen(Qt::black); float x; int y;
    for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
     const unsigned colIdx=chnIdx/chnPerCol;
     const int baseY=int(chnY/2.0f+chnY*(chnIdx%chnPerCol));
     y=baseY-int(x*chnY*ampX);
     x=s.amp[ampNo].dataBP[chnIdx]; // for now (2-40)
     y=baseY-int(x*chnY*conf->eegAmpX[ampNo]);
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

    // Trigger marker
    if (s.trigger>0) {
     sweepPainter.setPen(Qt::blue);
     for (unsigned colIdx=0;colIdx<colCount;++colIdx)
      sweepPainter.drawLine(wX[colIdx],0,wX[colIdx],conf->sweepFrameH-1);
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
    quint64 baseTail=0; unsigned N=0; int speedIdx=0; float ampX=1.f; quint64 hSnap=0,tSnap=0;
    {
      QMutexLocker locker(&conf->mutex);

      // Sleep until producer signals
      while (!conf->eegSweepPending[ampNo]) { conf->eegSweepWait.wait(&conf->mutex); }

      if (conf->quitPending) {
       // consume our token so the producer side doesn't deadlock waiting for us
       conf->eegSweepPending[ampNo]=false; conf->eegSweepUpdating--;
       if (conf->eegSweepUpdating==0) conf->tcpBufTail+=conf->scrUpdateSamples;
       hSnap=conf->tcpBufHead; tSnap=conf->tcpBufTail;
       threadActive=false; //qDebug() << "Thread got the finish signal:" << ampNo;
      } else {
       // snapshot what we need, then unlock before painting
       baseTail=conf->tcpBufTail; N=conf->scrUpdateSamples;
       speedIdx=conf->eegSweepSpeedIdx; ampX=conf->eegAmpX[ampNo];
       conf->eegSweepPending[ampNo]=false;
       conf->eegSweepUpdating--; // semaphore.. kind of..  
       if (conf->eegSweepUpdating==0) conf->tcpBufTail+=conf->scrUpdateSamples;
       hSnap=conf->tcpBufHead; tSnap=conf->tcpBufTail;
      }
    }
    // Paint independent of mutex
    updateBufferTick(baseTail,N,speedIdx,ampX);
    emit updateEEGFrame(); //qDebug() << ampNo << "updated.";

    // If this was the last scroller then advance tail..
    if (conf->eegSweepUpdating==0) conf->tcpBufTail+=conf->scrUpdateSamples;

    hSnap=conf->tcpBufHead; tSnap=conf->tcpBufTail;
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  ConfParam *conf; unsigned int ampNo; QImage *sweepBuffer; QVector<int> w0,wn,wX;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<TcpSamplePP> *tcpBuffer; unsigned int tcpBufSize,scrAvailSmp;
  bool threadActive;

  QFont chnFont,evtFont; QImage rBuffer,rBufferC; QTransform rTransform;
  QPainter sweepPainter,rotPainter; QVector<QStaticText> chnTextCache;

  int trigger;

  QVector<int> prevY; // Previous y pixel of EEG channels
  int prevYA=0;       // Previous y pixel of Audio channel
};
