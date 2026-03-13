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
   rebootAction->setStatusTip("Reboot node-acq via hwd server");
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

   // *** WIDGETS ***

   toggleRecordingButton=new QPushButton("RECORD",cntWidget);
   toggleRecordingButton->setGeometry(2,mainTabWidget->height()-54,80,20);
   toggleRecordingButton->setCheckable(true);
   connect(toggleRecordingButton,SIGNAL(clicked()),this,SLOT(slotToggleRecording()));

   QPushButton *dummyButton;

   // WAVPLAY BUTTONS
   wavPlayBG=new QButtonGroup(); wavPlayBG->setExclusive(true);
   for (int wavIdx=0;wavIdx<5;wavIdx++) { // WAV#
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(110+wavIdx*22,mainTabWidget->height()-54,20,20);
    wavPlayBG->addButton(dummyButton,wavIdx);
   }
   wavPlayBG->button(0)->setText("S");
   wavPlayBG->button(1)->setText("1");
   wavPlayBG->button(2)->setText("2");
   wavPlayBG->button(3)->setText("3");
   wavPlayBG->button(4)->setText("4"); wavPlayBG->button(0)->setChecked(true);
   connect(wavPlayBG,SIGNAL(buttonClicked(int)),this,SLOT(slotWavPlay(int)));

   // OPERATOR EVENT BUTTONS
   opEvtBG=new QButtonGroup(); opEvtBG->setExclusive(false);
   for (int evtIdx=0;evtIdx<5;evtIdx++) { // WAV#
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(false);
    dummyButton->setGeometry(250+evtIdx*28,mainTabWidget->height()-54,26,20);
    opEvtBG->addButton(dummyButton,evtIdx);
   }
   opEvtBG->button(0)->setText("O1");
   opEvtBG->button(1)->setText("O2");
   opEvtBG->button(2)->setText("O3");
   opEvtBG->button(3)->setText("O4");
   opEvtBG->button(4)->setText("O5");
   connect(opEvtBG,SIGNAL(buttonClicked(int)),this,SLOT(slotOpEvt(int)));

   syncButton=new QPushButton("SYNC",cntWidget);
   syncButton->setGeometry(410,mainTabWidget->height()-54,60,20);
   connect(syncButton,SIGNAL(clicked()),this,SLOT(slotSync()));

#ifdef EEGBANDSCOMP
   eegBandBG=new QButtonGroup(); eegBandBG->setExclusive(true);
   for (int bandIdx=0;bandIdx<6;bandIdx++) { // EEG frequency band
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(500+bandIdx*40,mainTabWidget->height()-54,38,20);
    eegBandBG->addButton(dummyButton,bandIdx);
   }
   eegBandBG->button(0)->setText("EEG");
   eegBandBG->button(1)->setText("Del");
   eegBandBG->button(2)->setText("The");
   eegBandBG->button(3)->setText("Alp");
   eegBandBG->button(4)->setText("Bet");
   eegBandBG->button(5)->setText("Gam"); eegBandBG->button(0)->setChecked(true);
   connect(eegBandBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGBand(int)));
#endif

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   scrSpeedBG=new QButtonGroup(); scrSpeedBG->setExclusive(true);
   for (int speedIdx=0;speedIdx<5;speedIdx++) { // EEG Scroll speed/resolution
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-220+speedIdx*42,mainTabWidget->height()-54,40,20);
    scrSpeedBG->addButton(dummyButton,speedIdx);
   }
   scrSpeedBG->button(0)->setText("x1");
   scrSpeedBG->button(1)->setText("x2");
   scrSpeedBG->button(2)->setText("x4");
   scrSpeedBG->button(3)->setText("x5");
   scrSpeedBG->button(4)->setText("x10"); scrSpeedBG->button(0)->setChecked(true);
   connect(scrSpeedBG,SIGNAL(buttonClicked(int)),this,SLOT(slotScrollSpeed(int)));

   setWindowTitle("Octopus HyperEEG/ERP Streaming/GL Client");
  }

  ~ControlWindow() override {}

  ConfParam *conf;

 signals: void streamData();

 private slots:

  // *** MENUS ***
  //
  void slotReboot() { if (!confirmAndProceed("Reboot")) return;
   conf->commandToDaemon(conf->wavPlayCommSocket,CMD_REBOOT);
   conf->commandToDaemon(conf->storCommSocket,CMD_REBOOT);
   conf->commandToDaemon(conf->acqPPCommSocket,CMD_REBOOT);
   conf->commandToDaemon(conf->acqCommSocket,CMD_REBOOT);
   slotQuit();
  }

  void slotShutdown() { if (!confirmAndProceed("Shutdown")) return;
   conf->commandToDaemon(conf->wavPlayCommSocket,CMD_SHUTDOWN);
   conf->commandToDaemon(conf->storCommSocket,CMD_SHUTDOWN);
   conf->commandToDaemon(conf->acqPPCommSocket,CMD_SHUTDOWN);
   conf->commandToDaemon(conf->acqCommSocket,CMD_SHUTDOWN);
   slotQuit();
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG Streamer GUI Client Node",
                           "(c) 2007-2026 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   conf->requestQuit();
   for (auto *t:conf->threads) { if (t) t->wait(); }
   if (conf->acqPPStrmSocket->state()!=QAbstractSocket::UnconnectedState)
    conf->acqPPStrmSocket->waitForDisconnected(1000); // timeout in ms
   QApplication::quit();
  }

  void slotToggleRecording() {
   if (!conf->ctrlRecordingActive) {
    conf->commandToDaemon(conf->storCommSocket,CMD_STOR_REC_ON);
    conf->ctrlRecordingActive=true;
   } else {
    conf->commandToDaemon(conf->storCommSocket,CMD_STOR_REC_OFF);
    conf->ctrlRecordingActive=false;
   }
  }

  void slotSync() { conf->commandToDaemon(conf->acqPPCommSocket,CMD_ACQ_AMPSYNC); }

  void slotWavPlay(int x) {
   conf->currentWav=x;
   if (x) {
    conf->commandToDaemon(conf->wavPlayCommSocket,CMD_WAVPLAY_PLAY+"="+QString::number(x)+"x.wav");
    qInfo() << "Requested WAV for playing (CMD_WAVPLAY_PLAY="+QString::number(x)+"x.wav).";
   } else {
    conf->commandToDaemon(conf->wavPlayCommSocket,CMD_WAVPLAY_STOP);
    qInfo() << "Requested WAV to stop (CMD_WAVPLAY_STOP).";
   }
  }

  void slotOpEvt(int x) {
   conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_OPEVT+"="+QString::number(x+1));
  }

  void slotScrollSpeed(int x) { conf->eegSweepSpeedIdx=x; }
#ifdef EEGBANDSCOMP
  void slotEEGBand(int x) { conf->eegBand=x; }
#endif

 private:
  bool confirmAndProceed(const QString &actionWord) {
   if (conf->ctrlRecordingActive) {
    QMessageBox::critical(this,"Operation blocked","Recording is currently active.\n\n"+actionWord+" is aborted.");
    return false;
   }
   // Add more blockers here later, for example:
   // if (conf->wavPlayActive) { ... return false; }
   // if (...) { ... return false; }

   // Confirmation
   QMessageBox msgBox(this);
   msgBox.setIcon(QMessageBox::Warning);
   msgBox.setWindowTitle(actionWord);
   msgBox.setText(actionWord+" all server nodes?");
   msgBox.setInformativeText("Normally it is supposed to be after today's scheduled experiments are complete.");
   QPushButton *yesBtn=msgBox.addButton(actionWord,QMessageBox::AcceptRole);
   msgBox.addButton(QMessageBox::Cancel);
   msgBox.setDefaultButton(QMessageBox::Cancel);
   msgBox.exec();
   return (msgBox.clickedButton()==yesBtn);
  }

  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QStatusBar *ctrlStatusBar; QMenuBar *menuBar;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction;
  QPushButton *syncButton;
  QPushButton *toggleRecordingButton;
#ifdef EEGBANDSCOMP
  QButtonGroup *eegBandBG;
#endif
  QButtonGroup *wavPlayBG,*opEvtBG,*scrSpeedBG; QVector<QPushButton*> cntSpeedButtons;
};
