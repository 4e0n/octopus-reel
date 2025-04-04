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

#ifndef ACQDAEMONGUI_H
#define ACQDAEMONGUI_H

#include <QtGui>
#include <QMainWindow>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

#include "acqdaemon.h"
#include "cmlevelframe.h"

class AcqDaemonGUI : public QMainWindow {
 Q_OBJECT
 public:
  AcqDaemonGUI(AcqDaemon *acqd,QWidget *parent=0) : QMainWindow(parent) {
   acqD=acqd; setGeometry(acqD->acqGuiX,acqD->acqGuiY,acqD->acqGuiW,acqD->acqGuiH);
   setFixedSize(acqD->acqGuiW,acqD->acqGuiH);

   CMLevelFrame *cml;
   for (unsigned int i=0;i<acqD->confAmpCount;i++) {
    cml=new CMLevelFrame(this,acqD,i);
    cml->setGeometry(20+i*(acqD->cmLevelFrameW+20),20,acqD->cmLevelFrameW,acqD->cmLevelFrameH);
    cmLevelFrame.append(cml); cml->show();
   }
   guiMutex=&(acqD->guiMutex);
   setWindowTitle("Octopus AcqDaemon GUI Window");
  }

 private:
  AcqDaemon *acqD; QMutex *guiMutex;
  QVector<CMLevelFrame*> cmLevelFrame;
};

#endif
