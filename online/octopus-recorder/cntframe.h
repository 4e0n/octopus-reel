
#ifndef CNTFRAME_H
#define CNTFRAME_H

#include <QFrame>
#include <QPainter>
#include <QBitmap>
#include <QMatrix>

#include "octopus_rec_master.h"
#include "octopus_channel.h"

class CntFrame : public QFrame {
 Q_OBJECT
 public:
  CntFrame(QWidget *pnt,RecMaster *rm) : QFrame(pnt) {
   parent=pnt; p=rm; w=parent->width()-9; h=parent->height()-60;
   setGeometry(2,2,w,h); scroll=false;

   setAttribute(Qt::WA_NoSystemBackground,true);
   extern void qt_x11_set_global_double_buffer(bool);
   qt_x11_set_global_double_buffer(false);

   chnCount=p->cntVisChns.size();
   colCount=ceil((float)chnCount/(float)(32.));
   chnPerCol=ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)w/(float)colCount);
   for (int i=0;i<colCount;i++) { w0.append(i*ww+1); wX.append(i*ww+1); }
   for (int i=0;i<colCount-1;i++) wn.append((i+1)*ww-1); wn.append(w-1);

   if (chnCount<16)
    chnFont=QFont("Helvetica",12,QFont::Bold);
   else if (chnCount>16 && chnCount<32)
    chnFont=QFont("Helvetica",11,QFont::Bold);
   else if (chnCount>96)
    chnFont=QFont("Helvetica",10);
   evtFont=QFont("Helvetica",8,QFont::Bold);
   rMatrix.rotate(-90);

   p->registerScrollHandler(this);
  }

 public slots:
  void slotScrData(bool t,bool e) {
   scroll=true; tick=t; event=e; repaint();
  }
 protected:
  virtual void paintEvent(QPaintEvent*) {
   int curCol;
   mainPainter.begin(this);
    mainPainter.setPen(Qt::black);
    mainPainter.setBackground(QBrush(Qt::white));
    mainPainter.setClipping(false);

    // ***         REPAINT REQUESTS NOT BELONGING TO SCROLL         ***
    // *** SO, ALL/OBSCURED SCROLLING DATA ARE NEEDED TO BE REDRAWN ***

    if (!scroll) {
     mainPainter.eraseRect(0,0,w-1,h-1);
     // restore backwards data..
    }

    // *** ORDINARY (ROLLING) SCROLL, NOTHING MORE ***
    if (scroll) {

     for (c=0;c<colCount;c++) {
      if (wX[c]<=wn[c]) {
       mainPainter.setPen(Qt::white);
       mainPainter.drawLine(wX[c],1,wX[c],h-2);
       if (wX[c]<(wn[c]-1)) {
        mainPainter.setPen(Qt::black);
        mainPainter.drawLine(wX[c]+1,1,wX[c]+1,h-2);
       }
      }
     }

     if (tick) {
      mainPainter.setPen(Qt::darkGray);
      mainPainter.drawRect(0,0,w-1,h-1);
      for (c=0;c<colCount;c++) {
       mainPainter.drawLine(w0[c]-1,1,w0[c]-1,h-2);
       mainPainter.drawLine(wX[c],1,wX[c],h-2);
      }
     }

     for (int i=0;i<chnCount;i++) {
      c=chnCount/chnPerCol; curCol=i/chnPerCol;
      chHeight=100.0; // 100 pixel is the indicated amp..
      chY=(float)(h)/(float)(chnPerCol);
      scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));
      scrPrvDataX=p->scrPrvData[(p->acqChannels)[p->cntVisChns[i]]->physChn];
      scrCurDataX=p->scrCurData[(p->acqChannels)[p->cntVisChns[i]]->physChn];
      mainPainter.setPen(Qt::darkGray);
      mainPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);
      mainPainter.setPen(Qt::black);
      mainPainter.drawLine(wX[curCol]-1, // Div400 bcs. range is 400uVpp..
       scrCurY-(int)(scrPrvDataX*chHeight*p->cntAmpX/400.0),
                           wX[curCol],
       scrCurY-(int)(scrCurDataX*chHeight*p->cntAmpX/400.0));
     }

     if (event) { event=false;
      rBuffer=new QPixmap(100,12); rBuffer->fill(Qt::white);
      rotPainter.begin(rBuffer);
       rotPainter.setFont(evtFont);
       rotPainter.setPen(Qt::darkGreen); // Erroneous event!
       if (p->curEventType==1) rotPainter.setPen(Qt::blue);
       else if (p->curEventType==2) rotPainter.setPen(Qt::red);
       rotPainter.drawText(2,9,p->curEventName);
      rotPainter.end();
      *rBuffer=rBuffer->transformed(rMatrix,Qt::SmoothTransformation);

      for (c=0;c<colCount;c++) {
       mainPainter.drawLine(wX[c],1,wX[c],h-2);
       if (wX[c]<(w0[c]+15)) mainPainter.drawPixmap(wn[c]-15,h-104,*rBuffer);
       else mainPainter.drawPixmap(wX[c]-14,h-104,*rBuffer);
      } delete rBuffer;
     } 

     if (chnCount>32 && wX[0]==70) { // Overhead and the Chn names together..
      mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
      for (int i=0;i<chnCount;i++) {
       c=chnCount/chnPerCol; curCol=i/chnPerCol;
       chHeight=100.0; // 100 pixel is the indicated amp..
       chY=(float)(h)/(float)(chnPerCol);
       scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
       mainPainter.drawText(w0[curCol]+4,scrCurY,
        dummyString.setNum((p->acqChannels)[p->cntVisChns[i]]->physChn+1)+" "+
                           (p->acqChannels)[p->cntVisChns[i]]->name);
      }
     } else if (chnCount<=32 && wX[0]<70) { // We have CPU time..
      mainPainter.setPen(QColor(50,50,150)); mainPainter.setFont(chnFont);
      for (int i=0;i<chnCount;i++) {
       c=chnCount/chnPerCol; curCol=i/chnPerCol;
       chHeight=100.0; // 100 pixel is the indicated amp..
       chY=(float)(h)/(float)(chnPerCol);
       scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
       mainPainter.drawText(w0[curCol]+4,scrCurY,
        dummyString.setNum((p->acqChannels)[p->cntVisChns[i]]->physChn+1)+" "+
                           (p->acqChannels)[p->cntVisChns[i]]->name);
      }
     }

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

    scroll=false;
   mainPainter.end();
  }

 private:
  QWidget *parent; RecMaster *p; QString dummyString;
  QBrush bgBrush; QPainter mainPainter,rotPainter; QVector<int> w0,wn,wX;
  QFont evtFont,chnFont; QPixmap *rBuffer; QMatrix rMatrix;
  float chHeight,chY,scrPrvDataX,scrCurDataX; int scrCurY;
  bool scroll,tick,event; int w,h,c,chnCount,colCount,chnPerCol;
};

#endif
