/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

#pragma once

#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QRect>
#include <cmath>
#include <vector>
#include <algorithm>
#include "confparam.h"

class CMFrame:public QFrame {
 Q_OBJECT
 public:
  enum CenterMode {
   UseMean=0,
   UseMedian=1
  };

  explicit CMFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr):QFrame(parent) {
   conf=c; ampNo=a;
   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    setGeometry(20+ampNo*(conf->frameW+20),20,conf->frameW,conf->frameH);
   } else {
    const int perRow=conf->guiMaxAmpPerLine;
    const int row=int(ampNo)/perRow;
    const int col=int(ampNo)%perRow;
    setGeometry(20+col*(conf->frameW+20),20+row*(conf->frameH+20),conf->frameW,conf->frameH);
   }
   cmImage=QImage(conf->frameW,conf->frameH,QImage::Format_RGB32);
   cmImage.fill(Qt::white);
  }

  void setCenterMode(CenterMode m) {
   centerMode=m;
   refStatsInit=false;
   bipStatsInit=false;
  }

  void refreshImage() {
   std::vector<float> refVals,bipVals;
   refVals.reserve(conf->refChns.size());
   bipVals.reserve(conf->bipChns.size());

   auto validDataIndex=[&](int dataIdx)->bool {
    if (ampNo>=unsigned(conf->curCMData.size())) return false;
    if (dataIdx<0 || dataIdx>=conf->curCMData[int(ampNo)].size()) return false;
    return true;
   };

   for (int i=0;i<conf->refChns.size();++i) {
    if (validDataIndex(i)) refVals.push_back(conf->curCMData[int(ampNo)][i]);
   }
   for (int i=0;i<conf->bipChns.size();++i) {
    const int dataIdx=int(conf->refChnCount)+i;
    if (validDataIndex(dataIdx)) bipVals.push_back(conf->curCMData[int(ampNo)][dataIdx]);
   }

   if (refVals.empty() && bipVals.empty()) {
    cmImage.fill(Qt::white);
    update();
    return;
   }

   const float alpha=0.15f;
   float refCenterNow=0.0f,refStdNow=1.0f;
   float bipCenterNow=0.0f,bipStdNow=1.0f;

   if (!refVals.empty()) {
    refCenterNow=computeCenter(refVals); refStdNow=computeStd(refVals,refCenterNow);
    if (!refStatsInit) {
     refSmoothCenter=refCenterNow; refSmoothStd=refStdNow; refStatsInit=true;
    } else {
     refSmoothCenter=(1.0f-alpha)*refSmoothCenter+alpha*refCenterNow;
     refSmoothStd=(1.0f-alpha)*refSmoothStd+alpha*refStdNow;
    }
   }

   if (!bipVals.empty()) {
    bipCenterNow=computeCenter(bipVals); bipStdNow=computeStd(bipVals,bipCenterNow);
    if (!bipStatsInit) {
     bipSmoothCenter=bipCenterNow; bipSmoothStd=bipStdNow; bipStatsInit=true;
    } else {
     bipSmoothCenter=(1.0f-alpha)*bipSmoothCenter+alpha*bipCenterNow;
     bipSmoothStd=(1.0f-alpha)*bipSmoothStd+alpha*bipStdNow;
    }
   }

   cmImage.fill(Qt::white);
   QPainter p(&cmImage);
   p.setRenderHint(QPainter::Antialiasing,true); p.setPen(Qt::black);
   p.drawRect(0,0,conf->frameW-1,conf->frameH-1); p.drawText(10,20,QString("AMP %1").arg(ampNo+1));

   unsigned tXmax=0,tYmax=0;

   auto updateMaxima=[&](const QVector<GUIChnInfo> &chs) {
    for (const auto &ch:chs) {
     if ((unsigned)ch.topoX>tXmax) tXmax=ch.topoX;
     if ((unsigned)ch.topoY>tYmax) tYmax=ch.topoY;
    }
   };

   updateMaxima(conf->refChns); updateMaxima(conf->bipChns); updateMaxima(conf->metaChns);

   if (tXmax==0||tYmax==0) { p.end(); update(); return; }

   const float w=float(conf->frameW); const float aspect=0.95f;
   const float cellSize=w/float(tXmax); const int markerSize=qMax(12,int(aspect*cellSize));
   const float cf=float(tXmax)/(float(tXmax)+1.0f);

   auto drawChannel=[&](const GUIChnInfo &ch,int dataIdx,float center,float spread) {
    if (!validDataIndex(dataIdx)) return;

    const float rawValue=conf->curCMData[int(ampNo)][dataIdx];
    const float z=(rawValue-center)/qMax(spread,1e-6f);
    const QColor col=paletteColor(z);

    const unsigned topoX=unsigned(ch.topoX); const unsigned topoY=unsigned(ch.topoY);

    unsigned int yOffset=(cellSize/2)*cf;
    if (topoY>1) yOffset+=cellSize/2;
    if (topoY>10) yOffset+=cellSize/2;

    const int x=int(cellSize/2-markerSize/2+cellSize*(topoX-1));
    const int y=int(cellSize/2-markerSize/2+cellSize*(topoY-1)+yOffset);

    QRect cr(x,y,markerSize,markerSize);

    p.setBrush(col); p.setPen(Qt::black); p.drawEllipse(cr);

    QFont f=p.font(); f.setPointSize(qMax(6,markerSize/5)); f.setBold(true); p.setFont(f);
    p.drawText(cr,Qt::AlignCenter,ch.chnName);
   };

   for (int i=0;i<conf->refChns.size();++i) {
    drawChannel(conf->refChns[i],i,refSmoothCenter,refSmoothStd);
   }

   for (int i=0;i<conf->bipChns.size();++i) {
    drawChannel(conf->bipChns[i],int(conf->refChnCount)+i,bipSmoothCenter,bipSmoothStd);
   }

   // No meta -- and probably won't be.

   p.end();
   update();
  }

  QImage cmImage;

 protected:
  void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QPainter mainPainter(this);
   mainPainter.drawImage(0,0,cmImage);
  }

 private:
  float computeCenter(const std::vector<float> &vals) const {
   if (vals.empty()) return 0.0f;
   return (centerMode==UseMedian) ? computeMedian(vals):computeMean(vals);
  }

  static float computeMean(const std::vector<float> &vals) {
   if (vals.empty()) return 0.0f;
   double sum=0.0;
   for (float v:vals) sum+=v;
   return float(sum/double(vals.size()));
  }

  static float computeMedian(std::vector<float> vals) {
   if (vals.empty()) return 0.0f;

   const size_t n=vals.size(); const size_t mid=n/2;

   std::nth_element(vals.begin(),vals.begin()+mid,vals.end());
   float med=vals[mid];

   if ((n%2)==0) {
    const auto maxIt=std::max_element(vals.begin(),vals.begin()+mid);
    med=0.5f*(med+*maxIt);
   }
   return med;
  }

  static float computeStd(const std::vector<float> &vals,float center) {
   if (vals.empty()) return 1.0f;
   double var=0.0;
   for (float v:vals) { const double d=double(v)-double(center); var+=d*d; }
   double s=std::sqrt(var/double(vals.size()));
   if (s<1e-6) s=1e-6;
   return float(s);
  }

  static QColor paletteColor(float z) {
   float t=(z+8.0f)/16.0f; // found after some trial & error
   if (t<0.0f) t=0.0f;
   if (t>1.0f) t=1.0f;

   const int idx=qBound(0,int(t*255.0f),255);

   if (idx<128) return QColor(2*idx,255,0); // green -> yellow
   return QColor(255,255-2*(idx-128),0);    // yellow -> red
  }

  ConfParam *conf=nullptr; unsigned int ampNo=0;

  //CenterMode centerMode=UseMedian; // change to UseMean for testing
  CenterMode centerMode=UseMean;

  bool refStatsInit=false; float refSmoothCenter=0.0f; float refSmoothStd=1.0f;
  bool bipStatsInit=false; float bipSmoothCenter=0.0f; float bipSmoothStd=1.0f;
};
