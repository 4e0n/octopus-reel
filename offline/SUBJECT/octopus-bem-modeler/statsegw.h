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

#ifndef STATSEG_W_H
#define STATSEG_W_H

#include <QtGui>
#include "octopus_bem_master.h"

class StatSegW : public QWidget {
 Q_OBJECT
 public:
  StatSegW(BEMMaster *sm) : QWidget(0,Qt::SubWindow) {
   p=sm; setGeometry(540,40,200,100); setFixedSize(200,100);

   centroidLabel=new QLabel("Centroid Count ("+
                            dummyString.setNum(p->kMeansCCount)+"):",this);
   centroidLabel->setGeometry(10,10,190,20);

   QSlider *centroidSlider=new QSlider(Qt::Horizontal,this);
   centroidSlider->setGeometry(10,30,180,20);
   centroidSlider->setRange(0,9);
   centroidSlider->setSingleStep(1);
   centroidSlider->setPageStep(1);
   centroidSlider->setSliderPosition(p->kMeansCCount-1);
   centroidSlider->setEnabled(true);
   connect(centroidSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetKMeansCCount(int)));

   setWindowTitle("Statistical Segmentation");
  }

 private slots:
  void slotSetKMeansCCount(int x) { p->kMeansCCount=x+1;
   centroidLabel->setText("Centroid Count ("+
                          dummyString.setNum(x+1)+"):");
  }

 private:
  BEMMaster *p; QLabel *centroidLabel; QString dummyString;
};

#endif
