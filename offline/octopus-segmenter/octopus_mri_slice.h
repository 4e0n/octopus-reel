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
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef OCTOPUS_MRI_SLICE_H
#define OCTOPUS_MRI_SLICE_H

#include <QVector>
#include <QImage>
#include <QPoint>

class MRISlice { // TODO: Can it be flawlessly extended from QImage? Search it..
 public:
  MRISlice() {} // Bound threshold and grace..
  MRISlice(QImage img) { data=img.copy(); update(); }

  void load(QString fn) { data.load(fn); update(); }

  void clr() { int w=data.width(); int h=data.height();
   for (int j=0;j<h;j++) for (int i=0;i<w;i++) data.setPixel(i,j,0);
  }

  void update() {
   quint8 pix; // Histogram and Bounding Box
   int i,j,w,h,xAvg=0,yAvg=0,histMax=0,count=0; QVector<int> dummy(256);

   w=data.width(); h=data.height(); xMin=yMin=100000; xMax=yMax=0;

   for (int j=0;j<5;j++) for (int i=0;i<w;i++) {
    data.setPixel(i,j,0); data.setPixel(i,h-j-1,0);
   }
   for (int j=0;j<h;j++) for (int i=0;i<5;i++) {
    data.setPixel(i,j,0); data.setPixel(w-i-1,j,0);
   }

   for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
    pix=qGray(data.pixel(i,j)); 
    if (pix) { if (i<xMin) xMin=i; if (i>xMax) xMax=i;
               if (j<yMin) yMin=j; if (j>yMax) yMax=j; }
   } xMin-=2; xMax+=2; yMin-=2; yMax+=2;

   // Compute/normalize histogram and center-of-gravity
   histogram.resize(256); for (int i=0;i<256;i++) histogram[i]=0.;
   for (j=yMin;j<=yMax;j++) for (i=xMin;i<=xMax;i++) {
    pix=qGray(data.pixel(i,j));
    if (pix) { xAvg+=i; yAvg+=j; count++; } dummy[pix]++;
   } center.setX((int)((float)(xAvg)/(float)(count)));
     center.setY((int)((float)(yAvg)/(float)(count)));

   for (int i=0;i<256;i++) if (dummy[i]>histMax) histMax=dummy[i];
   for (int i=0;i<256;i++) histogram[i]=(float)(dummy[i])/(float)(histMax);
  }

  void calcScalpCoords(float r,int count) {
   float theta; float dr=(2.*M_PI)/(float)(count); scalpCoord.resize(0);
   int x1,y1,x2,y2; QPoint p;
   x1=center.x(); y1=center.y();
   theta=0.;
   for (int i=0;i<count;i++) {
    x2=x1+(int)(r*cos(theta)-r*sin(theta));
    y2=y1-(int)(r*sin(theta)+r*cos(theta));
    p=bdc(x1,y1,x2,y2); scalpCoord.append(p);
    theta+=dr;
   }
  }

  void calcSkullCoords(float r,int count) {
   float theta; float dr=(2.*M_PI)/(float)(count); skullCoord.resize(0);
   int x1,y1,x2,y2; QPoint p;
   x1=center.x(); y1=center.y();
   for (theta=0.0;theta<(2.0*M_PI);theta+=dr) {
    x2=x1+(int)(r*cos(theta)-r*sin(theta));
    y2=y1-(int)(r*sin(theta)+r*cos(theta));
    p=bdc(x1,y1,x2,y2); skullCoord.append(p);
   }
  }

  void calcBrainCoords(float r,int count) {
   float theta; float dr=(2.*M_PI)/(float)(count); brainCoord.resize(0);
   int x1,y1,x2,y2; QPoint p;
   x1=center.x(); y1=center.y();
   for (theta=0.0;theta<(2.0*M_PI);theta+=dr) {
    x2=x1+(int)(r*cos(theta)-r*sin(theta));
    y2=y1-(int)(r*sin(theta)+r*cos(theta));
    p=bdc(x1,y1,x2,y2); brainCoord.append(p);
   }
  }

  QVector<QPoint> scalpCoord,skullCoord,brainCoord; QPoint center;
  QImage data; QVector<float> histogram;
  int xMin,xMax,yMin,yMax;

 private:
  // Bresenham detect lasy non-zero point
  QPoint bdc(int x1,int y1,int x2,int y2) {
   int x=x1,y=y1,d=0,dx=x2-x1,dy=y2-y1,c,m,xInc=1,yInc=1,cx=x1,cy=y1;
   int w=data.width(); int h=data.height();
   if (dx<0) { xInc=-1; dx=-dx; } if (dy<0) { yInc=-1; dy=-dy; }
   if (dy<dx) {
    c=2*dx; m=2*dy;
    while (x!=x2) {
     if (x>=0 && x<w && y>=0 && y<h &&
         qGray(data.pixel(x,y))) { cx=x; cy=y; }
     x+=xInc; d+=m; if (d>dx) { y+=yInc; d-=c; }
    }
   } else {
    c=2*dy; m=2*dx;
    while (y!=y2) {
     if (x>=0 && x<w && y>=0 && y<h && qGray(data.pixel(x,y))) { cx=x; cy=y; }
     y+=yInc; d+=m; if (d>dy) { x+=xInc; d-=c; }
    }
   }
   return QPoint(cx,cy);
  }

  int bt;
};

#endif
