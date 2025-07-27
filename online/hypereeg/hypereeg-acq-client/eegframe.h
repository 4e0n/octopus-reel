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

#ifndef EEGFRAME_H
#define EEGFRAME_H

#include <QFrame>
#include <QPainter>
#include <QStaticText>
#include "confparam.h"
#include "audioframe.h"
#include "eegthread.h"

class EEGFrame : public QFrame {
 Q_OBJECT
 public:
  explicit EEGFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr) : QFrame(parent) {
   conf=c; ampNo=a; scrollBuffer=QPixmap(conf->eegFrameW,conf->eegFrameH);

   chnCount=conf->chns.size(); colCount=std::ceil((float)chnCount/(float)(33)); // 33 !?
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   chnY=(float)(conf->eegFrameH)/(float)(chnPerCol); // reserved vertical pixel count per channel

   int ww=(int)((float)(conf->eegFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); wX.append(colIdx*ww+1); }
   for (unsigned int colIdx=0;colIdx<colCount-1;colIdx++) wn.append((colIdx+1)*ww-1);
   wn.append(conf->eegFrameW-1);

   evtFont=QFont("Helvetica",14,QFont::Bold); rTransform.rotate(-90);
   rBuffer=QPixmap(120,24); rBuffer.fill(Qt::transparent);
   rBufferC=QPixmap(); // cached - empty until first use

   if (chnCount<16) chnFont=QFont("Helvetica",12,QFont::Bold);
   else if (chnCount>16 && chnCount<32) chnFont=QFont("Helvetica",11,QFont::Bold);
   else if (chnCount>96) chnFont=QFont("Helvetica",10);

   chnTextCache.clear(); chnTextCache.reserve(chnCount);

   for (auto& chn:conf->chns) {
    const QString& chnName=chn.chnName;
    int chnNo=chn.physChn;
    QString label=QString::number(chnNo)+" "+chnName;
    QStaticText staticLabel(label); staticLabel.setTextFormat(Qt::PlainText);
    staticLabel.setTextWidth(-1);  // No width constraint
    chnTextCache.append(staticLabel);
   }

   scrAvailSmp=conf->scrAvailableSamples;

   resetScrollBuffer();

   eegThread=new EEGThread(conf,ampNo,this);
   conf->threads[ampNo]=eegThread;
   connect(eegThread,&EEGThread::updateEEGFrame,this,&EEGFrame::updateSlot);
   eegThread->start(QThread::HighestPriority);
  }

 public slots:
  void updateSlot() {
   updateBuffer();
   update();
  }

 protected:
  virtual void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QRect cr(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   mainPainter.begin(this);
   mainPainter.drawPixmap(0,0,scrollBuffer);
   mainPainter.setPen(Qt::black);
   mainPainter.drawRect(cr);
   // Channel names
   mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
   for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
    unsigned int colIdx=chnIdx/chnPerCol;
    scrCurY=(int)(-8+chnY/2.0+chnY*(chnIdx%chnPerCol));
    mainPainter.drawStaticText(w0[colIdx]+4,scrCurY,chnTextCache[chnIdx]);
   }
   mainPainter.end();
  }

 private:
  void resetScrollBuffer() {
   QRect cr(0,0,conf->eegFrameW-1,conf->eegFrameH-1); scrollBuffer.fill(Qt::white);
  }

  void updateBuffer() {
   unsigned int scrUpdateSmp=conf->scrUpdateSamples;
   unsigned int scrUpdateCount=scrAvailSmp/scrUpdateSmp;

   scrollPainter.begin(&scrollBuffer);
    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (conf->scrollMode) { // Scroll contents left by scrUpdateCount pixels
     for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
      scrollBuffer.scroll(-scrUpdateCount,0,scrollBuffer.rect()); // Scroll contents left by scrUpdateCount pixels
      wX[colIdx]=wn[colIdx]-scrUpdateCount; // Draw new segment on right
     }
    } else { // SweepMode: draw on moving cursor
     //for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
     // //if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx]; // Check for end of screen
//
//
//     }
 ;   }

    for (unsigned int smpIdx=0;smpIdx<scrUpdateCount-1;smpIdx++) {
     for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
      unsigned int colIdx=chnIdx/chnPerCol;
      scrCurY=(int)(chnY/2.0+chnY*(chnIdx%chnPerCol));

      // Cursor vertical line for each column
      if (wX[colIdx]<=wn[colIdx]) {
       scrollPainter.setPen(Qt::white); scrollPainter.drawLine(wX[colIdx],0,wX[colIdx],conf->eegFrameH-1);
       if (wX[colIdx]<(wn[colIdx]-1)) {
        scrollPainter.setPen(Qt::black); scrollPainter.drawLine(wX[colIdx]+1,0,wX[colIdx]+1,conf->eegFrameH-1);
       }
      }

      // Baselines
      //scrollPainter.setPen(Qt::darkGray);
      //scrollPainter.drawLine(wX[colIdx]-1,scrCurY,wX[colIdx],scrCurY);
      // Data
 
      int mu0=scrCurY-(int)((conf->s0)[smpIdx][chnIdx]*chnY*conf->ampX[ampNo]);
      int mu1=scrCurY-(int)((conf->s1)[smpIdx][chnIdx]*chnY*conf->ampX[ampNo]);
      int sigma0=(int)((conf->s0s)[smpIdx][chnIdx]*chnY*conf->ampX[ampNo]*0.5);
      int sigma1=(int)((conf->s1s)[smpIdx][chnIdx]*chnY*conf->ampX[ampNo]*0.5);

      scrollPainter.setPen(Qt::blue);
      scrollPainter.drawLine(wX[colIdx]-1,mu0-sigma0,wX[colIdx]-1,mu0+sigma0);
      scrollPainter.drawLine(wX[colIdx],  mu1-sigma1,wX[colIdx],  mu1+sigma1);

      scrollPainter.setPen(Qt::black);
      scrollPainter.drawLine(wX[colIdx]-1,mu0,wX[colIdx],mu1);
     }

     for (int colIdx=0;colIdx<wX.size();colIdx++) {
      wX[colIdx]++; if (wX[colIdx]>wn[colIdx]) wX[colIdx]=w0[colIdx]; // Check for end of screen
     }

    }


    // Upcoming event
//    if (conf->event) { conf->event=false; rBuffer.fill(Qt::transparent);
//
//     rotPainter.begin(&rBuffer);
//      rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
//      rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
//      if (conf->curEventType==1) penColor=Qt::blue; else if (conf->curEventType==2) penColor=Qt::red;
//      rotPainter.setPen(penColor); rotPainter.drawText(2,9,conf->curEventName);
//     rotPainter.end();
//     rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);
//
//     scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
//     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) {
//      scrollPainter.drawLine(wX[colIdx]-1,1,wX[colIdx]-1,conf->eegFrameH-1);
//      scrollPainter.drawPixmap(wX[colIdx]-15,conf->eegFrameH-104,rBufferC);
//     }
//    
//     scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); scrollPainter.setPen(Qt::blue); // Line color
//     QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount/chnPerCol);
//     for (unsigned int colIdx=0;colIdx<chnCount/chnPerCol;colIdx++) { int lineX=wX[colIdx]-1;
//      scrollPainter.drawLine(lineX,1,lineX,conf->eegFrameH-1);
//      // Prepare batched label blitting - for source in pixmap
//      fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->eegFrameH-104),
//                                                        QRectF(0,0,rBufferC.width(),rBufferC.height())));
//     }
//     scrollPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
//    } 

//    if (!conf->scrollMode) { // Scroll contents left by scrUpdateCount pixels
//     for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
//      // Main position increments of columns
//      //wX[colIdx]+=conf->scrUpdateCount;
//      wX[colIdx]++;
//     }
//    }

    //if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

   scrollPainter.end();
  }

  ConfParam *conf; EEGThread *eegThread; unsigned int ampNo;
  QPainter mainPainter,scrollPainter,rotPainter;
  QPixmap scrollBuffer,rBuffer,rBufferC; QTransform rTransform;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<QStaticText> chnTextCache; QFont chnFont,evtFont;

  unsigned int scrAvailSmp;

  QVector<int> w0,wn,wX;
};

#endif
