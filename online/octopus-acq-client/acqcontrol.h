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

#ifndef ACQCONTROL_H
#define ACQCONTROL_H

#include <QtGui>
#include <QMainWindow>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

#include "acqmaster.h"

class AcqControl : public QMainWindow {
 Q_OBJECT
 public:
  AcqControl(AcqMaster *acqm,QWidget *parent=0) : QMainWindow(parent) {
   acqM=acqm; setGeometry(acqM->ctrlGuiX,acqM->ctrlGuiY,acqM->ctrlGuiW,acqM->ctrlGuiH);
   setFixedSize(acqM->ctrlGuiW,acqM->ctrlGuiH);

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,width()-2,height()-60);
   cntWidget=new QWidget(mainTabWidget);
   cntWidget->setGeometry(0,0,mainTabWidget->width(),mainTabWidget->height());
   cntWidget->show();
   mainTabWidget->addTab(cntWidget,"EEG"); mainTabWidget->show();

   // *** HEAD & CONFIG WINDOW ***

   // *** STATUSBAR ***

   acqM->guiStatusBar=new QStatusBar(this);
   acqM->guiStatusBar->setGeometry(0,height()-20,width(),20);
   acqM->guiStatusBar->show(); setStatusBar(acqM->guiStatusBar);

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *sysMenu=new QMenu("&System",menuBar);
   QMenu *testMenu=new QMenu("&Test",menuBar);
   QMenu *paraMenu=new QMenu("&Paradigm",menuBar);
   menuBar->addMenu(sysMenu); menuBar->addMenu(testMenu);
   menuBar->addMenu(paraMenu); setMenuBar(menuBar);

   // System Menu
   toggleCalAction=new QAction("&Start calibration",this);
   rebootAction=new QAction("&Reboot Servers",this);
   shutdownAction=new QAction("&Shutdown Servers",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   toggleCalAction->setStatusTip(
    "Start amplifier calibration procedure (Lasts approximately 45 minutes).");
   rebootAction->setStatusTip("Reboot the ACQuisition and STIM servers");
   shutdownAction->setStatusTip("Shutdown the ACQuisition and STIM servers");
   aboutAction->setStatusTip("About Octopus GUI..");
   quitAction->setStatusTip("Quit Octopus GUI");
   connect(toggleCalAction,SIGNAL(triggered()),
           this,SLOT(slotToggleCalibration()));
   connect(rebootAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),(QObject *)acqM,SLOT(slotQuit()));
   sysMenu->addAction(toggleCalAction); sysMenu->addSeparator();
   sysMenu->addAction(rebootAction);
   sysMenu->addAction(shutdownAction); sysMenu->addSeparator();
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // Test Menu
   testSCAction=new QAction("Sine-Cosine Test",this);
   testSquareAction=new QAction("Square-wave Test",this);
   testSCAction->setStatusTip("Sine-Cosine dual-event Jitter Test");
   testSquareAction->setStatusTip("SquareWave single-Event Jitter Test");
   connect(testSCAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotTestSineCosine()));
   connect(testSquareAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotTestSquare()));
   testMenu->addAction(testSCAction);
   testMenu->addAction(testSquareAction);

   // Paradigm Menu
   paraLoadPatAction=new QAction("&Load STIM Pattern..",this);
   paraClickAction=new QAction("1ms Click",this);
   paraSquareBurstAction=new QAction("50ms Burst",this);
   paraIIDITDAction=new QAction("IID-ITD (PU)",this);
   paraIIDITD_ML_Action=new QAction("IID-ITD Mono-L",this);
   paraIIDITD_MR_Action=new QAction("IID-ITD Mono-R",this);
   paraLoadPatAction->setStatusTip(
    "Load precalculated STIMulus pattern..");
   paraClickAction->setStatusTip("Click of 1ms duration. SOA=1000ms");
   paraSquareBurstAction->setStatusTip(
    "SquareWave burst of 50ms duration, with 500us period, and SOA=1000ms");
   paraIIDITDAction->setStatusTip(
    "Dr. Ungan's specialized paradigm for Interaural Intensity vs.Time Delay");
   paraIIDITD_ML_Action->setStatusTip(
    "Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Left)");
   paraIIDITD_MR_Action->setStatusTip(
    "Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Right)");
   connect(paraLoadPatAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmLoadPattern()));
   connect(paraClickAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmClick()));
   connect(paraSquareBurstAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmSquareBurst()));
   connect(paraIIDITDAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmIIDITD()));
   connect(paraIIDITD_ML_Action,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmIIDITD_MonoL()));
   connect(paraIIDITD_MR_Action,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotParadigmIIDITD_MonoR()));
   paraMenu->addAction(paraLoadPatAction); paraMenu->addSeparator();
   paraMenu->addAction(paraClickAction);
   paraMenu->addAction(paraSquareBurstAction); paraMenu->addSeparator();
   paraMenu->addAction(paraIIDITDAction);
   paraMenu->addAction(paraIIDITD_ML_Action);
   paraMenu->addAction(paraIIDITD_MR_Action);


   // *** BUTTONS AT THE TOP ***

   toggleRecordingButton=new QPushButton("RECORD",cntWidget);
   toggleRecordingButton->setGeometry(2,mainTabWidget->height()-54,100,20);
   toggleRecordingButton->setCheckable(true);
   connect(toggleRecordingButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleRecording()));

   acqM->timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   acqM->timeLabel->setGeometry(110,mainTabWidget->height()-52,190,20);

   toggleStimulationButton=new QPushButton("STIM",cntWidget);
   toggleStimulationButton->setGeometry(310,mainTabWidget->height()-54,60,20);
   toggleStimulationButton->setCheckable(true);
   connect(toggleStimulationButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleStimulation()));

   toggleTriggerButton=new QPushButton("TRIG",cntWidget);
   toggleTriggerButton->setGeometry(374,mainTabWidget->height()-54,60,20);
   toggleTriggerButton->setCheckable(true);
   connect(toggleTriggerButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleTrigger()));

   manualSyncButton=new QPushButton("SYNC",cntWidget);
   manualSyncButton->setGeometry(460,mainTabWidget->height()-54,60,20);
   connect(manualSyncButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotManualSync()));

   manualTrigButton=new QPushButton("PING",cntWidget);
   manualTrigButton->setGeometry(550,mainTabWidget->height()-54,60,20);
   connect(manualTrigButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotManualTrig()));

   toggleNotchButton=new QPushButton("MABPF",cntWidget);
   toggleNotchButton->setGeometry(650,mainTabWidget->height()-54,60,20);
   toggleNotchButton->setCheckable(true); toggleNotchButton->setChecked(true);
   connect(toggleNotchButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleNotch()));

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   acqM->cntSpeedX=4;

   QPushButton *dummyButton;
   cntSpdBG=new QButtonGroup();
   cntSpdBG->setExclusive(true);
   for (int i=0;i<5;i++) { // EEG SPEED
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-310+i*60,
                             mainTabWidget->height()-54,60,20);
    cntSpdBG->addButton(dummyButton,i); }
   cntSpdBG->button(0)->setText("4s");
   cntSpdBG->button(1)->setText("2s");
   cntSpdBG->button(2)->setText("800ms");
   cntSpdBG->button(3)->setText("400ms");
   cntSpdBG->button(4)->setText("200ms");
   cntSpdBG->button(2)->setDown(true);

   connect(cntSpdBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntSpeed(int)));

   setWindowTitle("Octopus HyperEEG/ERP Streaming/GL Client");
  }

 signals: void scrollData();

 private slots:

  // *** MENUS ***

  void slotToggleCalibration() {
   if (acqM->calibration) { acqM->stopCalibration();
    toggleCalAction->setText("&Start calibration");
   } else { acqM->startCalibration();
    toggleCalAction->setText("&Stop calibration");
   }
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG/ERP Client",
                           "EEG/ERP Dashboard (VisRec GUI)\n"
                           "(c) 2007-2025 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to extend/redistribute it under conditions of GPL v3.\n");
  }


  // *** SPEED OF THE VISUALS ***

  void slotCntSpeed(int x) {
   switch (x) {
    case 0: acqM->cntSpeedX=10; cntSpdBG->button(2)->setDown(false); break;
    case 1: acqM->cntSpeedX= 8; cntSpdBG->button(2)->setDown(false); break;
    case 2: acqM->cntSpeedX= 4; break;
    case 3: acqM->cntSpeedX= 2; cntSpdBG->button(2)->setDown(false); break;
    case 4: acqM->cntSpeedX= 1; cntSpdBG->button(2)->setDown(false); break;
   }
  }

 private:
  AcqMaster *acqM; //CntFrame *cntFrame; unsigned int ampNo; HeadWindow *headWin;
  QMenuBar *menuBar;
  QAction *toggleCalAction,*rebootAction,*shutdownAction,
          *quitAction,*aboutAction,*testSCAction,*testSquareAction,
          *paraLoadPatAction,*paraClickAction,*paraSquareBurstAction,
          *paraIIDITDAction,*paraIIDITD_ML_Action,*paraIIDITD_MR_Action;
  QTabWidget *mainTabWidget; QWidget *cntWidget; QPushButton *manualSyncButton,*manualTrigButton;
  QPushButton *toggleRecordingButton,*toggleStimulationButton,
              *toggleTriggerButton,*toggleNotchButton;
  QButtonGroup *cntSpdBG;
  QVector<QPushButton*> cntSpeedButtons;
};

#endif
