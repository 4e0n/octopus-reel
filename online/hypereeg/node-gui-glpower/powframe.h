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
#include <algorithm>
#include "confparam.h"

class PowFrame:public QFrame {
 Q_OBJECT

 public:
  explicit PowFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr):QFrame(parent),conf(c),ampNo(a) {
   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    setGeometry(20+ampNo*(conf->frameW+20),20,conf->frameW,conf->frameH);
   } else {
    const int perRow=conf->guiMaxAmpPerLine;
    const int row=int(ampNo)/perRow;
    const int col=int(ampNo)%perRow;
    setGeometry(20+col*(conf->frameW+20),20+row*(conf->frameH+20),conf->frameW,conf->frameH);
   }
   powImage=QImage(conf->frameW,conf->frameH,QImage::Format_RGB32);
   powImage.fill(Qt::white);
  }

  void refreshImage() {
   powImage.fill(Qt::white);

   QPainter p(&powImage);
   p.setRenderHint(QPainter::Antialiasing,true);

   p.setPen(Qt::black);
   p.drawRect(0,0,conf->frameW-1,conf->frameH-1);

   QFont titleFont=p.font();
   titleFont.setBold(true);
   titleFont.setPointSize(12);
   p.setFont(titleFont);

   p.drawText(12,24,QString("Subject / AMP %1 - GFP Power").arg(ampNo+1));

   if (!validGFP()) {
    p.drawText(12,55,"No power data yet");
    p.end();
    update();
    return;
   }

   const int gfpIdx=int(conf->gfpAllIdx);

   float vals[ConfParam::POWER_BAND_COUNT];

   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
    vals[b]=conf->curPowData[int(ampNo)][gfpIdx][b];
   }

   float maxVal=0.0f;
   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b)
    maxVal=std::max(maxVal,vals[b]);

   if (maxVal<1e-9f) maxVal=1e-9f;

   const QString labels[ConfParam::POWER_BAND_COUNT] = {
    "overall","delta","theta","alpha","beta","gamma"
   };

   const int left=45;
   const int right=20;
   const int top=55;
   const int bottom=45;

   const int plotW=conf->frameW-left-right;
   const int plotH=conf->frameH-top-bottom;

   p.setPen(Qt::black);
   p.drawLine(left,top,left,top+plotH);
   p.drawLine(left,top+plotH,left+plotW,top+plotH);

   const int n=ConfParam::POWER_BAND_COUNT;
   const int gap=10;
   const int barW=(plotW-gap*(n+1))/n;

   QFont labFont=p.font();
   labFont.setBold(false);
   labFont.setPointSize(9);
   p.setFont(labFont);

   for (int b=0;b<n;++b) {
    const float norm=vals[b]/maxVal;
    const int h=int(norm*float(plotH));

    const int x=left+gap+b*(barW+gap);
    const int y=top+plotH-h;

    QRect br(x,y,barW,h);

    p.setBrush(barColor(b));
    p.setPen(Qt::black);
    p.drawRect(br);

    p.setBrush(Qt::NoBrush);

    p.drawText(QRect(x,top+plotH+4,barW,18),
               Qt::AlignCenter,
               labels[b]);

    p.drawText(QRect(x,top+plotH-22-h,barW,18),
               Qt::AlignCenter,
               QString::number(vals[b],'f',2));
   }

   p.setPen(Qt::darkGray);
   p.drawText(left,conf->frameH-8,
              QString("max=%1").arg(maxVal,0,'f',2));

   p.end();
   update();
  }

  QImage powImage;

 protected:
  void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QPainter mainPainter(this);
   mainPainter.drawImage(0,0,powImage);
  }

 private:
  bool validGFP() const {
   if (!conf) return false;
   if (ampNo>=unsigned(conf->curPowData.size())) return false;

   const int gfpIdx=int(conf->gfpAllIdx);

   if (gfpIdx<0) return false;
   if (gfpIdx>=conf->curPowData[int(ampNo)].size()) return false;
   if (conf->curPowData[int(ampNo)][gfpIdx].size()<ConfParam::POWER_BAND_COUNT) return false;

   return true;
  }

  static QColor barColor(int bandIdx) {
   switch (bandIdx) {
    case ConfParam::POWER_OVERALL: return QColor(80,80,80);
    case ConfParam::POWER_DELTA:   return QColor(60,120,255);
    case ConfParam::POWER_THETA:   return QColor(80,200,220);
    case ConfParam::POWER_ALPHA:   return QColor(80,200,80);
    case ConfParam::POWER_BETA:    return QColor(240,180,40);
    case ConfParam::POWER_GAMMA:   return QColor(230,70,70);
    default:                       return QColor(120,120,120);
   }
  }

  ConfParam *conf=nullptr;
  unsigned int ampNo=0;
};
