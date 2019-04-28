/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

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

#ifndef SLICEFRAME_H
#define SLICEFRAME_H

#include <QWidget>
#include <QFrame>
#include <QPainter>
#include <QMouseEvent>
#include <QComboBox>
#include "octopus_bem_master.h"
#include "../../../common/mrislice.h"

class SliceFrame : public QFrame {
 Q_OBJECT
 public:
  SliceFrame(QWidget* pnt,BEMMaster *sm,int x,int y,QString wmi) : QFrame(pnt) {
   parent=pnt; p=sm; if (wmi=="L") curBuffer=&(p->curBufferL);
                   else if (wmi=="R") curBuffer=&(p->curBufferR);
   setGeometry(x,y,p->fWidth,p->fHeight);
   setMouseTracking(true);	// Receive events also when mouse
                                // buttons not pressed
   penColorErase=QRgb(Qt::black); penColorFill=QRgb(Qt::white);
   penSize=p->brushSize; // Diameter is 21 pixels;
  }

 protected:
  void mousePressEvent(QMouseEvent *event) { MRIVolume *vBuf;
   if (p->mrSetExists) {
    vBuf=p->findVol(*curBuffer);
    QImage *curImg=&(vBuf->slice[p->curSlice]->data);
    int x=(float)(event->x())*(float)(p->xSize)/(float)(width());
    int y=(float)(event->y())*(float)(p->ySize)/(float)(height());
    switch (p->mouseMode) {
     default:
     case MOUSEMODE_DRAW: penSize=p->brushSize;
      if (event->buttons() & Qt::LeftButton) {
       for (int j=y-penSize/2;j<=y+penSize/2;j++)
        for (int i=x-penSize/2;i<=x+penSize/2;i++) {
         if (i>=0 && i<curImg->width() && j>=0 && j<curImg->height())
          curImg->setPixel(i,j,255);
        } repaint();
      } else if (event->buttons() & Qt::RightButton) {
       for (int j=y-penSize/2;j<=y+penSize/2;j++)
        for (int i=x-penSize/2;i<=x+penSize/2;i++) {
         if (i>=0 && i<curImg->width() && j>=0 && j<curImg->height())
          curImg->setPixel(i,j,0);
        } repaint();
      } break;
     case MOUSEMODE_POINT:
      if (event->buttons() & Qt::LeftButton) { p->coRegSelect(QPoint(x,y)); }
      break;
     case MOUSEMODE_SEED:  break;
    }
   }
  }
  void mouseMoveEvent(QMouseEvent *event) { MRIVolume *vBuf;
   if (p->mrSetExists) {
    vBuf=p->findVol(*curBuffer);
    QImage *curImg=&(vBuf->slice[p->curSlice]->data);
    int x=(float)(event->x())*(float)(p->xSize)/(float)(width());
    int y=(float)(event->y())*(float)(p->ySize)/(float)(height());
    switch (p->mouseMode) {
     default:
     case MOUSEMODE_DRAW: penSize=p->brushSize;
      if (event->buttons() & Qt::LeftButton) {
       for (int j=y-penSize/2;j<=y+penSize/2;j++)
        for (int i=x-penSize/2;i<=x+penSize/2;i++) {
         if (i>=0 && i<curImg->width() && j>=0 && j<curImg->height())
          curImg->setPixel(i,j,255);
        } repaint();
      } else if (event->buttons() & Qt::RightButton) {
       for (int j=y-penSize/2;j<=y+penSize/2;j++)
        for (int i=x-penSize/2;i<=x+penSize/2;i++) {
         if (i>=0 && i<curImg->width() && j>=0 && j<curImg->height())
          curImg->setPixel(i,j,0);
        } repaint();
      } break;
     case MOUSEMODE_POINT: break;
     case MOUSEMODE_SEED:  break;
    }
   }
  }

  virtual void paintEvent(QPaintEvent*) {
   MRIVolume *vBuf; MRISlice *currentSlice; int i,w,h;
   QImage img; QPixmap buf,buf2,buf3;
   QBrush bgBrush(Qt::black);
   QBrush ulBrush(Qt::gray,Qt::DiagCrossPattern);

   mainPainter.begin(this);
    mainPainter.setBackground(bgBrush);
    mainPainter.setBackgroundMode(Qt::TransparentMode);

    if (p->mrSetExists) {
     vBuf=p->findVol(*curBuffer);
     if (p->previewMode) currentSlice=&(p->previewSlice);
     else currentSlice=vBuf->slice[p->curSlice];

     img=currentSlice->data.copy(); w=img.width(); h=img.height();
     buf2=buf.fromImage(img);
     int w=buf2.width(); int h=buf2.height();

     bufferPainter.begin(&buf2);
      // Slice histogram and scalp coords if available
      bufferPainter.setPen(Qt::yellow);
      for (i=0;i<255;i++)
       bufferPainter.drawLine((int)((float)(i*w)/(float)(256.)),
                              h-vBuf->histogram[i]*0.9*h,
                              (int)((float)((i+1)*w)/(float)(256.)),
                              h-vBuf->histogram[i+1]*0.9*h);
      bufferPainter.setPen(Qt::red);

     bufferPainter.end();

     buf3=buf2.scaled(width(),height(),Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);

     mainPainter.drawPixmap(0,0,buf3);
     mainPainter.setPen(Qt::green);
     mainPainter.drawText(10,20,"Slice #"+dummyString.setNum(p->curSlice));
    } else {
     mainPainter.eraseRect(0,0,width()-1,height()-1);
     mainPainter.setPen(Qt::green);
     mainPainter.drawText(100,height()/2,"Data not loaded.");
    }
    mainPainter.setPen(Qt::darkGray);
    mainPainter.drawRect(0,0,width()-1,height()-1);
   mainPainter.end();
  }

 private:
  QWidget *parent; BEMMaster *p;
  QPainter mainPainter,bufferPainter;
  QString dummyString; int *curBuffer;
  int penSize; QRgb penColorErase,penColorFill;
};

#endif
