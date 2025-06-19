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

class CMLevelFrame : public QFrame {
 Q_OBJECT
 public:
  CMLevelFrame(QWidget *p,AcqDaemon *acqd,unsigned int a) : QFrame(p) {
   parent=p; acqD=acqd; ampNo=a;
   cmBuffer=new QPixmap(acqD->cmLevelFrameW,acqD->cmLevelFrameH);
   chnInfo=&acqD->chnInfo;
   hdrFont1=QFont("Helvetica",28,QFont::Bold);
   chnFont1=QFont("Helvetica",11,QFont::Bold);
   chnFont2=QFont("Helvetica",16,QFont::Bold);

   // Green <-> Yellow <-> Red
   for (unsigned int i=0;i<128;i++) palette.append(QColor(2*i,255,0));
   for (unsigned int i=0;i<128;i++) palette.append(QColor(255,255-2*i,0));

   acqD->registerCMLevelHandler(this);
  }

  QBrush cmBrush(int elec) {
   //return palette[(4*elec)%256];
   unsigned char col;
   float lev=(*chnInfo)[elec].cmLevel[ampNo];
   if (lev>255.0) col=255; else col=(unsigned char)(lev);
   return palette[col];
  }

  void updateBuffer() {
   QPainter cmPainter; unsigned int chIdx,topoX,topoY,a,y,sz; QString chName;
   QRect cr(0,0,acqD->cmLevelFrameW,acqD->cmLevelFrameH);
   QPen pen1(Qt::black,4);
   QPen pen2(Qt::black,2);

   a=(acqD->conf).cmCellSize; sz=5*a/6;

   //cmBuffer->fill(Qt::white);

   cmPainter.begin(cmBuffer);

   cmPainter.setBackground(QBrush(Qt::white));
   cmPainter.setPen(Qt::black);
   // cmPainter.setBrush(Qt::black);
   cmPainter.fillRect(cr,QBrush(Qt::white));
   cmPainter.drawRect(cr);

   QString hdrString="EEG ";
   hdrString.append(QString::number(ampNo+1));
   cmPainter.setFont(hdrFont1);
   cmPainter.drawText(QRect(0,0,acqD->cmLevelFrameW,a),Qt::AlignVCenter,hdrString);

   for (int i=0;i<(*chnInfo).size();i++) {
    chIdx=(*chnInfo)[i].physChn; chName=(*chnInfo)[i].chnName;
    topoX=(*chnInfo)[i].topoX; topoY=(*chnInfo)[i].topoY;
    cmPainter.setPen(pen1);
    if (chIdx<=32) cmPainter.setBrush(cmBrush(i));
    else if (chIdx<=64) cmPainter.setBrush(cmBrush(i));
    else cmPainter.setBrush(cmBrush(i));
    y=0;
    if (topoY>1) y+=a/2;
    if (topoY>10) y+=a/2;
    QRect cr(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2,sz,sz);
    QRect tr1(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2-sz/4,sz,sz);
    QRect tr2(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2+sz/6,sz,sz);
    //cmPainter.fillRect(cr,QBrush(Qt::gray));
    //cmPainter.drawRect(cr);
    cmPainter.drawEllipse(cr);
    cmPainter.setFont(chnFont1);
    cmPainter.drawText(tr1,Qt::AlignHCenter|Qt::AlignVCenter,QString::number(chIdx));
    cmPainter.setFont(chnFont2);
    cmPainter.drawText(tr2,Qt::AlignHCenter|Qt::AlignVCenter,chName);
   }
   cmPainter.setPen(pen2);
   cmPainter.drawLine(a/2,a,width()-a/2,a);
   cmPainter.drawLine(a/2,height()-a,width()-a/2,height()-a);

   cmPainter.end();
  }

 public slots:
  void slotCMLevelsReady() { repaint();
   std::vector<float> cm; // float cMin,cMax;
   for (int i=0;i<chnInfo->size();i++) cm.push_back((*chnInfo)[i].cmLevel[ampNo]);
   //cMin=*std::min_element(cm.begin(),cm.end()); cMax=*std::max_element(cm.begin(),cm.end());
   //qDebug() << "Amp" << ampNo << "Chn (min,max):" << cMin << "," << cMax;
  }

 protected:
  virtual void paintEvent(QPaintEvent*) {
   updateBuffer();
   mainPainter.begin(this);
    mainPainter.setBackgroundMode(Qt::TransparentMode);
    mainPainter.drawPixmap(0,0,*cmBuffer);
   mainPainter.end();
  }

 private:
  QWidget *parent; AcqDaemon *acqD; QString dummyString;
  QBrush bgBrush; QPainter mainPainter; QVector<int> w0,wn,wX;
  QFont hdrFont1,chnFont1,chnFont2; //QPixmap *rBuffer;
  unsigned int ampNo; QPixmap *cmBuffer;
  QVector<ChnInfo> *chnInfo; QVector<QColor> palette;
};

#endif

//for (int i=0;i<palette.size();i++) qDebug() << palette[i];
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

//QRect hr(a+width()/2,height()/2,9*width()/10,9*width()/10);
//cmPainter.setPen(Qt::black);
//cmPainter.setBrush(Qt::white);
//cmPainter.drawEllipse(hr);
   
//for (int i=0;i<chnCount;i++) {
// c=chnCount/chnPerCol; curCol=i/chnPerCol;
// chHeight=100.0; // 100 pixel is the indicated amp..
// chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
// scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//
// scrPrvDataX=acqD->scrPrvData[ampNo][i]; scrCurDataX=acqD->scrCurData[ampNo][i];
// scrPrvDataFX=acqD->scrPrvDataF[ampNo][i]; scrCurDataFX=acqD->scrCurDataF[ampNo][i];
//
// cmPainter.setPen(Qt::darkGray);
// //cmPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);
//
// if (acqD->notch) {
//  cmPainter.setPen(Qt::black); // Filtered in Red
//  cmPainter.drawLine( // Div400 bcs. range is 400uVpp..
//   wX[curCol]-1,scrCurY-(int)(scrPrvDataFX*chHeight*acqD->cntAmpX[ampNo]/4.0),
//   wX[curCol],scrCurY-(int)(scrCurDataFX*chHeight*acqD->cntAmpX[ampNo]/4.0));
// } else {
//  cmPainter.setPen(Qt::black);
//  cmPainter.drawLine( // Div400 bcs. range is 400uVpp..
//   wX[curCol]-1,scrCurY-(int)(scrPrvDataX*chHeight*acqD->cntAmpX[ampNo]/4.0),
//   wX[curCol],scrCurY-(int)(scrCurDataX*chHeight*acqD->cntAmpX[ampNo]/4.0));
// }
// if (i==0) qDebug("%2.2f",scrCurDataX);
//}

//if (event) { event=false;
// rBuffer=new QPixmap(100,12); rBuffer->fill(Qt::white);
// rotPainter.begin(rBuffer);
//  rotPainter.setFont(evtFont);
//  rotPainter.setPen(Qt::darkGreen); // Erroneous event!
//  if (acqD->curEventType==1) rotPainter.setPen(Qt::blue);
//  else if (acqD->curEventType==2) rotPainter.setPen(Qt::red);
//  rotPainter.drawText(2,9,acqD->curEventName);
// rotPainter.end();
// *rBuffer=rBuffer->transformed(rMatrix,Qt::SmoothTransformation);

// for (c=0;c<colCount;c++) {
//  cmPainter.drawLine(wX[c],1,wX[c],acqD->acqFrameH-2);
//  if (wX[c]<(w0[c]+15)) cmPainter.drawPixmap(wn[c]-15,acqD->acqFrameH-104,*rBuffer);
//  else cmPainter.drawPixmap(wX[c]-14,acqD->acqFrameH-104,*rBuffer);
// } delete rBuffer;
//} 

// cmPainter.setPen(QColor(50,50,150)); cmPainter.setFont(chnFont);
// for (int i=0;i<chnCount;i++) {
//  c=chnCount/chnPerCol; curCol=i/chnPerCol;
//  chHeight=100.0; // 100 pixel is the indicated amp..
//  chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
//  scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
//  cmPainter.drawText(w0[curCol]+4,scrCurY,
//   dummyString.setNum((acqD->acqChannels)[ampNo][acqD->cntVisChns[ampNo][i]]->physChn+1)+" "+
//                      (acqD->acqChannels)[ampNo][acqD->cntVisChns[ampNo][i]]->name);
// }

// if (wX[0]==150) cmPainter.drawLine(149,300,149,400); // Amplitude legend

// if (tick) { tick=false;
//  for (int i=0;i<chnCount;i++) { // 50Hz Levels
//   c=chnCount/chnPerCol; curCol=i/chnPerCol;
//   chHeight=100.0; // 100 pixel is the indicated amp..
//   chY=(float)(acqD->acqFrameH)/(float)(chnPerCol);
//   scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
//  }
// }

// Main position increments of columns
// for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

// protected:
//qDebug() << "CM Level update" << ampNo;
//updateBuffer();

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
