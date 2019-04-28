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

#ifndef OCTOPUS_MRIVOLUME_H
#define OCTOPUS_MRIVOLUME_H

#include <QVector>
#include "mrislice.h"

const int STK_DEPTH=100000;

class MRIVolume {
 public:
  MRIVolume() { histogram.resize(256); }

  ~MRIVolume() {
   for (int i=0;i<slice.size();i++) delete slice[i]; slice.resize(0);
  }

  void append(MRISlice *s) { slice.append(s); }

  void update() { int gr; float max;
   int w=slice[0]->data.width(); int h=slice[0]->data.height();
   for (int i=0;i<256;i++) histogram[i]=0.;
   for (int k=0;k<slice.size();k++)
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
     gr=qGray(slice[k]->data.pixel(i,j));
     histogram[gr]+=(float)(gr)/256.;
    }
   max=0.; for (int i=0;i<256;i++) if (histogram[i]>max) max=histogram[i];
   if (max>0.) for (int i=0;i<256;i++) histogram[i]/=max;
  }

  void normalize() { // Slice-based normalization to background level..
   for (int i=0;i<slice.size();i++) slice[i]->normalize(); update();
  }
   
  void threshold(int val,bool bin,bool fill,bool inv) {
   for (int i=0;i<slice.size();i++)
    slice[i]->threshold(val,bin,fill,inv); update();
  }

  void filter2D(QVector<float> *k) {// Slice-base simple 2D conv. filter
   for (int i=0;i<slice.size();i++) slice[i]->filter(k); update();
  }

  void edge2D(QVector<float> *k) {
   for (int i=0;i<slice.size();i++) slice[i]->edge(k); update();
  }

  void kMeans2D(int c) {
   for (int i=0;i<slice.size();i++) slice[i]->kMeans(c); update();
  }

  QVector<MRISlice*> slice; QVector<float> histogram; int xSize,ySize,zSize;
};

#endif
