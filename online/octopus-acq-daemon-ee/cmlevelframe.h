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

#ifndef CMLEVELFRAME_H
#define CMLEVELFRAME_H

#include <QFrame>
#include <QPainter>
#include <QBitmap>
#include <QMatrix>

#include "acqdaemon.h"

static int f32Idx[32]={4,5,6,19,21,23,25,27,29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,68,76,78, \
                       41,41,41};
static int s32Idx[32]={10,12,16,18,20,22,24,26,30,32,34,38,40,42,44,48,52,56,58,60,62,65,66,70,71, \
	               28,36,46,54,64,72,77};

class CMLevelFrame : public QFrame {
 Q_OBJECT
 public:
  CMLevelFrame(QWidget *p,AcqDaemon *acqd,unsigned int a) : QFrame(p) {
   parent=p; acqD=acqd; ampNo=a;
   cmBuffer=new QPixmap(900,900);
   for (int i=0;i<32;i++) first32Idx.append(f32Idx[i]-1);
   for (int i=0;i<32;i++) second32Idx.append(s32Idx[i]-1);

//   chnCount=acqD->cntVisChns[ampNo].size();
//   colCount=ceil((float)chnCount/(float)(33.));
//   chnPerCol=ceil((float)(chnCount)/(float)(colCount));
//   int ww=(int)((float)(acqD->acqFrameW)/(float)colCount);
//   for (int i=0;i<colCount;i++) { w0.append(i*ww+1); wX.append(i*ww+1); }
//   for (int i=0;i<colCount-1;i++) wn.append((i+1)*ww-1); wn.append(acqD->acqFrameW-1);
//
//   if (chnCount<16)
//    chnFont=QFont("Helvetica",12,QFont::Bold);
//   else if (chnCount>16 && chnCount<32)
//    chnFont=QFont("Helvetica",11,QFont::Bold);
//   else if (chnCount>96)
//    chnFont=QFont("Helvetica",10);
//   evtFont=QFont("Helvetica",8,QFont::Bold);
//   rMatrix.rotate(-90);
//
//   cmBuffer=QPixmap(acqD->acqFrameW,acqD->acqFrameH); resetScrollBuffer();

   acqD->registerCMLevelHandler(this);
  }

  void resetBuffer() {
   QPainter cmPainter; QRect cr(0,0,width(),height());
   cmBuffer->fill(Qt::black);
   cmPainter.begin(cmBuffer);
    cmPainter.setBackground(QBrush(Qt::black));
    cmPainter.setPen(Qt::black);
   // cmPainter.setBrush(Qt::black);
    cmPainter.fillRect(cr,QBrush(Qt::black));
    cmPainter.drawRect(cr);
    int rowSz=9; int cSz=width()/rowSz;
    for (int i=0;i<first32Idx.length();i++) {
     int x=first32Idx[i]%9; int y=first32Idx[i]/9;
     QRect fs32Rect(x*cSz,y*cSz,cSz,cSz);
     cmPainter.fillRect(fs32Rect,QBrush(Qt::gray));
    }
    for (int i=0;i<second32Idx.length();i++) {
     int x=second32Idx[i]%9; int y=second32Idx[i]/9;
     QRect fs32Rect(x*cSz,y*cSz,cSz,cSz);
     cmPainter.fillRect(fs32Rect,QBrush(Qt::lightGray));
    }
    for (int i=1;i<rowSz;i++) {
     cmPainter.drawLine(0,(i)*cSz,width(),(i)*cSz); cmPainter.drawLine((i)*cSz,0,(i)*cSz,height());
    }
   cmPainter.end();
  }
  
  void updateBuffer() {
   QPainter cmPainter; int curCol;
   cmPainter.begin(cmBuffer);
   
 //  for (c=0;c<colCount;c++) {
 //   if (wX[c]<=wn[c]) {
 //    cmPainter.setPen(Qt::white); //white
 //    cmPainter.drawLine(wX[c],1,wX[c],acqD->acqFrameH-2);
 //    if (wX[c]<(wn[c]-1)) {
 //     cmPainter.setPen(Qt::black);
 //     cmPainter.drawLine(wX[c]+1,1,wX[c]+1,acqD->acqFrameH-2);
 //    }
 //   }
 //  }

 //  for (int i=0;i<chnCount;i++) {
 //   c=chnCount/chnPerCol; curCol=i/chnPerCol;
 //   chHeight=100.0; // 100 pixel is the indicated amp..
 //   chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
 //   scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//
//    scrPrvDataX=acqD->scrPrvData[ampNo][i]; scrCurDataX=acqD->scrCurData[ampNo][i];
//    scrPrvDataFX=acqD->scrPrvDataF[ampNo][i]; scrCurDataFX=acqD->scrCurDataF[ampNo][i];
//
//    cmPainter.setPen(Qt::darkGray);
//    //cmPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);
//
//    if (acqD->notch) {
//    cmPainter.setPen(Qt::black); // Filtered in Red
//    cmPainter.drawLine( // Div400 bcs. range is 400uVpp..
//     wX[curCol]-1,scrCurY-(int)(scrPrvDataFX*chHeight*acqD->cntAmpX[ampNo]/4.0),
//     wX[curCol],scrCurY-(int)(scrCurDataFX*chHeight*acqD->cntAmpX[ampNo]/4.0));
//    } else {
//    cmPainter.setPen(Qt::black);
//    cmPainter.drawLine( // Div400 bcs. range is 400uVpp..
//     wX[curCol]-1,scrCurY-(int)(scrPrvDataX*chHeight*acqD->cntAmpX[ampNo]/4.0),
//     wX[curCol],scrCurY-(int)(scrCurDataX*chHeight*acqD->cntAmpX[ampNo]/4.0));
//    }
//      if (i==0) qDebug("%2.2f",scrCurDataX);
//   }

//   if (event) { event=false;
//    rBuffer=new QPixmap(100,12); rBuffer->fill(Qt::white);
//    rotPainter.begin(rBuffer);
//     rotPainter.setFont(evtFont);
//     rotPainter.setPen(Qt::darkGreen); // Erroneous event!
//     if (acqD->curEventType==1) rotPainter.setPen(Qt::blue);
//     else if (acqD->curEventType==2) rotPainter.setPen(Qt::red);
//     rotPainter.drawText(2,9,acqD->curEventName);
//    rotPainter.end();
//    *rBuffer=rBuffer->transformed(rMatrix,Qt::SmoothTransformation);

//    for (c=0;c<colCount;c++) {
//     cmPainter.drawLine(wX[c],1,wX[c],acqD->acqFrameH-2);
//     if (wX[c]<(w0[c]+15)) cmPainter.drawPixmap(wn[c]-15,acqD->acqFrameH-104,*rBuffer);
//     else cmPainter.drawPixmap(wX[c]-14,acqD->acqFrameH-104,*rBuffer);
//    } delete rBuffer;
//   } 

//    cmPainter.setPen(QColor(50,50,150)); cmPainter.setFont(chnFont);
//    for (int i=0;i<chnCount;i++) {
//     c=chnCount/chnPerCol; curCol=i/chnPerCol;
//     chHeight=100.0; // 100 pixel is the indicated amp..
//     chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
//     scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
//     cmPainter.drawText(w0[curCol]+4,scrCurY,
//      dummyString.setNum((acqD->acqChannels)[ampNo][acqD->cntVisChns[ampNo][i]]->physChn+1)+" "+
//                         (acqD->acqChannels)[ampNo][acqD->cntVisChns[ampNo][i]]->name);
//    }

//   if (wX[0]==150) cmPainter.drawLine(149,300,149,400); // Amplitude legend

//     if (tick) { tick=false;
//      for (int i=0;i<chnCount;i++) { // 50Hz Levels
//       c=chnCount/chnPerCol; curCol=i/chnPerCol;
//       chHeight=100.0; // 100 pixel is the indicated amp..
//       chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
//       scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//      }
//     }

     // Main position increments of columns
//     for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

   cmPainter.end();
  }

 public slots:
  void slotCMLevelsReady() {
   //qDebug() << "CM Level update" << ampNo;
//   scroll=true; tick=t; event=e;
   updateBuffer();
   repaint();
  }

 protected:
  virtual void paintEvent(QPaintEvent*) {
   resetBuffer();
   mainPainter.begin(this);
    int curCol;
    mainPainter.setBackgroundMode(Qt::TransparentMode);
    mainPainter.drawPixmap(0,0,*cmBuffer);
   mainPainter.end();
  }

 protected:
//  void resizeEvent(QResizeEvent *event) {
//	  ;
   //resizeEvent(event); // Call base class event handler
   //setGeometry(x(),y(),parent->width()-9,parent->width()-60);

   //int width = event->size().width();
   //int height = event->size().height();

   // Resize child widgets proportionally
   //label1->setGeometry(width / 4, height / 4, width / 2, 30);
   //label2->setGeometry(width / 4, height / 2, width / 2, 30);
//  }

 private:
  QWidget *parent; AcqDaemon *acqD; QString dummyString;
  QBrush bgBrush; QPainter mainPainter; QVector<int> w0,wn,wX;
//  QFont evtFont,chnFont; QPixmap *rBuffer;
//  float chHeight,chY,scrPrvDataX,scrCurDataX,scrPrvDataFX,scrCurDataFX; int scrCurY;
//  bool scroll,tick,event;
//  int c,chnCount,colCount,chnPerCol;
  QPixmap *cmBuffer;
  unsigned int ampNo;
  QVector<int> first32Idx,second32Idx;
};

#endif
