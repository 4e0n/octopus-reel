/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#ifndef CMFRAME_H
#define CMFRAME_H

#include <QFrame>
#include <QPainter>
#include "confparam.h"
#include "cmthread.h"

class CMFrame : public QFrame {
 Q_OBJECT
 public:
  explicit CMFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr) : QFrame(parent) {
   conf=c; ampNo=a;

   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    setGeometry(20+ampNo*(conf->frameW+20),20,conf->frameW,conf->frameH);
   } else {
    setGeometry(20+(conf->ampCount%(conf->ampCount/2))*(conf->frameW+20),
                20+(conf->ampCount/(conf->ampCount/2))*(conf->frameH+20),
                conf->frameW,conf->frameH);
   }

   cmBuffer=QPixmap(conf->frameW,conf->frameH);
   cmThread=new CMThread(conf,ampNo,&cmBuffer,this);
   connect(cmThread,&CMThread::updateCMFrame,this,QOverload<>::of(&CMFrame::update));
   cmThread->start();
  }

  QPixmap cmBuffer; CMThread *cmThread;

 protected:
  virtual void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   mainPainter.begin(this);
   mainPainter.drawPixmap(0,0,cmBuffer);
   mainPainter.end();
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPainter mainPainter;
};

#endif
