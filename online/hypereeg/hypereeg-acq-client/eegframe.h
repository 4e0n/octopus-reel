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

#ifndef EEGFRAME_H
#define EEGFRAME_H

#include <QFrame>
#include <QPainter>
#include "confparam.h"
#include "eegthread.h"

// Simple structure to hold mean and standard deviation for each time point
//struct EEGScrollPoint { float mean=0.0f; float stdev=0.0f; };

class EEGFrame : public QFrame {
 Q_OBJECT
 public:
  explicit EEGFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr) : QFrame(parent) {
   conf=c; ampNo=a; scrollSched=false;
   scrollBuffer=QPixmap(conf->eegFrameW,conf->eegFrameH);
   eegThread=new EEGThread(conf,ampNo,&scrollBuffer,this);
   connect(eegThread,&EEGThread::updateEEGFrame,this,QOverload<>::of(&EEGFrame::update));
   eegThread->start(QThread::HighestPriority);
  }

  bool scrollSched; QPixmap scrollBuffer; EEGThread *eegThread;

 protected:
  virtual void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   mainPainter.begin(this);
   mainPainter.drawPixmap(0,0,scrollBuffer); scrollSched=false;
   mainPainter.end();
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPainter mainPainter;
};

#endif
