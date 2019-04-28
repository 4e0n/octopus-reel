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

#ifndef OCTOPUS_MRISLICE_H
#define OCTOPUS_MRISLICE_H

#include <QVector>
#include <QImage>
#include <QPoint>

class MRISlice { // Can QImage be extended instead, rather than a nested var?
 public:
  MRISlice() {}
  MRISlice(QImage img) { data=img.copy(); }

  void load(QString fn) { data.load(fn); }

  void clear() { int w=data.width(); int h=data.height();
   for (int j=0;j<h;j++) for (int i=0;i<w;i++) data.setPixel(i,j,0);
  }

  void normalize() { int avg=0,count=0,pix,w,h,sw,sh;
   w=data.width(); h=data.height(); sw=w/10; sh=h/10;
   for (int j=0;j<sh;j++) for (int i=0;i<sw;i++) { // 1/10 segment in corners
    avg+=qGray(data.pixel(i,j));
    avg+=qGray(data.pixel(w-1-i,j));
    avg+=qGray(data.pixel(i,h-1-j));
    avg+=qGray(data.pixel(w-1-i,h-1-j)); count+=4;
   } avg=(int)((float)(avg)/(float)(count));
   if (avg) { // Prevent div-by-zero..
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
     pix=qGray(data.pixel(i,j)); pix=(int)((float)(pix)-(float)(avg));
     if (pix<0) pix=0; data.setPixel(i,j,pix);
    }
   } 
  }

  void threshold(int thr,bool bin,bool fill,bool inv) {
   quint8 pix; int x,y,fx,lx,w,h; w=data.width(); h=data.height();
   if (fill) {
    for (y=0;y<h;y++) {
     x=0; while ((x<w) && qGray(data.pixel(x,y))<thr) {
      data.setPixel(x,y,0); x++; } fx=x+1;
     x=w-1; while ((x>=0) && qGray(data.pixel(x,y))<thr) {
      data.setPixel(x,y,0); x--; } lx=x-1;
     for (x=fx;x<lx;x++) { data.setPixel(x,y,255); }
    }
   } else {
    if (inv) {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix<=thr) data.setPixel(x,y,255); else data.setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix<=thr) data.setPixel(x,y,pix); else data.setPixel(x,y,0);
      }
     }
    } else {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix>=thr) data.setPixel(x,y,255); else data.setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix>=thr) data.setPixel(x,y,pix); else data.setPixel(x,y,0);
      }
     }
    }
   }
  }

  void threshold(MRISlice* dst,int thr,bool bin,bool fill,bool inv) {
   quint8 pix; int x,y,fx,lx,w,h; w=data.width(); h=data.height();
   if (fill) {
    for (y=0;y<h;y++) {
     x=0; while ((x<w) && qGray(data.pixel(x,y))<thr) {
      dst->data.setPixel(x,y,0); x++; } fx=x+1;
     x=w-1; while ((x>=0) && qGray(data.pixel(x,y))<thr) {
      dst->data.setPixel(x,y,0); x--; } lx=x-1;
     for (x=fx;x<lx;x++) { dst->data.setPixel(x,y,255); }
    }
   } else {
    if (inv) {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix<=thr) dst->data.setPixel(x,y,255);
       else dst->data.setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix<=thr) dst->data.setPixel(x,y,pix);
       else dst->data.setPixel(x,y,0);
      }
     }
    } else {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix>=thr) dst->data.setPixel(x,y,255);
       else dst->data.setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(data.pixel(x,y));
       if (pix>=thr) dst->data.setPixel(x,y,pix);
       else dst->data.setPixel(x,y,0);
      }
     }
    }
   }
  }

  void filter(QVector<float> *k) { // Filter ourself
   int x,y,i,j,w,h,X,Y,t,index; int n=sqrt(k->size())/2;
   QImage dummyImage=data.copy(); w=dummyImage.width(); h=dummyImage.height();
   for (y=0;y<h;y++) for (x=0;x<w;x++) { t=index=0;
    for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) { Y=y+j; X=x+i;
     if (X>=0 && Y>=0 && X<w && Y<h)
      t+=(int)((float)(qGray(dummyImage.pixel(X,Y)))*(*k)[index]); index++;
    } data.setPixel(x,y,t);
   }
  }

  void filter(MRISlice *dst,QVector<float> *k) {
   int x,y,i,j,w,h,X,Y,t,index; int n=sqrt(k->size())/2;
   QImage *s=&data; QImage *d=&(dst->data);
   w=s->width(); h=s->height();
   for (y=0;y<h;y++) for (x=0;x<w;x++) { t=index=0;
    for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) { Y=y+j; X=x+i;
     if (X>=0 && Y>=0 && X<w && Y<h)
      t+=(int)((float)(qGray(s->pixel(X,Y)))*(*k)[index]); index++;
    } d->setPixel(x,y,t);
   }
  }

  void edge(QVector<float> *e) { // Detect Edges using kernel k
   int x,y,i,j,w,h,X,Y,t,index; int n=sqrt(e->size())/2;
   QImage dummyImage=data.copy(); w=dummyImage.width(); h=dummyImage.height();
   for (y=0;y<h;y++) for (x=0;x<w;x++) { t=index=0;
    for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) { Y=y+j; X=x+i;
     if (X>=0 && Y>=0 && X<w && Y<h)
      t+=(int)((float)(qGray(dummyImage.pixel(X,Y)))*(*e)[index]); index++;
    } if (t<=0 && t>255) qDebug("%d",t); data.setPixel(x,y,(quint8)(t));
   }
  }

  void kMeans(int c) { // kMeans Seg with homogeneous initial centroids..
   QVector<quint8> map,prvCent,curCent,deltas; int maxd,current,w,h;
   QVector<int> sums,counts; quint8 offset=0,pix; bool iteration=true;
   QImage dummyImage=data.copy(); w=dummyImage.width(); h=dummyImage.height();
   map.resize(w*h);

   // Construct centroids homogeneously
   prvCent.resize(c); curCent.resize(c); sums.resize(c); counts.resize(c);
   quint8 delta=(int)((float)(256.)/(float)(curCent.size()+1));
   for (int i=0;i<curCent.size();i++) curCent[i]=delta*(quint8)(i+1); 

   // Main iteration
   while (iteration) {
    // Assign current pixels to nearest centroid..
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
     pix=qGray(dummyImage.pixel(i,j)); deltas.resize(0);
     for (int m=0;m<curCent.size();m++) deltas.append(abs(pix-curCent[m]));
     maxd=1000;
     for (int m=0;m<curCent.size();m++) { // Take the smallest one..
      if (deltas[m]<maxd) { maxd=deltas[m]; offset=m; }
     } map[j*w+i]=offset;
    }

    for (int i=0;i<prvCent.size();i++) prvCent[i]=curCent[i]; // Copy to prev..

    // Compute updated ones..
    for (int i=0;i<curCent.size();i++) sums[i]=counts[i]=0;
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
     current=map[j*w+i]; pix=qGray(dummyImage.pixel(i,j));
     sums[current]+=pix; counts[current]++;
    }
    for (int i=0;i<curCent.size();i++)
     if (counts[i]>0) curCent[i]=sums[i]/counts[i];

    iteration=false;
    for (int i=0;i<curCent.size();i++)
     if (curCent[i]!=prvCent[i]) { iteration=true; break; }
   }

   for (int j=0;j<h;j++) for (int i=0;i<w;i++) { // Set final slice
    pix=curCent[map[j*w+i]]; data.setPixel(i,j,pix); // Maximized centroid
   }
  }

  QImage data;
};

#endif
