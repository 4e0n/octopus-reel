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
#include <QDateTime>
#include <QThread>
#include "unistd.h"
#include "confparam.h"
#include "../common/tcp_commands.h"

class ControlWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit ControlWindow(ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
   conf=c; setGeometry(conf->guiCtrlX,conf->guiCtrlY,conf->guiCtrlW,conf->guiCtrlH);
   setFixedSize(conf->guiCtrlW,conf->guiCtrlH);

   // *** TABS & TABWIDGETS ***
   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,width()-2,height()-60);
   cntWidget=new QWidget(mainTabWidget);
   cntWidget->setGeometry(0,0,mainTabWidget->width(),mainTabWidget->height());
   cntWidget->show();
   mainTabWidget->addTab(cntWidget,"EEG"); mainTabWidget->show();

   // *** HEAD & CONFIG WINDOW ***

   // *** STATUSBAR ***
   ctrlStatusBar=new QStatusBar(this);
   ctrlStatusBar->setGeometry(0,height()-20,width(),20);
   ctrlStatusBar->show(); setStatusBar(ctrlStatusBar);

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *sysMenu=new QMenu("&System",menuBar);
   menuBar->addMenu(sysMenu);

   // System Menu
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   aboutAction->setStatusTip("About HyperStream GUI..");
   quitAction->setStatusTip("Quit HyperStream GUI");
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // *** BUTTONS AT THE TOP ***

   QPushButton *dummyButton;

   eegBandBG=new QButtonGroup();
   eegBandBG->setExclusive(true);
   for (int bandIdx=0;bandIdx<6;bandIdx++) { // EEG frequency band
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-760+bandIdx*70,mainTabWidget->height()-54,70,20);
    eegBandBG->addButton(dummyButton,bandIdx);
   }
   eegBandBG->button(0)->setText("2-40Hz");
   eegBandBG->button(1)->setText("Delta");
   eegBandBG->button(2)->setText("Theta");
   eegBandBG->button(3)->setText("Alpha");
   eegBandBG->button(4)->setText("Beta");
   eegBandBG->button(5)->setText("Gamma"); eegBandBG->button(0)->setChecked(true);
   connect(eegBandBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGBand(int)));

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   scrSpeedBG=new QButtonGroup();
   scrSpeedBG->setExclusive(true);
   for (int speedIdx=0;speedIdx<5;speedIdx++) { // EEG Scroll speed/resolution
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-310+speedIdx*60,mainTabWidget->height()-54,60,20);
    scrSpeedBG->addButton(dummyButton,speedIdx);
   }
   scrSpeedBG->button(0)->setText("10");
   scrSpeedBG->button(1)->setText("5");
   scrSpeedBG->button(2)->setText("4");
   scrSpeedBG->button(3)->setText("2");
   scrSpeedBG->button(4)->setText("1"); scrSpeedBG->button(0)->setChecked(true);
   connect(scrSpeedBG,SIGNAL(buttonClicked(int)),this,SLOT(slotScrollSpeed(int)));

   setWindowTitle("Octopus HyperEEG/ERP Streaming/GL Client");
  }

  ~ControlWindow() override {}

  ConfParam *conf;

 signals: void streamData();

 private slots:

  // *** MENUS ***

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG Streamer GUI Client Node",
                           "(c) 2007-2026 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   { QMutexLocker locker(&conf->mutex); conf->quitPending=true; }

   if (conf->acqStrmSocket->state() != QAbstractSocket::UnconnectedState)
    conf->acqStrmSocket->waitForDisconnected(1000); // timeout in ms

   for (auto& thread:conf->threads) { thread->wait(); delete thread; }
   
   QApplication::quit();
  }

  void slotScrollSpeed(int x) { conf->eegSweepDivider=conf->eegSweepCoeff[x]; }
  void slotEEGBand(int x) { conf->eegBand=x; }

 private:
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QStatusBar *ctrlStatusBar; QMenuBar *menuBar;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction;
  QButtonGroup *eegBandBG,*scrSpeedBG; QVector<QPushButton*> cntSpeedButtons;
};
