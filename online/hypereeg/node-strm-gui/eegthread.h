/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#ifndef EEGTHREAD_H
#define EEGTHREAD_H

#include <QThread>
#include <QMutex>
#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QStaticText>
#include <QVector>
#include <thread>
#include "confparam.h"
#include "audioframe.h"
#include "../sample.h"

class EEGThread : public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,QImage *sb=nullptr,//QImage *asb=nullptr,
                     QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; guiBuffer=sb; threadActive=true;

   chnCount=conf->chns.size();
   colCount=std::ceil((float)chnCount/(float)(33.));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)(conf->eegFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); wX.append(colIdx*ww+1); }
   for (unsigned int colIdx=0;colIdx<colCount-1;colIdx++) wn.append((colIdx+1)*ww-1);
   wn.append(conf->eegFrameW-1);

   tcpBuffer=&conf->tcpBuffer; tcpBufSize=conf->tcpBufSize; scrAvailSmp=conf->scrAvailableSamples;
   chnY=(float)(conf->eegFrameH)/(float)(chnPerCol); // reserved vertical pixel count per channel

   evtFont=QFont("Helvetica",14,QFont::Bold);
   rTransform.rotate(-90);

   rBuffer=QImage(120,24,QImage::Format_RGB32); rBuffer.fill(Qt::transparent);
   rBufferC=QImage(); // cached - empty until first use

   resetScrollBuffer();
  }

  void resetScrollBuffer() {
   QRect cr(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   guiBuffer->fill(Qt::white);
  }

  void mean(unsigned int startIdx,std::vector<float> *mu,std::vector<float> *sigma) {
   unsigned int tcpBufTail=conf->tcpBufTail; unsigned int scrUpdateSmp=conf->scrUpdateSamples;
   const float invSmp=1.0f/static_cast<float>(scrUpdateSmp);
   if (conf->notchActive) {
    for (unsigned int chnIndex=0;chnIndex<chnCount;chnIndex++) {
     float sum=0.0f,sumSq=0.0f;
     for (unsigned int smpIndex=0;smpIndex<scrUpdateSmp;smpIndex++) {
      float data=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize].amp[ampNo].dataF[chnIndex];
      sum+=data; sumSq+=data*data;
     }
     float m=sum*invSmp;
     (*mu)[chnIndex]=m; (*sigma)[chnIndex]=std::sqrt(sumSq*invSmp-m*m);
    }
   } else {
    for (unsigned int chnIndex=0;chnIndex<chnCount;chnIndex++) {
     float sum=0.0f,sumSq=0.0f;
     for (unsigned int smpIndex=0;smpIndex<scrUpdateSmp;smpIndex++) {
      float data=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize].amp[ampNo].data[chnIndex];
      sum+=data; sumSq+=data*data;
     }
     float m=sum*invSmp;
     (*mu)[chnIndex]=m; (*sigma)[chnIndex]=std::sqrt(sumSq*invSmp-m*m);
    }
   }
  }
  
  void updateBuffer() {
   unsigned int scrUpdateSmp=conf->scrUpdateSamples;
   unsigned int scrUpdateCount=scrAvailSmp/scrUpdateSmp;

   guiPainter.begin(guiBuffer);
    guiPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (conf->scrollMode) { // Scroll contents left by scrUpdateCount pixels
     //for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     // guiBuffer->scroll(-scrUpdateCount,0,guiBuffer->rect()); // Scroll contents left by scrUpdateCount pixels
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
     mean((smpIdx+0)*scrUpdateSmp,&s0,&s0s); mean((smpIdx+1)*scrUpdateSmp,&s1,&s1s);

     for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
      unsigned int colIdx=chnIdx/chnPerCol;
      scrCurY=(int)(chnY/2.0+chnY*(chnIdx%chnPerCol));

      // Cursor vertical line for each column
      if (wX[colIdx]<=wn[colIdx]) {
       guiPainter.setPen(Qt::white); guiPainter.drawLine(wX[colIdx],0,wX[colIdx],conf->eegFrameH-1);
       if (wX[colIdx]<(wn[colIdx]-1)) {
        guiPainter.setPen(Qt::black); guiPainter.drawLine(wX[colIdx]+1,0,wX[colIdx]+1,conf->eegFrameH-1);
       }
      }

      // Baselines
      //guiPainter.setPen(Qt::darkGray);
      //guiPainter.drawLine(wX[colIdx]-1,scrCurY,wX[colIdx],scrCurY);
      // Data
 
      int mu0=scrCurY-(int)(s0[chnIdx]*chnY*conf->ampX[ampNo]);
      int mu1=scrCurY-(int)(s1[chnIdx]*chnY*conf->ampX[ampNo]);
      int sigma0=(int)(s0s[chnIdx]*chnY*conf->ampX[ampNo]*0.5);
      int sigma1=(int)(s1s[chnIdx]*chnY*conf->ampX[ampNo]*0.5);

      guiPainter.setPen(Qt::blue);
      guiPainter.drawLine(wX[colIdx]-1,mu0-sigma0,wX[colIdx]-1,mu0+sigma0);
      guiPainter.drawLine(wX[colIdx],  mu1-sigma1,wX[colIdx],  mu1+sigma1);

      guiPainter.setPen(Qt::black);
      guiPainter.drawLine(wX[colIdx]-1,mu0,wX[colIdx],mu1);
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

     guiPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) {
      guiPainter.drawLine(wX[colIdx]-1,1,wX[colIdx]-1,conf->eegFrameH-1);
      guiPainter.drawPixmap(wX[colIdx]-15,conf->eegFrameH-104,rBufferC);
     }
    
     guiPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); guiPainter.setPen(Qt::blue); // Line color
     QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount/chnPerCol);
     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) { int lineX=wX[colIdx]-1;
      guiPainter.drawLine(lineX,1,lineX,conf->eegFrameH-1);
      // Prepare batched label blitting - for source in pixmap
      fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->eegFrameH-104),
                                                        QRectF(0,0,rBufferC.width(),rBufferC.height())));
     }
     guiPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
    } 
*/
    // Channel names
//    guiPainter.setPen(QColor(50,50,150)); guiPainter.setFont(chnFont);
//    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
//     unsigned int colIdx=chnIdx/chnPerCol;
//     scrCurY=(int)(-8+chnY/2.0+chnY*(chnIdx%chnPerCol));
//     guiPainter.drawStaticText(w0[colIdx]+4,scrCurY,chnTextCache[chnIdx]);
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
    //if (wX[0]==150) guiPainter.drawLine(149,300,149,400); // Amplitude legend

   guiPainter.end();
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
  
   //while (true) {
   while (threadActive) { // && !QThread::currentThread()->isInterruptionRequested()) {
    {
     QMutexLocker locker(&conf->mutex);
     // Sleep until producer signals
     while (!conf->guiPending[ampNo]) { conf->guiWait.wait(&conf->mutex); }

     if (conf->quitPending) {
      threadActive=false; //qDebug() << "Thread got the finish signal:" << ampNo;
     } else {
      updateBuffer();
      emit updateEEGFrame(); //qDebug() << ampNo << "updated.";
     }
     conf->guiPending[ampNo]=false;
     conf->guiUpdating--; // semaphore.. kind of..  
     // If this was the last scroller, then advance tail..
     if (conf->guiUpdating==0) conf->tcpBufTail+=conf->scrAvailableSamples;
     //if (!conf->quitPending)
    }
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  ConfParam *conf; unsigned int ampNo; QImage *guiBuffer; QVector<int> w0,wn,wX;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize,scrAvailSmp;
  bool threadActive;

  QFont chnFont,evtFont; QImage rBuffer,rBufferC; QTransform rTransform;
  QPainter guiPainter,rotPainter; QVector<QStaticText> chnTextCache;
};

#endif
