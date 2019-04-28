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

#ifndef REGGROW_W_H
#define REGGROW_W_H

#include <QtGui>
#include "octopus_bem_master.h"

class RegGrowW : public QWidget {
 Q_OBJECT
 public:
  RegGrowW(BEMMaster *sm) : QWidget(0,Qt::SubWindow) {
   p=sm; setGeometry(p->guiX+p->guiWidth+20,p->guiY,170,190);
   setFixedSize(170,190);

   bndLabel=new QLabel(dummyString.setNum(p->floodBoundary),this);
   bndLabel->setGeometry(10,10,30,20);

   QSlider *bndSlider=new QSlider(Qt::Vertical,this);
   bndSlider->setGeometry(12,30,20,156);
   bndSlider->setRange(0,255);
   bndSlider->setSingleStep(1);
   bndSlider->setPageStep(1);
   bndSlider->setSliderPosition(p->thrValue);
   bndSlider->setEnabled(true);
   connect(bndSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetBndValue(int)));

   QCheckBox *binCheckBox=new QCheckBox("Binarization",this);
   binCheckBox->setGeometry(50,20,120,20);
   connect(binCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotFldBinSet(int)));

   QCheckBox *invCheckBox=new QCheckBox("Invert",this);
   invCheckBox->setGeometry(50,50,120,20);
   connect(invCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotFldInvSet(int)));

   setWindowTitle("Region Growing");
  }

 private slots:
  void slotSetBndValue(int x) {
   bndLabel->setText(dummyString.setNum(x)); p->floodBoundary=x;
  }
  void slotFldBinSet(int x) { p->thrBin=(bool)x; }
  void slotFldInvSet(int x) { p->thrInv=(bool)x; }

 private:
  BEMMaster *p; QLabel *bndLabel; QString dummyString;
};

#endif
