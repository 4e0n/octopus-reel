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

#pragma once

#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QStaticText>
#include "confparam.h"
#include "eegthread.h"

class EEGFrame : public QFrame {
 Q_OBJECT
 public:
  explicit EEGFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr) : QFrame(parent) {
   conf=c; ampNo=a; sweepSched=false;
   sweepBuffer=QImage(conf->sweepFrameW,conf->sweepFrameH,QImage::Format_RGB32);

   chnCount=conf->chns.size();
   colCount=std::ceil((float)(chnCount)/(float)(chnCount/2));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   chnY=(float)(conf->sweepFrameH-conf->audWaveH)/(float)(chnPerCol); // reserved vertical pixel count per channel
   int ww=(int)((float)(conf->sweepFrameW)/(float)colCount);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) { w0.append(colIdx*ww+1); }
   if (chnCount<16) chnFont=QFont("Helvetica",16,QFont::Bold);
   else if (chnCount>16 && chnCount<32) chnFont=QFont("Helvetica",14,QFont::Bold);
   else if (chnCount>96) chnFont=QFont("Helvetica",12);
   chnTextCache.clear(); chnTextCache.reserve(chnCount);
   for (unsigned int i=0;i<chnCount;i++) {
    int chnNo=conf->chns[i].physChn;
    const QString& chnName=conf->chns[i].chnName;
    QString label=QString::number(chnNo)+" "+chnName;
    QStaticText staticLabel(label); staticLabel.setTextFormat(Qt::PlainText);
    staticLabel.setTextWidth(-1);  // No width constraint
    chnTextCache.append(staticLabel);
   }
   audLabel.setText("AUDIO");
   eegThread=new EEGThread(conf,ampNo,&sweepBuffer,this);
   conf->threads[ampNo]=eegThread;
   connect(eegThread,&EEGThread::updateEEGFrame,this,QOverload<>::of(&EEGFrame::update));
   eegThread->start(QThread::HighestPriority);
  }

  bool sweepSched; QImage sweepBuffer; EEGThread *eegThread;

 protected:
  virtual void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QRect cr(0,0,conf->sweepFrameW-1,conf->sweepFrameH-1);
   mainPainter.begin(this);
   mainPainter.drawImage(0,0,sweepBuffer); sweepSched=false;
   mainPainter.setPen(Qt::black);
   mainPainter.drawRect(cr);
   // Channel names
   mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
   for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) { // 2 audio channels
    unsigned int colIdx=chnIdx/chnPerCol;
    scrCurY=(int)(-8+chnY/2.0+chnY*(chnIdx%chnPerCol));
    mainPainter.drawStaticText(w0[colIdx]+4,scrCurY,chnTextCache[chnIdx]);
    mainPainter.setPen(Qt::black);
    mainPainter.drawLine(width()/2,0,width()/2,height()-1);
   }
   mainPainter.setPen(Qt::blue);
   for (unsigned int colIdx=0;colIdx<colCount;colIdx++) {
    mainPainter.drawStaticText(w0[colIdx]+4,conf->sweepFrameH-conf->audWaveH/2,audLabel);
   }
   mainPainter.end();
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPainter mainPainter;
  unsigned int chnCount,colCount,chnPerCol,scrCurY; float chnY;
  QVector<int> w0; QVector<QStaticText> chnTextCache; QFont chnFont;
  QStaticText audLabel;
};
