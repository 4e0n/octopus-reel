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
#include <QPainter>
#include <QStaticText>
#include <QVector>
#include <thread>
#include "confparam.h"
#include "../sample.h"

class EEGThread : public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,QPixmap *sb=nullptr,QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; scrollBuffer=sb;

   chnCount=conf->chns.size();
   colCount=std::ceil((float)chnCount/(float)(33.));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)(conf->eegFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); wX.append(colIdx*ww+1); }
   for (unsigned int colIdx=0;colIdx<colCount-1;colIdx++) wn.append((colIdx+1)*ww-1);
   wn.append(conf->eegFrameW-1);

   if (chnCount<16) chnFont=QFont("Helvetica",12,QFont::Bold);
   else if (chnCount>16 && chnCount<32) chnFont=QFont("Helvetica",11,QFont::Bold);
   else if (chnCount>96) chnFont=QFont("Helvetica",10);

   evtFont=QFont("Helvetica",14,QFont::Bold);
   rTransform.rotate(-90);

   rBuffer=QPixmap(120,24); rBuffer.fill(Qt::transparent);
   rBufferC=QPixmap(); // cached - empty until first use

   resetScrollBuffer();

   chnTextCache.clear(); chnTextCache.reserve(chnCount);

   for (auto& chn:conf->chns) {
    const QString& chnName=chn.chnName;
    int chnNo=chn.physChn;
    QString label=QString::number(chnNo)+" "+chnName;
    QStaticText staticLabel(label); staticLabel.setTextFormat(Qt::PlainText);
    staticLabel.setTextWidth(-1);  // No width constraint
    chnTextCache.append(staticLabel);
   }
  }

  void resetScrollBuffer() {
   QRect cr(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   scrollBuffer->fill(Qt::white);
   scrollPainter.begin(scrollBuffer);
    scrollPainter.setPen(Qt::red);
    scrollPainter.drawRect(cr);
    scrollPainter.drawLine(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   scrollPainter.end();
  }

  void updateBuffer() {
   scrollPainter.begin(scrollBuffer);
    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Cursor vertical line for each column
    for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     if (wX[colIdx]<=wn[colIdx]) {
      scrollPainter.setPen(Qt::white);
      scrollPainter.drawLine(wX[colIdx],1,wX[colIdx],conf->eegFrameH-2);
      if (wX[colIdx]<(wn[colIdx]-1)) {
       scrollPainter.setPen(Qt::black);
       scrollPainter.drawLine(wX[colIdx]+1,1,wX[colIdx]+1,conf->eegFrameH-2);
      }
     }
    }

    //if (conf->tick) {
    // scrollPainter.setPen(Qt::darkGray); scrollPainter.drawRect(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
    // for (int colIdx=0;colIdx<colCount;colIdx++) {
    //  scrollPainter.drawLine(w0[colIdx]-1,1,w0[colIdx]-1,conf->eegFrameH-2);
    //  scrollPainter.drawLine(wX[colIdx],1,wX[colIdx],conf->eegFrameH-2);
    // }
    //}

    QVector<TcpSample> *tcpBuffer=&conf->tcpBuffer; unsigned int tcpBufSize=conf->tcpBufSize;
    unsigned int tcpBufTail=conf->tcpBufTail; unsigned int scrSmpCount=conf->scrSmpCount;

    chnY=(float)(conf->eegFrameH)/(float)(chnPerCol); // reserved vertical pixel count per channel

    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
     //int c=chnCount/chnPerCol;
     unsigned int curCol=chnIdx/chnPerCol;
     scrCurY=(int)(chnY/2.0+chnY*(chnIdx%chnPerCol));
     // Baseline
     scrollPainter.setPen(Qt::darkGray);
     scrollPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);

     scrollPainter.setPen(Qt::black);

     for (unsigned int smpIdx=0;smpIdx<scrSmpCount-1;smpIdx++) {
      std::vector<float> s0=(*tcpBuffer)[(tcpBufTail+smpIdx+0)%tcpBufSize].amp[ampNo].dataF;
      std::vector<float> s1=(*tcpBuffer)[(tcpBufTail+smpIdx+1)%tcpBufSize].amp[ampNo].dataF;
      scrollPainter.drawLine( // Div400 bcs. range is 400uVpp..
       wX[curCol]-1,scrCurY-(int)(s0[chnIdx]*chnY*conf->ampX[ampNo]),
       wX[curCol],  scrCurY-(int)(s1[chnIdx]*chnY*conf->ampX[ampNo])
      );
     }
    }

    if (conf->event) { conf->event=false; rBuffer.fill(Qt::transparent);

     rotPainter.begin(&rBuffer);
      rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
      rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
      if (conf->curEventType==1) penColor=Qt::blue; else if (conf->curEventType==2) penColor=Qt::red;
      rotPainter.setPen(penColor); rotPainter.drawText(2,9,conf->curEventName);
     rotPainter.end();
     rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);

     scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) {
      scrollPainter.drawLine(wX[colIdx]-1,1,wX[colIdx]-1,conf->eegFrameH-1);
      scrollPainter.drawPixmap(wX[colIdx]-15,conf->eegFrameH-104,rBufferC);
     }
    
     scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); scrollPainter.setPen(Qt::blue); // Line color
     QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount/chnPerCol);
     for (unsigned int curCol=0;curCol<chnCount/chnPerCol;curCol++) { int lineX=wX[curCol]-1;
      scrollPainter.drawLine(lineX,1,lineX,conf->eegFrameH-1);
      // Prepare batched label blitting - for source in pixmap
      fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->eegFrameH-104),
                                                        QRectF(0,0,rBufferC.width(),rBufferC.height())));
     }
     scrollPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
    } 

    // Main position increments of columns
    for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { wX[colIdx]++; if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx]; }

    // Channel names
    scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
     unsigned int curCol=chnIdx/chnPerCol;
     scrCurY=(int)(5+chnY/2.0+chnY*(chnIdx%chnPerCol));
     scrollPainter.drawStaticText(w0[curCol]+4,scrCurY,chnTextCache[chnIdx]);
    }

   //if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

   scrollPainter.end();
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
  
   //while (true) {
   while (!QThread::currentThread()->isInterruptionRequested()) {
    {
     QMutexLocker locker(&conf->mutex);
     // Sleep until producer signals
     while (!conf->scrollPending[ampNo]) {
      conf->scrollWait.wait(&conf->mutex);
     }

     updateBuffer();
     emit updateEEGFrame();
     //qDebug() << ampNo << "updated.";

     conf->scrollPending[ampNo]=false;
     conf->scrollersUpdating--; // semaphore.. kind of..  
     if (conf->scrollersUpdating==0) conf->tcpBufTail+=conf->scrSmpCount; // If this was the last scroller, then advance tail..
    }
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPixmap *scrollBuffer; QVector<int> w0,wn,wX;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QFont chnFont,evtFont; QPixmap rBuffer,rBufferC; QTransform rTransform;
  QPainter scrollPainter,rotPainter; QVector<QStaticText> chnTextCache;
};

#endif
