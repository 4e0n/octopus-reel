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

#include "acqmaster.h"
#include "channel.h"

class CntFrame : public QFrame {
 Q_OBJECT
 public:
  CntFrame(QWidget *p,AcqMaster *acqm,unsigned int a) : QFrame(p) {
   parent=p; acqM=acqm; ampNo=a; scroll=false;

   //setAttribute(Qt::WA_NativeWindow,true);
   //setAttribute(Qt::WA_PaintOnScreen,true);
   //setAttribute(Qt::WA_NoSystemBackground,true);
   //extern void qt_x11_set_global_double_buffer(bool);
   //qt_x11_set_global_double_buffer(false);

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

   evtFont=QFont("Helvetica",8,QFont::Bold);
   rTransform.rotate(-90);

   scrollBuffer=QPixmap(acqM->acqFrameW,acqM->acqFrameH); resetScrollBuffer();

   acqM->registerScrollHandler(this);
  }

  void resetScrollBuffer() { QPainter scrollPainter; QRect cr(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
   scrollBuffer.fill(Qt::white);
   scrollPainter.begin(&scrollBuffer);
    //scrollPainter.setBackgroundMode(Qt::OpaqueMode);
    scrollPainter.setPen(Qt::red);
   // scrollPainter.setBrush(Qt::black);
   // scrollPainter.setBackground(QBrush(Qt::white));
   // scrollPainter.fillRect(cr,QBrush(Qt::white));
    scrollPainter.drawRect(cr);
    scrollPainter.drawLine(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
   scrollPainter.end();
  }
  
  void updateBuffer() { QPainter scrollPainter; int curCol;
   
   scrollPainter.begin(&scrollBuffer);
   
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

   if (event) { event=false; rBuffer=new QPixmap(100,12); rBuffer->fill(Qt::white);
    rotPainter.begin(rBuffer);
     rotPainter.setFont(evtFont); rotPainter.setPen(Qt::darkGreen); // Erroneous event!
     if (acqM->curEventType==1) rotPainter.setPen(Qt::blue); else if (acqM->curEventType==2) rotPainter.setPen(Qt::red);
     rotPainter.drawText(2,9,acqM->curEventName);
    rotPainter.end();
    *rBuffer=rBuffer->transformed(rTransform,Qt::SmoothTransformation);

    for (c=0;c<colCount;c++) {
     scrollPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2);
     if (wX[c]<(w0[c]+15)) scrollPainter.drawPixmap(wn[c]-15,acqM->acqFrameH-104,*rBuffer); else scrollPainter.drawPixmap(wX[c]-14,acqM->acqFrameH-104,*rBuffer);
    } delete rBuffer;
   } 

//   if (chnCount>32 && wX[0]==70) { // Overhead and the Chn names together..
   scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
   for (int i=0;i<chnCount;i++) {
    c=chnCount/chnPerCol; curCol=i/chnPerCol; chHeight=100.0; // 100 pixel is the indicated amp..
    chY=(float)(acqM->acqFrameH)/(float)(chnPerCol); scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
    scrollPainter.drawText(w0[curCol]+4,scrCurY,
     dummyString.setNum((acqM->acqChannels)[ampNo][acqM->cntVisChns[ampNo][i]]->physChn+1)+" "+(acqM->acqChannels)[ampNo][acqM->cntVisChns[ampNo][i]]->name);
   }
//   } else if (chnCount<=32 && wX[0]<70) { // We have CPU time..
//    scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
//    for (int i=0;i<chnCount;i++) {
//     c=chnCount/chnPerCol; curCol=i/chnPerCol;
//     chHeight=100.0; // 100 pixel is the indicated amp..
//     chY=(float)(acqM->acqFrameH)/(float)(chnPerCol);
//     scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
//     scrollPainter.drawText(w0[curCol]+4,scrCurY,
//      dummyString.setNum((acqM->acqChannels)[acqM->cntVisChns[i]]->physChn+1)+" "+
//                         (acqM->acqChannels)[acqM->cntVisChns[i]]->name);
//    }
//   }

   if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

//     if (tick) { tick=false;
//      for (int i=0;i<chnCount;i++) { // 50Hz Levels
//       c=chnCount/chnPerCol; curCol=i/chnPerCol;
//       chHeight=100.0; // 100 pixel is the indicated amp..
//       chY=(float)(acqM->acqFrameH)/(float)(chnPerCol);
//       scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//      }
//     }

     // Main position increments of columns
   for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

   scrollPainter.end();
  }

 public slots: void slotScrData(bool t,bool e) { scroll=true; tick=t; event=e; updateBuffer(); repaint(); }
 
 protected:
  virtual void paintEvent(QPaintEvent*) {
   mainPainter.begin(this);
   mainPainter.setBackgroundMode(Qt::TransparentMode); mainPainter.drawPixmap(0,0,scrollBuffer);
/*
    QRect cr=contentsRect(); cr.setWidth(cr.width()-1); cr.setHeight(cr.height()-1);

    mainPainter.setPen(Qt::red);
    mainPainter.setBrush(Qt::white);
    mainPainter.setBackground(QBrush(Qt::white));
    mainPainter.fillRect(contentsRect(),QBrush(Qt::white));
    mainPainter.drawRect(cr);

    mainPainter.setClipping(false);
    // ***         REPAINT REQUESTS NOT BELONGING TO SCROLL         ***
    // *** SO, ALL/OBSCURED SCROLLING DATA ARE NEEDED TO BE REDRAWN ***

    if (!scroll) { // Restore backwards data
     //mainPainter.eraseRect(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
     mainPainter.setPen(Qt::red);
     mainPainter.setBrush(Qt::white);
     ////mainPainter.setBackground(QBrush(Qt::white));
     //mainPainter.fillRect(contentsRect(),QBrush(Qt::white));
     //mainPainter.drawRect(cr);
    }

    // *** ORDINARY (ROLLING) SCROLL, NOTHING MORE ***
    if (scroll) {


     for (c=0;c<colCount;c++) {
      if (wX[c]<=wn[c]) {
       mainPainter.setPen(Qt::white); //white
       mainPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2);
       if (wX[c]<(wn[c]-1)) {
        mainPainter.setPen(Qt::black);
        mainPainter.drawLine(wX[c]+1,1,wX[c]+1,acqM->acqFrameH-2);
       }
      }
     }

     if (tick) {
      mainPainter.setPen(Qt::darkGray);
      mainPainter.drawRect(0,0,acqM->acqFrameW-1,acqM->acqFrameH-1);
      for (c=0;c<colCount;c++) {
       mainPainter.drawLine(w0[c]-1,1,w0[c]-1,acqM->acqFrameH-2);
       mainPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2);
      }
     }

     for (int i=0;i<chnCount;i++) {
      c=chnCount/chnPerCol; curCol=i/chnPerCol;
      chHeight=100.0; // 100 pixel is the indicated amp..
      chY=(float)(h)/(float)(chnPerCol);
      scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//      scrPrvDataX=acqM->scrPrvData[(acqM->acqChannels)[acqM->cntVisChns[i]]->physChn];
//      scrCurDataX=acqM->scrCurData[(acqM->acqChannels)[acqM->cntVisChns[i]]->physChn];
      mainPainter.setPen(Qt::darkGray);
      mainPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);
      mainPainter.setPen(Qt::black);
      mainPainter.drawLine(wX[curCol]-1, // Div400 bcs. range is 400uVpp..
//       scrCurY-(int)(scrPrvDataX*chHeight*acqM->cntAmpX[ampNo]/400.0),
                           wX[curCol],
//       scrCurY-(int)(scrCurDataX*chHeight*acqM->cntAmpX[ampNo]/400.0));

//      if (i==0) qDebug("%2.2f",scrCurDataX);
     }

     if (event) { event=false;
      rBuffer=new QPixmap(100,12); rBuffer->fill(Qt::white);
      rotPainter.begin(rBuffer);
       rotPainter.setFont(evtFont);
       rotPainter.setPen(Qt::darkGreen); // Erroneous event!
       if (acqM->curEventType==1) rotPainter.setPen(Qt::blue);
       else if (acqM->curEventType==2) rotPainter.setPen(Qt::red);
       rotPainter.drawText(2,9,acqM->curEventName);
      rotPainter.end();
      *rBuffer=rBuffer->transformed(rMatrix,Qt::SmoothTransformation);

      for (c=0;c<colCount;c++) {
       mainPainter.drawLine(wX[c],1,wX[c],acqM->acqFrameH-2);
       if (wX[c]<(w0[c]+15)) mainPainter.drawPixmap(wn[c]-15,acqM->acqFrameH-104,*rBuffer);
       else mainPainter.drawPixmap(wX[c]-14,acqM->acqFrameH-104,*rBuffer);
      } delete rBuffer;
     } 

//     if (chnCount>32 && wX[0]==70) { // Overhead and the Chn names together..
//      mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
//      for (int i=0;i<chnCount;i++) {
//       c=chnCount/chnPerCol; curCol=i/chnPerCol;
//       chHeight=100.0; // 100 pixel is the indicated amp..
//       chY=(float)(h)/(float)(chnPerCol);
//       scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
//       mainPainter.drawText(w0[curCol]+4,scrCurY,
//        dummyString.setNum((acqM->acqChannels)[acqM->cntVisChns[i]]->physChn+1)+" "+
//                           (acqM->acqChannels)[acqM->cntVisChns[i]]->name);
//      }
//     } else if (chnCount<=32 && wX[0]<70) { // We have CPU time..
      mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
      for (int i=0;i<chnCount;i++) {
       c=chnCount/chnPerCol; curCol=i/chnPerCol;
       chHeight=100.0; // 100 pixel is the indicated amp..
       chY=(float)(h)/(float)(chnPerCol);
       scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
       mainPainter.drawText(w0[curCol]+4,scrCurY,
        dummyString.setNum((acqM->acqChannels)[acqM->cntVisChns[i]]->physChn+1)+" "+
                           (acqM->acqChannels)[acqM->cntVisChns[i]]->name);
      }
//     }

     if (wX[0]==150) mainPainter.drawLine(149,300,149,400); // Amplitude legend

//     if (tick) { tick=false;
//      for (int i=0;i<chnCount;i++) { // 50Hz Levels
//       c=chnCount/chnPerCol; curCol=i/chnPerCol;
//       chHeight=100.0; // 100 pixel is the indicated amp..
//       chY=(float)(h)/(float)(chnPerCol);
//       scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//      }
//     }

     // Main position increments of columns
     for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

    }
    // *** SCROLL RELATED POST-PROCESS ***
    if (!scroll) { mainPainter.drawRect(contentsRect()); }

    // *** ALL-TIMES POST-PROCESS (SLOWS DOWN DRASTICALLY) ***
*/
   scroll=false;
 
   mainPainter.end();
  }

 protected:
  //void resizeEvent(QResizeEvent *event) {
   //resizeEvent(event); // Call base class event handler
   //setGeometry(x(),y(),parent->width()-9,parent->width()-60);

   //int width = event->size().width();
   //int height = event->size().height();

   // Resize child widgets proportionally
   //label1->setGeometry(width / 4, height / 4, width / 2, 30);
   //label2->setGeometry(width / 4, height / 2, width / 2, 30);
  //}

 private:
  QWidget *parent; AcqMaster *acqM; QString dummyString; QBrush bgBrush; QPainter mainPainter,rotPainter; QVector<int> w0,wn,wX;
  QFont evtFont,chnFont; QPixmap *rBuffer; QTransform rTransform; float chHeight,chY,scrPrvDataX,scrCurDataX,scrPrvDataFX,scrCurDataFX; int scrCurY;
  bool scroll,tick,event; int c,chnCount,colCount,chnPerCol; QPixmap scrollBuffer; unsigned int ampNo;
};

#endif
