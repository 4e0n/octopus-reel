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
#include <QPainter>
#include "../../../common/event.h"
#include "confparam.h"

class GLLegendFrame : public QFrame {
 Q_OBJECT
 public:
  GLLegendFrame(ConfParam *c=nullptr,QWidget *p=nullptr) : QFrame(p) { conf=c; parent=p; }

  ConfParam *conf;

 public slots:
  void slotRepaintLegend() { repaint(); }

 protected:
  virtual void paintEvent(QPaintEvent*) {
   mainPainter.begin(this);
    mainPainter.setBackground(QBrush(Qt::white));
    mainPainter.setBackgroundMode(Qt::OpaqueMode);
    mainPainter.setPen(Qt::black);
    mainPainter.eraseRect(frameRect()); mainPainter.drawRect(frameRect());
    mainPainter.drawText(16,16,"#   Event           Color    Accepted  Rejected");
    mainPainter.drawLine(4,22,width()-4,22);
    for (int i=0;i<acqEvents.size();i++) {
     if (acqEvents[i]->accepted || acqEvents[i]->rejected) {
      noStr.setNum(acqEvents[i]->no);
      accStr.setNum(acqEvents[i]->accepted);
      rejStr.setNum(acqEvents[i]->rejected);
      mainPainter.setPen(Qt::black);
      mainPainter.drawText(8,45+i*20,noStr);
      mainPainter.drawText(30,45+i*20,acqEvents[i]->name);
      mainPainter.drawText(174,45+i*20,accStr);
      mainPainter.drawText(236,45+i*20,rejStr);
      QBrush eBrush(conf->rgbPalette[acqEvents[i]->colorIndex]);
      mainPainter.fillRect(122,45+i*20-10,25,6,eBrush);
     }
    }
   mainPainter.end();
  }

 private:
  QWidget *parent; QString noStr,accStr,rejStr; QVector<Event*> acqEvents;
  QBrush bgBrush; QPainter mainPainter; QFont evtFont;
};
