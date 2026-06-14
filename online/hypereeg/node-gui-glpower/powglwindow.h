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
#include <QHBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QMutexLocker>
#include "confparam.h"
#include "powglwidget.h"

class PowGLWindow:public QMainWindow {
 Q_OBJECT
 public:
  explicit PowGLWindow(ConfParam *c=nullptr,QWidget *parent=nullptr):QMainWindow(parent) {
   conf=c; setGeometry(conf->guiX,conf->guiY+conf->guiH+90,conf->guiW,conf->guiH); setFixedSize(conf->guiW,conf->guiH);

   QWidget *central=new QWidget(this); QHBoxLayout *lay=new QHBoxLayout(central);
   lay->setContentsMargins(10,10,10,10); lay->setSpacing(10);

   for (unsigned int ampIdx=0;ampIdx<conf->ampCount;++ampIdx) {
    PowGLWidget *w=new PowGLWidget(conf,ampIdx,central,PowGLWidget::ModePower);
    connect(w,&PowGLWidget::viewChanged,this,&PowGLWindow::updateGLFrames);
    lay->addWidget(w,1);
    glFrames.append(w);
   }

   PowGLWidget *cw=new PowGLWidget(conf,0,central,PowGLWidget::ModeCorrelation);
   connect(cw,&PowGLWidget::viewChanged,this,&PowGLWindow::updateGLFrames);
   lay->addWidget(cw,1);
   glFrames.append(cw);

   rotTimer=new QTimer(this);
   connect(rotTimer,&QTimer::timeout,this,&PowGLWindow::slotRotate);
   rotTimer->start(conf->glRotateMs);

   setCentralWidget(central); setWindowTitle("Octopus-HyperEEG GL Power Topography");
  }

  void updateGLFrames() { for (auto *f:glFrames) if (f) f->refreshImage(); }

  void rebuildInterpolations() {
   for (auto *f:glFrames) { if (f) f->rebuildInterpolation(); }
  }

  void showNode() { show(); raise(); activateWindow(); }
  void hideNode() { hide(); }

 private slots:
  void slotRotate() {
   if (!conf || !conf->glAutoRotate) return;
   {
    QMutexLocker lk(&conf->glViewMutex);
    // Rotate around Z: head spins in XY plane.
    conf->glZRot+=conf->glRotateStep;
    while (conf->glZRot<0) conf->glZRot+=360*16;
    while (conf->glZRot>360*16) conf->glZRot-=360*16;
   }
   updateGLFrames();
  }

 private:
  ConfParam *conf=nullptr; QVector<PowGLWidget*> glFrames; QTimer *rotTimer=nullptr;
};
