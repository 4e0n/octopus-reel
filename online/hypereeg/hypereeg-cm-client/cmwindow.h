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

#ifndef ACQCONTROLWINDOW_H
#define ACQCONTROLWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include "confparam.h"
#include "cmframe.h"

class CMWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit CMWindow(ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
   conf=c; setGeometry(conf->guiX,conf->guiY,conf->guiW,conf->guiH);
   setFixedSize(conf->guiW,conf->guiH);

   statusBar=new QStatusBar(this); statusBar->setGeometry(0,height()-20,width(),20);
   statusBar->show(); setStatusBar(statusBar);

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(conf->guiW);
   QMenu *sysMenu=new QMenu("&System",menuBar);
   menuBar->addMenu(sysMenu);

   // System Menu
   rebootAction=new QAction("&Reboot Servers",this);
   shutdownAction=new QAction("&Shutdown Servers",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   rebootAction->setStatusTip("Reboot the ACQuisition and STIM servers");
   shutdownAction->setStatusTip("Shutdown the ACQuisition and STIM servers");
   aboutAction->setStatusTip("About OctopusHCM GUI..");
   quitAction->setStatusTip("Quit OctopusHCM GUI");
   sysMenu->addAction(rebootAction);
   sysMenu->addAction(shutdownAction); sysMenu->addSeparator();
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);
//   connect(rebootAction,&QAction::triggered,this,[=](){statusBar->setText("Status: Reboot daemon host");});
//   connect(shutdownAction,&QAction::triggered,this,[=](){statusBar->setText("Status: Shutdown daemon host");});
//   connect(aboutAction,&QAction::triggered,this,[=](){statusBar->setText("Status: About Octopus Hyper CM Client");});
//   connect(quitAction,&QAction::triggered,this,[=](){statusBar->setText("Status: Quit Octopus Hyper CM Client");});
   connect(rebootAction,SIGNAL(triggered()),this,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),this,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));

   CMFrame *cmF;
   for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) {
    cmF=new CMFrame(conf,ampIdx,this);
    cmF->show(); cmFrame.append(cmF);
   }
   setWindowTitle("Octopus HyperEEG Common Mode Levels");
  }

  ~CMWindow() override {}

  ConfParam *conf;

 signals: void updateData();

 private slots:

  // *** MENUS ***

  void slotReboot() {}
  void slotShutdown() {}

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG/CM Client",
                           "EEG Common Mode Dashboard (CM GUI)\n"
                           "(c) 2007-2025 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   conf->application->quit();
  }

 private:
  QStatusBar *statusBar; QMenuBar *menuBar; QMenu *sysMenu;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction;
  QVector<CMFrame*> cmFrame;
};

#endif
