/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef OCTOPUS_ACQ_CLIENT_H
#define OCTOPUS_ACQ_CLIENT_H

#include <QtGui>

#include "octopus_acq_master.h"
#include "headwindow.h"
#include "cntframe.h"

class AcqClient : public QMainWindow {
 Q_OBJECT
 public:
  AcqClient(AcqMaster *acqm,QWidget *parent=0) : QMainWindow(parent) {
   acqM=acqm; setGeometry(acqM->guiX,acqM->guiY,acqM->guiWidth,acqM->guiHeight);
   setFixedSize(acqM->guiWidth,acqM->guiHeight);

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,width()-2,height()-60);

   cntWidget=new QWidget(mainTabWidget);
   cntWidget->setGeometry(0,0,mainTabWidget->width(),mainTabWidget->height());
   cntFrame=new CntFrame(cntWidget,acqM); cntWidget->show();
   mainTabWidget->addTab(cntWidget,"EEG"); mainTabWidget->show();


   // *** HEAD & CONFIG WINDOW ***

   headWin=new HeadWindow(this,acqM); headWin->show();


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

   toggleRecordingButton=new QPushButton("REC",cntWidget);
   toggleStimulationButton=new QPushButton("STIM",cntWidget);
   toggleTriggerButton=new QPushButton("TRIG",cntWidget);
   toggleNotchButton=new QPushButton("NOTCH FILTER",cntWidget);

   toggleRecordingButton->setGeometry(20,mainTabWidget->height()-54,48,20);
   toggleStimulationButton->setGeometry(450,mainTabWidget->height()-54,48,20);
   toggleTriggerButton->setGeometry(505,mainTabWidget->height()-54,48,20);
   toggleNotchButton->setGeometry(570,mainTabWidget->height()-54,88,20);

   toggleRecordingButton->setCheckable(true);
   toggleStimulationButton->setCheckable(true);
   toggleTriggerButton->setCheckable(true);
   toggleNotchButton->setCheckable(true);
   toggleNotchButton->setChecked(true);

   connect(toggleRecordingButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleRecording()));
   connect(toggleStimulationButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleStimulation()));
   connect(toggleTriggerButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleTrigger()));
   connect(toggleNotchButton,SIGNAL(clicked()),
           (QObject *)acqM,SLOT(slotToggleNotch()));

   acqM->timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   acqM->timeLabel->setGeometry(200,mainTabWidget->height()-52,150,20);

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   QPushButton *dummyButton;
   cntAmpBG=new QButtonGroup(); cntSpdBG=new QButtonGroup();
   cntAmpBG->setExclusive(true); cntSpdBG->setExclusive(true);
   for (int i=0;i<6;i++) { // EEG AMP
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-676+i*60,
                             mainTabWidget->height()-54,60,20);
    cntAmpBG->addButton(dummyButton,i); }
   for (int i=0;i<5;i++) { // EEG SPEED
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-326+i*60,
                             mainTabWidget->height()-54,60,20);
    cntSpdBG->addButton(dummyButton,i); }

   cntAmpBG->button(0)->setText("1mV");
   cntAmpBG->button(1)->setText("500uV");
   cntAmpBG->button(2)->setText("200uV");
   cntAmpBG->button(3)->setText("100uv");
   cntAmpBG->button(4)->setText("50uV");
   cntAmpBG->button(5)->setText("20uV");
   cntAmpBG->button(3)->setDown(true);

   cntSpdBG->button(0)->setText("4s");
   cntSpdBG->button(1)->setText("2s");
   cntSpdBG->button(2)->setText("800ms");
   cntSpdBG->button(3)->setText("400ms");
   cntSpdBG->button(4)->setText("200ms");
   cntSpdBG->button(2)->setDown(true);

   acqM->cntAmpX=(1000000.0/100.0);
   connect(cntAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntAmp(int)));
   connect(cntSpdBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntSpeed(int)));

   setWindowTitle("Octopus EEG/ERP GUI Client v1.1.0");
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
   QMessageBox::about(this,"About Octopus-ReEL",
                           "EEG/ERP Dashboard (visual&rec. GUI)\n"
                           "(c) 2007-2025 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to redistribute it under conditions of GPL v3.\n");
  }


  // *** AMPLITUDE & SPEED OF THE VISUALS ***

  void slotCntAmp(int x) {
   switch (x) {
    case 0: acqM->cntAmpX=(1000000.0/1000.0);cntAmpBG->button(3)->setDown(false);break;
    case 1: acqM->cntAmpX=(1000000.0/500.0);cntAmpBG->button(3)->setDown(false);break;
    case 2: acqM->cntAmpX=(1000000.0/200.0);cntAmpBG->button(3)->setDown(false);break;
    case 3: acqM->cntAmpX=(1000000.0/100.0);break;
    case 4: acqM->cntAmpX=(1000000.0/50.0);cntAmpBG->button(3)->setDown(false);break;
    case 5: acqM->cntAmpX=(1000000.0/20.0);cntAmpBG->button(3)->setDown(false);break;
   }
  }

  void slotCntSpeed(int x) {
   switch (x) {
    case 0: acqM->cntSpeedX=10; cntSpdBG->button(2)->setDown(false); break;
    case 1: acqM->cntSpeedX= 8; cntSpdBG->button(2)->setDown(false); break;
    case 2: acqM->cntSpeedX= 4; break;
    case 3: acqM->cntSpeedX= 2; cntSpdBG->button(2)->setDown(false); break;
    case 4: acqM->cntSpeedX= 1; cntSpdBG->button(2)->setDown(false); break;
   }
  }

 protected:
  void resizeEvent(QResizeEvent *event) {
   int w,h;
   //resizeEvent(event); // Call base class event handler

   //if (acqM->clientRunning) {
   acqM->guiWidth=w=event->size().width();
   acqM->guiHeight=h=event->size().height();

   // Resize child widgets proportionally
   cntFrame->setGeometry(2,2,w-9,h-60);
   mainTabWidget->setGeometry(1,32,w-2,h-60);
   cntWidget->setGeometry(0,0,w,h);
   acqM->guiStatusBar->setGeometry(0,h-20,w,20);
   acqM->timeLabel->setGeometry(acqM->timeLabel->x(),mainTabWidget->height()-52,
                                acqM->timeLabel->width(),acqM->timeLabel->height());
   menuBar->setFixedWidth(w);
   toggleRecordingButton->setGeometry(toggleRecordingButton->x(),mainTabWidget->height()-54,
                                      toggleRecordingButton->width(),toggleRecordingButton->height());
   toggleStimulationButton->setGeometry(toggleStimulationButton->x(),mainTabWidget->height()-54,
                                        toggleStimulationButton->width(),toggleStimulationButton->height());
   toggleTriggerButton->setGeometry(toggleTriggerButton->x(),mainTabWidget->height()-54,
                                    toggleTriggerButton->width(),toggleTriggerButton->height());
   toggleNotchButton->setGeometry(toggleNotchButton->x(),mainTabWidget->height()-54,
                                  toggleNotchButton->width(),toggleNotchButton->height());
   for (int i=0;i<6;i++) cntAmpBG->button(i)->setGeometry(mainTabWidget->width()-676+i*60,mainTabWidget->height()-54,
                                                          cntAmpBG->button(i)->width(),cntAmpBG->button(i)->height());
   for (int i=0;i<5;i++) cntSpdBG->button(i)->setGeometry(mainTabWidget->width()-326+i*60,mainTabWidget->height()-54,
                                                          cntSpdBG->button(i)->width(),cntSpdBG->button(i)->height());
   //}
  }

 private:
  AcqMaster *acqM; CntFrame *cntFrame; HeadWindow *headWin; QMenuBar *menuBar;
  QAction *toggleCalAction,*rebootAction,*shutdownAction,
          *quitAction,*aboutAction,*testSCAction,*testSquareAction,
          *paraLoadPatAction,*paraClickAction,*paraSquareBurstAction,
          *paraIIDITDAction,*paraIIDITD_ML_Action,*paraIIDITD_MR_Action;
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QPushButton *toggleRecordingButton,*toggleStimulationButton,
              *toggleTriggerButton,*toggleNotchButton;
  QButtonGroup *cntAmpBG,*cntSpdBG;
  QVector<QPushButton*> cntAmpButtons,cntSpeedButtons;
};

#endif
