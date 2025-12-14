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

class EEGThread : public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,QImage *sb=nullptr,QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; sweepBuffer=sb; trigger=0; threadActive=true;

   chnCount=conf->chns.size();
   colCount=std::ceil((float)chnCount/(float)(33.));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
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
   QRect cr(0,0,conf->sweepFrameW-1,conf->sweepFrameH-1);
   sweepBuffer->fill(Qt::white);
  }

  void meanf(unsigned int startIdx, float* mu, float* sigma) {
   const unsigned tcpBufTail=conf->tcpBufTail; const unsigned scrUpdateSmp=conf->scrUpdateSamples;
   const float invSmp=1.0f/float(scrUpdateSmp);
   double sum=0.0,sumSq=0.0;
   SIMD_REDUCE2(sum,sumSq)
   for (int smpIndex=0;smpIndex<int(scrUpdateSmp);++smpIndex) {
    const auto& s=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize];
    const float x=audioEnvFromN_RMS(s.audioN);
    sum+=x; sumSq+=double(x)*double(x);
   }
   const double m=sum*invSmp; const double v=std::max(0.0,sumSq*invSmp-m*m);
   *mu=float(m); *sigma=float(std::sqrt(v));
  }

  void mean(unsigned int startIdx,std::vector<float>* mu,std::vector<float>* sigma) {
   const unsigned tcpBufTail=conf->tcpBufTail; const unsigned scrUpdateSmp=conf->scrUpdateSamples;
   const float invSmp=1.0f/float(scrUpdateSmp); //const bool useNotch=conf->ctrlNotchActive;
   PARFOR(chnIndex,0,int(chnCount)) {
    double sum=0.0,sumSq=0.0;
//    if (useNotch) {
//     SIMD_REDUCE2(sum,sumSq)
//     for (int smpIndex=0;smpIndex<int(scrUpdateSmp);++smpIndex) {
//      const auto& s=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize];
//      const float x=s.amp[ampNo].dataN[chnIndex];
//      if (s.trigger>0) trigger=s.trigger;
//      sum+=x; sumSq+=double(x)*double(x);
//     }
//    } else {
     SIMD_REDUCE2(sum,sumSq)
     for (int smpIndex=0;smpIndex<int(scrUpdateSmp);++smpIndex) {
      const auto& s=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize];
      float x;
      switch (conf->eegBand) {
       default:
       case 0: x=s.amp[ampNo].dataBP[chnIndex]; break;
       case 1: x=s.amp[ampNo].dataD[chnIndex]; break;
       case 2: x=s.amp[ampNo].dataT[chnIndex]; break;
       case 3: x=s.amp[ampNo].dataA[chnIndex]; break;
       case 4: x=s.amp[ampNo].dataB[chnIndex]; break;
       case 5: x=s.amp[ampNo].dataG[chnIndex]; break;
      }
      if (s.trigger>0) trigger=s.trigger;
      sum+=x; sumSq+=double(x)*double(x);
     }
//    }
    const double m=sum*invSmp; const double v=std::max(0.0,sumSq*invSmp-m*m);
    (*mu)[chnIndex]=float(m); (*sigma)[chnIndex]=float(std::sqrt(v));
   }
  }

  void updateBuffer() {
   unsigned int scrUpdateSmp=conf->scrUpdateSamples;
   unsigned int scrUpdateCount=scrAvailSmp/scrUpdateSmp;

   sweepPainter.begin(sweepBuffer);
    sweepPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (conf->eegSweepMode) { // Scroll contents left by scrUpdateCount pixels
     //for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     // sweepBuffer->scroll(-scrUpdateCount,0,sweepBuffer->rect()); // Scroll contents left by scrUpdateCount pixels
     // wX[colIdx]=wn[colIdx]-scrUpdateCount; // Draw new segment on right
     //}
    } else { // SweepMode: draw on moving cursor
     //for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     // //if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx]; // Check for end of screen
//
//
//     }
;   }

    for (unsigned int smpIdx=0;smpIdx<scrUpdateCount-1;smpIdx++) {
     std::vector<float> s0(chnCount); std::vector<float> s0s(chnCount);
     std::vector<float> s1(chnCount); std::vector<float> s1s(chnCount);
     float sA0,sA1; float sA0s,sA1s;
     mean((smpIdx+0)*scrUpdateSmp,&s0,&s0s); mean((smpIdx+1)*scrUpdateSmp,&s1,&s1s);
     meanf((smpIdx+0)*scrUpdateSmp,&sA0,&sA0s); meanf((smpIdx+1)*scrUpdateSmp,&sA1,&sA1s);

     TcpSample tcpS=(*tcpBuffer)[(conf->tcpBufTail+smpIdx)%conf->tcpBufSize];

//     if (tcpS.offset%1000==0) {
//      qint64 now=QDateTime::currentMSecsSinceEpoch(); qint64 age=now-tcpS.timestampMs;
//      qInfo() << "Sample latency @updateBuffer:" << age << "ms";
//     }

     for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
      unsigned int colIdx=chnIdx/chnPerCol;
      scrCurY=(int)(chnY/2.0+chnY*(chnIdx%chnPerCol));

      // Cursor vertical line for each column
      if (wX[colIdx]<=wn[colIdx]) {
       sweepPainter.setPen(Qt::white); sweepPainter.drawLine(wX[colIdx],0,wX[colIdx],conf->sweepFrameH-1);
       if (wX[colIdx]<(wn[colIdx]-1)) {
        sweepPainter.setPen(Qt::black); sweepPainter.drawLine(wX[colIdx]+1,0,wX[colIdx]+1,conf->sweepFrameH-1);
       }
      }

      // Baselines
      //sweepPainter.setPen(Qt::darkGray);
      //sweepPainter.drawLine(wX[colIdx]-1,scrCurY,wX[colIdx],scrCurY);
      // Data
 
      int mu0=scrCurY-(int)(s0[chnIdx]*chnY*conf->eegAmpX[ampNo]);
      int mu1=scrCurY-(int)(s1[chnIdx]*chnY*conf->eegAmpX[ampNo]);
      int sigma0=(int)(s0s[chnIdx]*chnY*conf->eegAmpX[ampNo]*0.5);
      int sigma1=(int)(s1s[chnIdx]*chnY*conf->eegAmpX[ampNo]*0.5);

      sweepPainter.setPen(Qt::blue);
      sweepPainter.drawLine(wX[colIdx]-1,mu0-sigma0,wX[colIdx]-1,mu0+sigma0);
      sweepPainter.drawLine(wX[colIdx],  mu1-sigma1,wX[colIdx],  mu1+sigma1);

      sweepPainter.setPen(Qt::black);
      sweepPainter.drawLine(wX[colIdx]-1,mu0,wX[colIdx],mu1);

     }

     scrCurY=conf->sweepFrameH-conf->audWaveH/2;
     int mu0=scrCurY-(int)(sA0*conf->audWaveH*20);
     int mu1=scrCurY-(int)(sA1*conf->audWaveH*20);
     //int sigma0=(int)(sA0s*conf->audWaveH*20*0.5);
     //int sigma1=(int)(sA1s*conf->audWaveH*20*0.5);

     sweepPainter.setPen(Qt::red);
     for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     // sweepPainter.drawLine(wX[colIdx]-1,mu0-sigma0,wX[colIdx]-1,mu0+sigma0);
     // sweepPainter.drawLine(wX[colIdx],  mu1-sigma1,wX[colIdx],  mu1+sigma1);
     // sweepPainter.setPen(Qt::black);
      sweepPainter.drawLine(wX[colIdx]-1,mu0,wX[colIdx],mu1);
     }

     if (trigger) { trigger=0;
      sweepPainter.setPen(Qt::blue);
      for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
       sweepPainter.drawLine(wX[colIdx]-1,0,wX[colIdx]-1,conf->sweepFrameH);
      }
     }

     for (int colIdx=0;colIdx<wX.size();colIdx++) {
      wX[colIdx]++; if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx]; // Check for end of screen
     }
    }
 
/*
    // Upcoming event
    if (conf->event) { conf->event=false; rBuffer.fill(Qt::transparent);

     rotPainter.begin(&rBuffer);
      rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
      rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
      if (conf->curEventType==1) penColor=Qt::blue; else if (conf->curEventType==2) penColor=Qt::red;
      rotPainter.setPen(penColor); rotPainter.drawText(2,9,conf->curEventName);
     rotPainter.end();
     rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);

     sweepPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) {
      sweepPainter.drawLine(wX[colIdx]-1,1,wX[colIdx]-1,conf->sweepFrameH-1);
      sweepPainter.drawPixmap(wX[colIdx]-15,conf->sweepFrameH-104,rBufferC);
     }
    
     sweepPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); sweepPainter.setPen(Qt::blue); // Line color
     QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount/chnPerCol);
     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) { int lineX=wX[colIdx]-1;
      sweepPainter.drawLine(lineX,1,lineX,conf->sweepFrameH-1);
      // Prepare batched label blitting - for source in pixmap
      fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->sweepFrameH-104),
                                                        QRectF(0,0,rBufferC.width(),rBufferC.height())));
     }
     sweepPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
    } 
*/
    // Channel names
//    sweepPainter.setPen(QColor(50,50,150)); sweepPainter.setFont(chnFont);
//    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
//     unsigned int colIdx=chnIdx/chnPerCol;
//     scrCurY=(int)(-8+chnY/2.0+chnY*(chnIdx%chnPerCol));
//     sweepPainter.drawStaticText(w0[colIdx]+4,scrCurY,chnTextCache[chnIdx]);
//    }
/*
    if (!conf->scrollMode) { // Scroll contents left by scrUpdateCount pixels
     for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
      // Main position increments of columns
      //wX[colIdx]+=conf->scrUpdateCount;
      wX[colIdx]++;
     }
    }
*/
   //if (wX[0]==150) sweepPainter.drawLine(149,300,149,400); // Amplitude legend

   sweepPainter.end();
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
  
   while (threadActive) {
    {
     QMutexLocker locker(&conf->mutex);
     // Sleep until producer signals
     while (!conf->eegSweepPending[ampNo]) { conf->eegSweepWait.wait(&conf->mutex); }

     if (conf->quitPending) {
      threadActive=false; //qDebug() << "Thread got the finish signal:" << ampNo;
     } else {
      updateBuffer();
      emit updateEEGFrame(); //qDebug() << ampNo << "updated.";
     }
     conf->eegSweepPending[ampNo]=false;
     conf->eegSweepUpdating--; // semaphore.. kind of..  
     // If this was the last scroller, then advance tail..
     if (conf->eegSweepUpdating==0) conf->tcpBufTail+=conf->scrAvailableSamples;
    }
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  ConfParam *conf; unsigned int ampNo; QImage *sweepBuffer; QVector<int> w0,wn,wX;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize,scrAvailSmp;
  bool threadActive;

  QFont chnFont,evtFont; QImage rBuffer,rBufferC; QTransform rTransform;
  QPainter sweepPainter,rotPainter; QVector<QStaticText> chnTextCache;

  int trigger;
};
