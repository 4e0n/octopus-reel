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
#include <algorithm>
#include "confparam.h"

class PowCorrFrame:public QFrame {
 Q_OBJECT

 public:
  explicit PowCorrFrame(ConfParam *c=nullptr,QWidget *parent=nullptr):QFrame(parent),conf(c) {
   corrImage=QImage(width(),height(),QImage::Format_RGB32); corrImage.fill(Qt::white);
  }

  void refreshImage() {
   const int W=width(); const int H=height(); if (W<=0 || H<=0) return;

   corrImage=QImage(W,H,QImage::Format_RGB32); corrImage.fill(Qt::white);

   QPainter p(&corrImage); p.setRenderHint(QPainter::Antialiasing,true);
   p.setPen(Qt::black); p.drawRect(0,0,W-1,H-1);

   QFont titleFont=p.font(); titleFont.setBold(true); titleFont.setPointSize(12); p.setFont(titleFont);
   p.drawText(12,24,"Inter-subject GFP co-change");

   if (conf->spatialCorrValid && conf->glSelectedBand>=0 && conf->glSelectedBand<conf->curSpatialCorr.size()) {
    float r=conf->curSpatialCorr[conf->glSelectedBand]; r=qBound(-1.0f,r,1.0f);
    const float t=0.5f*(r+1.0f);
    QColor c;
    if (t<0.5f) { float u=t*2.0f; c=QColor(int(255*u),int(255*u),255); }
    else { float u=(t-0.5f)*2.0f; c=QColor(255,int(255*(1.0f-u)),int(255*(1.0f-u))); }

    p.setBrush(c); p.setPen(Qt::black); p.drawEllipse(QRect(width()-68,14,46,46)); p.setBrush(Qt::NoBrush);

    QFont f=p.font(); f.setBold(true); f.setPointSize(10); p.setFont(f);
    p.drawText(QRect(width()-72,62,56,18),Qt::AlignCenter,QString::number(r,'f',2));
    p.setFont(titleFont); p.drawText(12,43,"Spatio-temporal similarity");
   }

   if (!conf || !conf->corrValuesValid || conf->curCorr.size()<ConfParam::POWER_BAND_COUNT) {
    p.drawText(12,55,"No correlation data yet"); p.end();
    update();
    return;
   }

   const QString labels[ConfParam::POWER_BAND_COUNT]={"overall","delta","theta","alpha","beta","gamma"};

   const int left=45; const int right=20; const int top=75; const int bottom=45;

   const int plotW=W-left-right; const int plotH=H-top-bottom; const int midY=top+plotH/2;

   p.setPen(Qt::black); p.drawLine(left,top,left,top+plotH); p.drawLine(left,midY,left+plotW,midY);

   p.setPen(Qt::darkGray); p.drawText(8,top+8,"+1"); p.drawText(12,midY+4,"0"); p.drawText(8,top+plotH,"-1");

   const int n=ConfParam::POWER_BAND_COUNT; const int gap=10; const int barW=(plotW-gap*(n+1))/n;
   QFont labFont=p.font(); labFont.setBold(false); labFont.setPointSize(9); p.setFont(labFont);

   for (int b=0;b<n;++b) {
    float r=conf->curCorr[b]; r=std::max(-1.0f,std::min(1.0f,r));
    const int x=left+gap+b*(barW+gap); const int h=int(std::fabs(r)*float(plotH/2));
    QRect br; if (r>=0.0f) br=QRect(x,midY-h,barW,h); else br=QRect(x,midY,barW,h);

    p.setBrush(corrColor(r)); p.setPen(Qt::black); p.drawRect(br); p.setBrush(Qt::NoBrush);

    p.drawText(QRect(x,top+plotH+4,barW,18),Qt::AlignCenter,labels[b]);
    p.drawText(QRect(x,br.top()-20,barW,18),Qt::AlignCenter,QString::number(r,'f',2));
   }

   p.end();
   update();
  }

 protected:
  void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QPainter painter(this); painter.drawImage(0,0,corrImage);
  }

 private:
  static QColor corrColor(float r) {
   if (r>=0.0f) return QColor(220,80,80);
   return QColor(80,120,230);
  }

  ConfParam *conf=nullptr; QImage corrImage;
};
