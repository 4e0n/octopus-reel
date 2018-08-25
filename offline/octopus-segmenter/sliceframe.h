/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef SLICEFRAME_H
#define SLICEFRAME_H

#include <QWidget>
#include <QFrame>
#include <QPainter>
#include <QMouseEvent>
#include <QComboBox>
#include "octopus_seg_master.h"
#include "octopus_mri_slice.h"

class SliceFrame : public QFrame {
 Q_OBJECT
 public:
  SliceFrame(QWidget* pnt,SegMaster *sm,int x,int y,QString wmi) : QFrame(pnt) {
   parent=pnt; p=sm; if (wmi=="L") curVolume=&(p->curVolumeL);
                   else if (wmi=="R") curVolume=&(p->curVolumeR);
   setGeometry(x,y,p->fWidth,p->fHeight);
   setMouseTracking(true);	// Receive events also when mouse
                                // buttons not pressed
   penColorErase=QRgb(Qt::black); penColorFill=QRgb(Qt::white);
   penSize=p->brushSize; // Diameter is 21 pixels;
  }

 protected:
  void mousePressEvent(QMouseEvent *event) {
   if (p->mrSetExists) {
    QImage *curImg=&(p->volume[*curVolume].slice[p->curSlice]->data);
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
      if (event->buttons() & Qt::LeftButton) { p->selection(QPoint(x,y)); }
      break;
     case MOUSEMODE_SEED:  break;
    }
   }
  }
  void mouseMoveEvent(QMouseEvent *event) {
   if (p->mrSetExists) {
    QImage *curImg=&(p->volume[*curVolume].slice[p->curSlice]->data);
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
   MRISlice *currentSlice;
   QImage img; QPixmap buf,buf2,buf3;
   QBrush bgBrush(Qt::black);
   QBrush ulBrush(Qt::gray,Qt::DiagCrossPattern);

   mainPainter.begin(this);
    mainPainter.setBackground(bgBrush);
    mainPainter.setBackgroundMode(Qt::TransparentMode);

    if (p->mrSetExists) {
     if (p->previewMode) currentSlice=&(p->previewSlice);
     else currentSlice=p->volume[*curVolume].slice[p->curSlice];
     img=currentSlice->data.copy();
     buf2=buf.fromImage(img);
     int w=buf2.width(); int h=buf2.height();

     bufferPainter.begin(&buf2);
      // Slice histogram and scalp coords if available
      bufferPainter.setPen(Qt::yellow);
      for (int i=0;i<255;i++)
       bufferPainter.drawLine((int)((float)(i*w)/(float)(256.)),
                              h-currentSlice->histogram[i]*0.9*h,
                              (int)((float)((i+1)*w)/(float)(256.)),
                              h-currentSlice->histogram[i+1]*0.9*h);
      bufferPainter.setPen(Qt::red);
      if (p->scalpCoord2D.size()) {
       QVector<QPoint> *coords=&(p->scalpCoord2D[p->curSlice]);
       int cCount=coords->size();
       if (cCount) {
        for (int i=0;i<cCount;i++)
         bufferPainter.drawLine((*coords)[i].x(),
                                (*coords)[i].y(),
                                (*coords)[(i+1)%cCount].x(),
                                (*coords)[(i+1)%cCount].y());
                                // Reweave to the beginning..
       }
      }

      bufferPainter.setPen(Qt::cyan);
      bufferPainter.drawLine(0,currentSlice->center.y(),
                             w-1,currentSlice->center.y());
      bufferPainter.drawLine(currentSlice->center.x(),0,
                             currentSlice->center.x(),h-1);
      bufferPainter.setPen(Qt::magenta);
      bufferPainter.drawLine(0,currentSlice->yMin,
                             w-1,currentSlice->yMin);
      bufferPainter.drawLine(0,currentSlice->yMax,
                             w-1,currentSlice->yMax);
      bufferPainter.drawLine(currentSlice->xMin,0,
                             currentSlice->xMin,h-1);
      bufferPainter.drawLine(currentSlice->xMax,0,
                             currentSlice->xMax,h-1);

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
  QWidget *parent; SegMaster *p;
  QPainter mainPainter,bufferPainter;
  QString dummyString; int *curVolume;
  int penSize; QRgb penColorErase,penColorFill;
};

#endif
//   // Construct moving average kernel of diameter 2*x+1
//   int count=0; int n=2*fltValue+1; QPixmap kPix(n,n); kPix.fill(Qt::black);
//   kp.begin(&kPix); kp.setPen(Qt::white); kp.setBrush(Qt::white);
//    kp.drawEllipse(0,0,kPix.width()-1,kPix.height()-1);
//   kp.end(); ki=kPix.toImage().convertToFormat(QImage::Format_Indexed8,
//                                               Qt::ColorOnly);
