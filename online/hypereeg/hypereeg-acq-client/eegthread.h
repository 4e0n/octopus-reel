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

class EEGThread : public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,QPixmap *sb=nullptr,QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; scrollBuffer=sb;

   chnCount=conf->chns.size();
   colCount=std::ceil((float)chnCount/(float)(33.));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)(conf->eegFrameW)/(float)colCount);
   for (int i=0;i<colCount;i++) { w0.append(i*ww+1); wX.append(i*ww+1); }
   for (int i=0;i<colCount-1;i++) wn.append((i+1)*ww-1);
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

   for (int i=0;i<chnCount;++i) {
    const QString& chName=conf->chns[i].chnName;
    int chNo=conf->chns[i].physChn;
    QString label=QString::number(chNo)+" "+chName;
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

  void updateBuffer() { int curCol;
   scrollPainter.begin(scrollBuffer);
    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Cursor vertical line for each column
    for (c=0;c<colCount;c++) {
     if (wX[c]<=wn[c]) {
      scrollPainter.setPen(Qt::white); scrollPainter.drawLine(wX[c],1,wX[c],conf->eegFrameH-2);
      if (wX[c]<(wn[c]-1)) { scrollPainter.setPen(Qt::black); scrollPainter.drawLine(wX[c]+1,1,wX[c]+1,conf->eegFrameH-2); }
     }
    }

    if (conf->tick) {
     scrollPainter.setPen(Qt::darkGray); scrollPainter.drawRect(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
     for (c=0;c<colCount;c++) {
      scrollPainter.drawLine(w0[c]-1,1,w0[c]-1,conf->eegFrameH-2);
      scrollPainter.drawLine(wX[c],1,wX[c],conf->eegFrameH-2);
     }
    }

    for (int i=0;i<chnCount;i++) {
     c=chnCount/chnPerCol; curCol=i/chnPerCol;
     chHeight=100.0; // 100 pixel is the indicated amp..
     chY=(float)(conf->eegFrameH)/(float)(chnPerCol); scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));

     // Baseline
     scrollPainter.setPen(Qt::darkGray);
     //scrollPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);

     scrollPainter.setPen(Qt::black);
     scrollPainter.drawLine( // Div400 bcs. range is 400uVpp..
      wX[curCol]-1,
      scrCurY-(int)(conf->scrPrvDataF[ampNo][i]*2e5), // *chHeight*conf->eegAmpX[ampNo]/4.0),
      wX[curCol],
      scrCurY-(int)(conf->scrCurDataF[ampNo][i]*2e5) // *chHeight*conf->eegAmpX[ampNo]/4.0)
     );
    }

    if (conf->event) { conf->event=false; rBuffer.fill(Qt::transparent);
     rotPainter.begin(&rBuffer);
      rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
      rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
      if (conf->curEventType==1) penColor=Qt::blue; else if (conf->curEventType==2) penColor=Qt::red;
      rotPainter.setPen(penColor); rotPainter.drawText(2,9,conf->curEventName);
     rotPainter.end();
     rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);

     //scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
     //for (c=0;c<chnCount/chnPerCol;c++) {
     // scrollPainter.drawLine(wX[c]-1,1,wX[c]-1,conf->eegFrameH-1);
     // scrollPainter.drawPixmap(wX[c]-15,conf->eegFrameH-104,rBufferC);
     //}
    
     scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); scrollPainter.setPen(Qt::blue); // Line color
     QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount / chnPerCol);
     for (c=0;c<chnCount/chnPerCol;c++) { int lineX=wX[c]-1;
      scrollPainter.drawLine(lineX,1,lineX,conf->eegFrameH-1);
      // Prepare batched label blitting - for source in pixmap
      fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->eegFrameH-104),
                                                        QRectF(0,0,rBufferC.width(),rBufferC.height())));
     }
     scrollPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
    } 

    // Channel names
    scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
    for (int i=0;i<chnCount;++i) { curCol=i/chnPerCol;
     chY=(float)(conf->eegFrameH)/(float)(chnPerCol); scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
     scrollPainter.drawStaticText(w0[curCol]+4,scrCurY,chnTextCache[i]);
    }

    if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

    // Main position increments of columns
    for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }
   scrollPainter.end();
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
   while (true) {
    //QMutexLocker locker(&(conf->mutex));
    //conf->mutex.lock();
    if (conf->updateInstant) {
     updateBuffer();
     emit updateEEGFrame();
     conf->updated[ampNo]=true;
     //qDebug() << "Updating buffer";
    }
    bool allUpdated=true;
    for (unsigned int i=0;i<conf->ampCount;i++) if (conf->updated[i]==false) { allUpdated=false; break; }
    if (allUpdated){
     //qDebug() << "ALL UPDATED!";
     conf->updateInstant=false;
     //conf->mutex.unlock();
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(5));
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPixmap *scrollBuffer; QVector<int> w0,wn,wX;
  int chnCount,colCount,chnPerCol,c,scrCurY; float chHeight,chY;
  QFont chnFont,evtFont; QPixmap rBuffer,rBufferC; QTransform rTransform;
  QPainter scrollPainter,rotPainter; QVector<QStaticText> chnTextCache;
};

#endif
