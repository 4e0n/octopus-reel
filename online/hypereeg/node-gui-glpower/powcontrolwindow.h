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

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include "confparam.h"

class PowControlWindow:public QMainWindow {
 Q_OBJECT
 public:
  explicit PowControlWindow(ConfParam *c=nullptr,QWidget *parent=nullptr):QMainWindow(parent) {
   conf=c;

   QWidget *w=new QWidget(this);
   QVBoxLayout *lay=new QVBoxLayout(w);

   addFloatSlider(lay,"Map gain",0,200,int(conf->glMapGain*100.0f),
                  [this](int v){ conf->glMapGain=float(v)/100.0f; emit parametersChanged(false); });

   addIntSlider(lay,"Map alpha",0,255,int(conf->glMapAlpha),
                [this](int v){ conf->glMapAlpha=v; emit parametersChanged(false); });

   addFloatSlider(lay,"Corr dead zone",0,100,int(conf->corrDeadZone*100.0f),
                  [this](int v){ conf->corrDeadZone=float(v)/100.0f; emit parametersChanged(false); });

   addIntSlider(lay,"Interp neighbors",3,32,conf->glInterpNeighbors,
                [this](int v){ conf->glInterpNeighbors=v; emit parametersChanged(true); });

   addFloatSlider(lay,"Interp power",10,300,int(conf->glInterpPower*100.0f),
                  [this](int v){ conf->glInterpPower=float(v)/100.0f; emit parametersChanged(true); });

   QCheckBox *scalp=new QCheckBox("Show scalp",w); scalp->setChecked(conf->showScalp);
   connect(scalp,&QCheckBox::toggled,this,[this](bool on){ conf->showScalp=on; emit parametersChanged(false); });
   lay->addWidget(scalp);

   QCheckBox *brain=new QCheckBox("Show brain",w); brain->setChecked(conf->showBrain);
   connect(brain,&QCheckBox::toggled,this,[this](bool on){ conf->showBrain=on; emit parametersChanged(false); });
   lay->addWidget(brain);

   QCheckBox *elec=new QCheckBox("Show electrodes",w); elec->setChecked(conf->showElectrodes);
   connect(elec,&QCheckBox::toggled,this,[this](bool on){ conf->showElectrodes=on; emit parametersChanged(false); });
   lay->addWidget(elec);

   QCheckBox *rot=new QCheckBox("Auto rotate",w); rot->setChecked(conf->glAutoRotate);
   connect(rot,&QCheckBox::toggled,this,[this](bool on){ conf->glAutoRotate=on; emit parametersChanged(false); });
   lay->addWidget(rot);

   setCentralWidget(w); setWindowTitle("GLPower Controls");
   //setGeometry(conf->guiX+conf->guiW+30,conf->guiY+400,360,360);
   setGeometry(100,conf->guiY+600,360,360);
  }

 signals:
  void parametersChanged(bool rebuildInterpolation);

 private:
  template<typename Func> void addIntSlider(QVBoxLayout *lay,const QString &name,int minV,int maxV,int val,Func fn) {
   QLabel *lab=new QLabel(QString("%1: %2").arg(name).arg(val),this);
   QSlider *s=new QSlider(Qt::Horizontal,this);
   s->setRange(minV,maxV); s->setValue(val);
   connect(s,&QSlider::valueChanged,this,[=](int v){ lab->setText(QString("%1: %2").arg(name).arg(v)); fn(v); });
   lay->addWidget(lab); lay->addWidget(s);
  }

  template<typename Func>
  void addFloatSlider(QVBoxLayout *lay,const QString &name,int minV,int maxV,int val,Func fn) {
   QLabel *lab=new QLabel(QString("%1: %2").arg(name).arg(float(val)/100.0f,0,'f',2),this);
   QSlider *s=new QSlider(Qt::Horizontal,this);
   s->setRange(minV,maxV); s->setValue(val);
   connect(s,&QSlider::valueChanged,this,[=](int v){ lab->setText(QString("%1: %2").arg(name).arg(float(v)/100.0f,0,'f',2)); fn(v); });
   lay->addWidget(lab); lay->addWidget(s);
  }

  ConfParam *conf=nullptr;
};
