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

#ifndef DEFORM_W_H
#define DEFORM_W_H

#include <QtGui>
#include <QString>
#include "octopus_seg_master.h"

class DeformW : public QWidget {
 Q_OBJECT
 public:
  DeformW(SegMaster *sm) : QWidget(0,Qt::SubWindow) { p=sm;
   setGeometry(540,40,500,300); setFixedSize(500,300);

   gvfIterLabel=new QLabel("GVF Iter# ("+
                           dummyString.setNum(p->gvfIter)+"):",this);
   gvfIterLabel->setGeometry(10,10,90,20);
   QSlider *gvfIterSlider=new QSlider(Qt::Horizontal,this);
   gvfIterSlider->setGeometry(100,10,200,20);
   gvfIterSlider->setRange(0,99);
   gvfIterSlider->setSingleStep(1);
   gvfIterSlider->setPageStep(1);
   gvfIterSlider->setSliderPosition(p->gvfIter-1);
   gvfIterSlider->setEnabled(true);
   connect(gvfIterSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetGVFIter(int)));

   gvfMuLabel=new QLabel("GVF mu# ("+
                         dummyString.setNum(p->gvfMu)+"):",this);
   gvfMuLabel->setGeometry(10,10+22*1,90,20);
   QSlider *gvfMuSlider=new QSlider(Qt::Horizontal,this);
   gvfMuSlider->setGeometry(100,10+22*1,200,20);
   gvfMuSlider->setRange(0,49);
   gvfMuSlider->setSingleStep(1);
   gvfMuSlider->setPageStep(1);
   gvfMuSlider->setSliderPosition(p->gvfMu*100-1);
   gvfMuSlider->setEnabled(true);
   connect(gvfMuSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetGVFMu(int)));


   defIterLabel=new QLabel("Def.Iter#:",this);
   defIterLabel->setGeometry(10,10+22*2,90,20);


   defAlphaLabel=new QLabel("Def.alpha#:",this);
   defAlphaLabel->setGeometry(10,10+22*3,90,20);


   defBetaLabel=new QLabel("Def.beta#:",this);
   defBetaLabel->setGeometry(10,10+22*4,90,20);


   defTauLabel=new QLabel("Def.tau#:",this);
   defTauLabel->setGeometry(10,10+22*5,90,20);


   modelRxLabel=new QLabel("Model Rx#:",this);
   modelRxLabel->setGeometry(10,10+22*6,90,20);


   modelRyLabel=new QLabel("Model Ry#:",this);
   modelRyLabel->setGeometry(10,10+22*7,90,20);


   modelRzLabel=new QLabel("Model Rz#:",this);
   modelRzLabel->setGeometry(10,10+22*8,90,20);



   setWindowTitle("Deformable Model Parameters");
  }

 private slots:
  void slotSetGVFIter(int x) { p->gvfIter=x+1;
   printf("%d\n",p->gvfIter);
   gvfIterLabel->setText("GVF Iter# ("+dummyString.setNum(p->gvfIter)+"):");
  }
  void slotSetGVFMu(int x) { p->gvfMu=(float)(x+1)/100.;
   printf("%2.2f\n",p->gvfMu);
   gvfMuLabel->setText("GVF mu# ("+dummyString.setNum(p->gvfMu)+"):");
  }

 private:
  SegMaster *p; QString dummyString;
  QLabel *gvfIterLabel,*gvfMuLabel,*defIterLabel,
         *defAlphaLabel,*defBetaLabel,*defTauLabel,
         *modelRxLabel,*modelRyLabel,*modelRzLabel;
};

#endif
