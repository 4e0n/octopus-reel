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
   rebootAction=new QAction("&Reboot Servers",this);
   shutdownAction=new QAction("&Shutdown Servers",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   rebootAction->setStatusTip("Reboot node_acqd via hwd server");
   shutdownAction->setStatusTip("Shutdown ACQuisition host computer");
   aboutAction->setStatusTip("About HyperStream GUI..");
   quitAction->setStatusTip("Quit HyperStream GUI");
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

   conf->timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   conf->timeLabel->setGeometry(110,mainTabWidget->height()-52,190,20);

   manualSyncButton=new QPushButton("SYNC",cntWidget);
   manualSyncButton->setGeometry(460,mainTabWidget->height()-54,60,20);
   connect(manualSyncButton,SIGNAL(clicked()),this,SLOT(slotManualSync()));

   manualTrigButton=new QPushButton("PING",cntWidget);
   manualTrigButton->setGeometry(550,mainTabWidget->height()-54,60,20);
   connect(manualTrigButton,SIGNAL(clicked()),this,SLOT(slotManualTrig()));

   synthTrig1Button=new QPushButton("TRIG(1)",cntWidget);
   synthTrig1Button->setGeometry(640,mainTabWidget->height()-54,60,20);
   connect(synthTrig1Button,SIGNAL(clicked()),this,SLOT(slotSynthTrig1()));
   synthTrig2Button=new QPushButton("TRIG(2)",cntWidget);
   synthTrig2Button->setGeometry(730,mainTabWidget->height()-54,60,20);
   connect(synthTrig2Button,SIGNAL(clicked()),this,SLOT(slotSynthTrig2()));

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

  void slotReboot() {}
  void slotShutdown() {}

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG Streamer GUI Client Node",
                           "(c) 2007-2026 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   { QMutexLocker locker(&conf->mutex); conf->quitPending=true; }

   if (conf->strmSocket->state() != QAbstractSocket::UnconnectedState)
    conf->strmSocket->waitForDisconnected(1000); // timeout in ms
   //while (conf->strmSocket->state() != QAbstractSocket::UnconnectedState);

   for (auto& thread:conf->threads) { thread->wait(); delete thread; }
   
   QApplication::quit();
  }

  void slotToggleRecording() {
   QDateTime currentDT(QDateTime::currentDateTime());
   if (!conf->ctrlRecordingActive) {
    // Generate filename using current date and time
    // add current data and time to base: trial-20071228-123012-332.heg
    QString cntFN="trial-"+currentDT.toString("yyyyMMdd-hhmmss-zzz");
    conf->hegFile.setFileName(cntFN+".heg");
    if (!conf->hegFile.open(QIODevice::WriteOnly)) {
     qDebug() << "node_gui: <ToggleRecording> <ERROR> Cannot open .occ file for writing!";
     return;
    } conf->hegStream.setDevice(&conf->hegFile);

//    conf->hegStream << (int)(OCTOPUS_VER);		// Version
    conf->hegStream << conf->ampCount << "\n";		// Amplifier Count
    conf->hegStream << conf->sampleRate << "\n";	// Sample rate
    conf->hegStream << conf->chns.size() << "\n";	// Sample rate
//    conf->hegStream << cntRecChns.size();	// Channel count

    for (int i=0;i<conf->chns.size();i++) // Channel names - Cstyle
     conf->hegStream << conf->chns[i].chnName << " "; //.toLatin1().data();
    conf->hegStream << "\n";	// Sample rate

    conf->timeLabel->setText("Rec.Time: 00:00:00");
    conf->recCounter=0; conf->ctrlRecordingActive=true;
   } else { conf->ctrlRecordingActive=false;
    //if (stimulation) slotToggleStimulation();
    conf->hegStream.setDevice(0); conf->hegFile.close();
   }
  }

  void slotManualSync() { conf->commandToDaemon(CMD_ACQD_AMPSYNC); }
  void slotManualTrig() { conf->commandToDaemon(CMD_ACQD_S_TRIG_1000); }
  void slotSynthTrig1() { conf->commandToDaemon(CMD_ACQD_S_TRIG_1); }
  void slotSynthTrig2() { conf->commandToDaemon(CMD_ACQD_S_TRIG_2); }

//  void slotToggleNotch() {
//   {
//    QMutexLocker locker(&conf->mutex);
//    conf->ctrlNotchActive ? conf->ctrlNotchActive=false : conf->ctrlNotchActive=true;
//   }
//  }

  void slotScrollSpeed(int x) { conf->eegSweepDivider=conf->eegSweepCoeff[x]; }
  void slotEEGBand(int x) { conf->eegBand=x; }

 private:
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QStatusBar *ctrlStatusBar; QMenuBar *menuBar;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction;
  QPushButton *manualSyncButton,*manualTrigButton,*synthTrig1Button,*synthTrig2Button;
  QPushButton *toggleRecordingButton;
  QButtonGroup *eegBandBG,*scrSpeedBG; QVector<QPushButton*> cntSpeedButtons;
};
