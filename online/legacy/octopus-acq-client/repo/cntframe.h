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

#ifndef CNTFRAME_H
#define CNTFRAME_H

#include <QFrame>
#include <QPainter>
#include <QBitmap>
#include <QMatrix>
#include <QStaticText>

#include "acqmaster.h"
#include "channel.h"

class CntFrame : public QFrame {
 Q_OBJECT
 public:
  CntFrame(QWidget *p,AcqMaster *acqm,unsigned int a) : QFrame(p) {
   parent=p; acqM=acqm; ampNo=a; scroll=false;

   chnCount=acqM->cntVisChns[ampNo].size();
   colCount=ceil((float)chnCount/(float)(33.));
   chnPerCol=ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)(acqM->acqFrameW)/(float)colCount);
   for (int i=0;i<colCount;i++) { w0.append(i*ww+1); wX.append(i*ww+1); }
   for (int i=0;i<colCount-1;i++) wn.append((i+1)*ww-1);
   wn.append(acqM->acqFrameW-1);

   if (chnCount<16) chnFont=QFont("Helvetica",12,QFont::Bold);
   else if (chnCount>16 && chnCount<32) chnFont=QFont("Helvetica",11,QFont::Bold);
   else if (chnCount>96) chnFont=QFont("Helvetica",10);

   evtFont=QFont("Helvetica",14,QFont::Bold);
   rTransform.rotate(-90);

   rBuffer=QPixmap(120,24); rBuffer.fill(Qt::transparent);
   rBufferC=QPixmap(); // cached - empty until first use

   scrollBuffer=QPixmap(acqM->acqFrameW,acqM->acqFrameH); resetScrollBuffer();

   chnTextCache.clear(); chnTextCache.reserve(chnCount);

   for (int i=0;i<chnCount;++i) {
    const QString& chName=acqM->acqChannels[ampNo][acqM->cntVisChns[ampNo][i]]->name;
    int chNo=acqM->acqChannels[ampNo][acqM->cntVisChns[ampNo][i]]->physChn+1;

    QString label=QString::number(chNo)+" "+chName;
    QStaticText staticLabel(label); staticLabel.setTextFormat(Qt::PlainText);
    staticLabel.setTextWidth(-1);  // No width constraint
    chnTextCache.append(staticLabel);
   }

   acqM->registerScrollHandler(this);
  }

  void resetScrollBuffer() { QPainter scrollPainter; QRect cr(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
   scrollBuffer.fill(Qt::white);
   scrollPainter.begin(&scrollBuffer);
    scrollPainter.setPen(Qt::red);
    scrollPainter.drawRect(cr);
    scrollPainter.drawLine(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
   scrollPainter.end();
  }
  
  void updateBuffer() { QPainter scrollPainter; int curCol;
   scrollPainter.begin(&scrollBuffer);
   scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
   
   for (c=0;c<colCount;c++) {
    if (wX[c]<=wn[c]) {
     scrollPainter.setPen(Qt::white); scrollPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2);
     if (wX[c]<(wn[c]-1)) { scrollPainter.setPen(Qt::black); scrollPainter.drawLine(wX[c]+1,1,wX[c]+1,acqM->acqFrameH-2); }
    }
   }

   if (tick) {
    scrollPainter.setPen(Qt::darkGray); scrollPainter.drawRect(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
    for (c=0;c<colCount;c++) { scrollPainter.drawLine(w0[c]-1,1,w0[c]-1,acqM->acqFrameH-2); scrollPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2); }
   }

   for (int i=0;i<chnCount;i++) {
    c=chnCount/chnPerCol; curCol=i/chnPerCol;
    chHeight=100.0; // 100 pixel is the indicated amp..
    chY=(float)(acqM->acqFrameH)/(float)(chnPerCol); scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));

    scrPrvDataX=acqM->scrPrvData[ampNo][i]; scrCurDataX=acqM->scrCurData[ampNo][i]; scrPrvDataFX=acqM->scrPrvDataF[ampNo][i]; scrCurDataFX=acqM->scrCurDataF[ampNo][i];

    scrollPainter.setPen(Qt::darkGray); //scrollPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);

    if (acqM->notch) {
     scrollPainter.setPen(Qt::black); scrollPainter.drawLine( // Div400 bcs. range is 400uVpp..
      wX[curCol]-1,scrCurY-(int)(scrPrvDataFX*chHeight*acqM->cntAmpX[ampNo]/4.0),wX[curCol],scrCurY-(int)(scrCurDataFX*chHeight*acqM->cntAmpX[ampNo]/4.0));
    } else {
     scrollPainter.setPen(Qt::black); scrollPainter.drawLine( // Div400 bcs. range is 400uVpp..
      wX[curCol]-1,scrCurY-(int)(scrPrvDataX *chHeight*acqM->cntAmpX[ampNo]/4.0),wX[curCol],scrCurY-(int)(scrCurDataX *chHeight*acqM->cntAmpX[ampNo]/4.0));
    }
   }

   if (event) { event=false; rBuffer.fill(Qt::transparent);
    rotPainter.begin(&rBuffer);
     rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
     rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
     if (acqM->curEventType==1) penColor=Qt::blue; else if (acqM->curEventType==2) penColor=Qt::red;
     rotPainter.setPen(penColor); rotPainter.drawText(2,9,acqM->curEventName);
    rotPainter.end();
    rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);

    //scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    //for (c=0;c<chnCount/chnPerCol;c++) {
    // scrollPainter.drawLine(wX[c]-1,1,wX[c]-1,acqM->acqFrameH-1);
    // scrollPainter.drawPixmap(wX[c]-15,acqM->acqFrameH-104,rBufferC);
    //}
    
    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); scrollPainter.setPen(Qt::blue); // Line color
    QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount / chnPerCol);
    for (c=0;c<chnCount/chnPerCol;c++) { int lineX=wX[c]-1;
     scrollPainter.drawLine(lineX,1,lineX,acqM->acqFrameH-1);
     // Prepare batched label blitting - for source in pixmap
     fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,acqM->acqFrameH-104),QRectF(0,0,rBufferC.width(),rBufferC.height())));
    }
    scrollPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
   }

   // Channel names
   scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
   for (int i=0;i<chnCount;++i) { curCol=i/chnPerCol;
    chY=(float)(acqM->acqFrameH)/(float)(chnPerCol); scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
    scrollPainter.drawStaticText(w0[curCol]+4,scrCurY,chnTextCache[i]);
   }

   if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

   // Main position increments of columns
   for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

   scrollPainter.end();
  }

 public slots: void slotScrData(bool t,bool e) { scroll=true; tick=t; event=e; updateBuffer(); repaint(); }
 
 protected:
  virtual void paintEvent(QPaintEvent*) {
   mainPainter.begin(this);
   mainPainter.drawPixmap(0,0,scrollBuffer);
   scroll=false;
   mainPainter.end();
  }

 private:
  QWidget *parent; AcqMaster *acqM; QString dummyString; QBrush bgBrush; QPainter mainPainter,rotPainter; QVector<int> w0,wn,wX;
  QFont evtFont,chnFont; QPixmap rBuffer,rBufferC; QTransform rTransform; float chHeight,chY,scrPrvDataX,scrCurDataX,scrPrvDataFX,scrCurDataFX; int scrCurY;
  bool scroll,tick,event; int c,chnCount,colCount,chnPerCol; QPixmap scrollBuffer; unsigned int ampNo;

  QVector<QStaticText> chnTextCache;
};

#endif
