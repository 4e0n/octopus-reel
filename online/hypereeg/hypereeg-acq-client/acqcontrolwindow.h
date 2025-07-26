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
#include <QThread>
#include "unistd.h"
#include "confparam.h"

class AcqControlWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit AcqControlWindow(ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
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
   rebootAction=new QAction("&Reboot Servers",this);
   shutdownAction=new QAction("&Shutdown Servers",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   rebootAction->setStatusTip("Reboot the ACQuisition and STIM servers");
   shutdownAction->setStatusTip("Shutdown the ACQuisition and STIM servers");
   aboutAction->setStatusTip("About Octopus GUI..");
   quitAction->setStatusTip("Quit Octopus GUI");
   //connect(rebootAction,&QAction::triggered,this,[=](){ctrlStatusBar->setText("Status: Quit Action");});
   connect(rebootAction,SIGNAL(triggered()),this,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),this,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));
   sysMenu->addAction(rebootAction);
   sysMenu->addAction(shutdownAction); sysMenu->addSeparator();
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // *** BUTTONS AT THE TOP ***

   toggleRecordingButton=new QPushButton("RECORD",cntWidget);
   toggleRecordingButton->setGeometry(2,mainTabWidget->height()-54,100,20);
   toggleRecordingButton->setCheckable(true);
   connect(toggleRecordingButton,SIGNAL(clicked()),this,SLOT(slotToggleRecording()));

   timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   timeLabel->setGeometry(110,mainTabWidget->height()-52,190,20);

   manualSyncButton=new QPushButton("SYNC",cntWidget);
   manualSyncButton->setGeometry(460,mainTabWidget->height()-54,60,20);
   connect(manualSyncButton,SIGNAL(clicked()),this,SLOT(slotManualSync()));

   manualTrigButton=new QPushButton("PING",cntWidget);
   manualTrigButton->setGeometry(550,mainTabWidget->height()-54,60,20);
   connect(manualTrigButton,SIGNAL(clicked()),this,SLOT(slotManualTrig()));

   toggleNotchButton=new QPushButton("Notch",cntWidget);
   toggleNotchButton->setGeometry(650,mainTabWidget->height()-54,60,20);
   toggleNotchButton->setCheckable(true); toggleNotchButton->setChecked(true);
   connect(toggleNotchButton,SIGNAL(clicked()),this,SLOT(slotToggleNotch()));

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   QPushButton *dummyButton;
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
   scrSpeedBG->button(4)->setText("1"); scrSpeedBG->button(2)->setDown(true);

   connect(scrSpeedBG,SIGNAL(buttonClicked(int)),this,SLOT(slotScrollSpeed(int)));

   setWindowTitle("Octopus HyperEEG/ERP Streaming/GL Client");
  }

  ~AcqControlWindow() override {}

  ConfParam *conf;

 signals: void scrollData();

 private slots:

  // *** MENUS ***

  void slotReboot() {}
  void slotShutdown() {}

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG/ERP Client",
                           "EEG/ERP Dashboard (VisRec GUI)\n"
                           "(c) 2007-2025 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   { QMutexLocker locker(&conf->mutex); conf->quitPending=true; }

   if (conf->eegDataSocket->state() != QAbstractSocket::UnconnectedState)
    conf->eegDataSocket->waitForDisconnected(1000); // timeout in ms
   if (conf->cmDataSocket->state() != QAbstractSocket::UnconnectedState)
    conf->cmDataSocket->waitForDisconnected(1000); // timeout in ms
   //while (conf->eegDataSocket->state() != QAbstractSocket::UnconnectedState);
   //while (conf->cmDataSocket->state() != QAbstractSocket::UnconnectedState);

   for (auto& thread:conf->threads) { thread->wait(); delete thread; }
   
   QApplication::quit();
  }

  void slotToggleRecording() {}
  void slotManualSync() {}
  void slotManualTrig() {}

  void slotToggleNotch() {
   {
    QMutexLocker locker(&conf->mutex);
    conf->notchActive ? conf->notchActive=false : conf->notchActive=true;
   }
  }

  void slotScrollSpeed(int x) { conf->eegScrollDivider=conf->eegScrollCoeff[x]; }

 private:
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QStatusBar *ctrlStatusBar; QMenuBar *menuBar;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction;
  QLabel *timeLabel; QPushButton *manualSyncButton,*manualTrigButton;
  QPushButton *toggleRecordingButton,*toggleNotchButton;
  QButtonGroup *scrSpeedBG; QVector<QPushButton*> cntSpeedButtons;
};

#endif
