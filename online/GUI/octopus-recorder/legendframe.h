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

#ifndef LEGENDFRAME_H
#define LEGENDFRAME_H

#include <QFrame>
#include <QPainter>

#include "octopus_rec_master.h"

class LegendFrame : public QFrame {
 Q_OBJECT
 public:
  LegendFrame(QWidget *pnt,RecMaster *rm) : QFrame(pnt) { parent=pnt; p=rm; }

 public slots:

 protected:
  virtual void paintEvent(QPaintEvent*) {
   mainPainter.begin(this);
   mainPainter.setBackground(QBrush(Qt::white));
   mainPainter.setBackgroundMode(Qt::OpaqueMode);
   mainPainter.setPen(Qt::black);
   mainPainter.eraseRect(frameRect()); mainPainter.drawRect(frameRect());
   mainPainter.drawText(16,16,"#   Event           Color"
                              "    Accepted  Rejected");
   mainPainter.drawLine(4,22,width()-4,22);
   for (int i=0;i<p->acqEvents.size();i++) {
    if (p->acqEvents[i]->accepted || p->acqEvents[i]->rejected) {
     noStr.setNum(p->acqEvents[i]->no);
     accStr.setNum(p->acqEvents[i]->accepted);
     rejStr.setNum(p->acqEvents[i]->rejected);
     mainPainter.setPen(Qt::black);
     mainPainter.drawText(8,45+i*20,noStr);
     mainPainter.drawText(30,45+i*20,p->acqEvents[i]->name);
     mainPainter.drawText(174,45+i*20,accStr);
     mainPainter.drawText(236,45+i*20,rejStr);
     QBrush eBrush(p->acqEvents[i]->color);
     mainPainter.fillRect(122,45+i*20-10,25,6,eBrush);
    }
   }
   mainPainter.end();
  }

 private:
  QWidget *parent; RecMaster *p; QString noStr,accStr,rejStr;
  QBrush bgBrush; QPainter mainPainter; QFont evtFont;
};

#endif
