/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_RECORDER_H
#define OCTOPUS_RECORDER_H

#include <QtGui>

#include "octopus_rec_master.h"
#include "headwindow.h"
#include "cntframe.h"

class Recorder : public QMainWindow {
 Q_OBJECT
 public:
  Recorder(RecMaster *rm,QWidget *parent=0) : QMainWindow(parent) {
   p=rm; setGeometry(p->guiX,p->guiY,p->guiWidth,p->guiHeight);
   setFixedSize(p->guiWidth,p->guiHeight);


   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,width()-2,height()-60);

   cntWidget=new QWidget(mainTabWidget);
   cntWidget->setGeometry(0,0,mainTabWidget->width(),mainTabWidget->height());
   cntFrame=new CntFrame(cntWidget,p); cntWidget->show();
   mainTabWidget->addTab(cntWidget,"EEG"); mainTabWidget->show();


   // *** HEAD & CONFIG WINDOW ***

   headWin=new HeadWindow(this,p); headWin->show();


   // *** STATUSBAR ***

   p->guiStatusBar=new QStatusBar(this);
   p->guiStatusBar->setGeometry(0,height()-20,width(),20);
   p->guiStatusBar->show(); setStatusBar(p->guiStatusBar);

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
           (QObject *)p,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),(QObject *)p,SLOT(slotQuit()));
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
           (QObject *)p,SLOT(slotTestSineCosine()));
   connect(testSquareAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotTestSquare()));
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
           (QObject *)p,SLOT(slotParadigmLoadPattern()));
   connect(paraClickAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotParadigmClick()));
   connect(paraSquareBurstAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotParadigmSquareBurst()));
   connect(paraIIDITDAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotParadigmIIDITD()));
   connect(paraIIDITD_ML_Action,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotParadigmIIDITD_MonoL()));
   connect(paraIIDITD_MR_Action,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotParadigmIIDITD_MonoR()));
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

   toggleRecordingButton->setGeometry(20,mainTabWidget->height()-54,48,20);
   toggleStimulationButton->setGeometry(450,mainTabWidget->height()-54,48,20);
   toggleTriggerButton->setGeometry(505,mainTabWidget->height()-54,48,20);

   toggleRecordingButton->setCheckable(true);
   toggleStimulationButton->setCheckable(true);
   toggleTriggerButton->setCheckable(true);

   connect(toggleRecordingButton,SIGNAL(clicked()),
           (QObject *)p,SLOT(slotToggleRecording()));
   connect(toggleStimulationButton,SIGNAL(clicked()),
           (QObject *)p,SLOT(slotToggleStimulation()));
   connect(toggleTriggerButton,SIGNAL(clicked()),
           (QObject *)p,SLOT(slotToggleTrigger()));

   p->timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   p->timeLabel->setGeometry(200,mainTabWidget->height()-52,150,20);

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

   cntAmpBG->button(0)->setText("5uV");   cntAmpBG->button(1)->setText("10uV");
   cntAmpBG->button(2)->setText("25uV");  cntAmpBG->button(3)->setText("50uV");
   cntAmpBG->button(4)->setText("100uV"); cntAmpBG->button(5)->setText("200uV");
   cntAmpBG->button(5)->setDown(true);
   cntSpdBG->button(0)->setText("4s");    cntSpdBG->button(1)->setText("2s");
   cntSpdBG->button(2)->setText("800ms"); cntSpdBG->button(3)->setText("400ms");
   cntSpdBG->button(4)->setText("200ms"); cntSpdBG->button(2)->setDown(true);

   p->cntAmpX=(200.0/200.0);
   connect(cntAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntAmp(int)));
   connect(cntSpdBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntSpeed(int)));

   setWindowTitle("Octopus EEG/ERP Recorder v0.9.5");
  }

 signals: void scrollData();

 private slots:

  // *** MENUS ***

  void slotToggleCalibration() {
   if (p->calibration) { p->stopCalibration();
    toggleCalAction->setText("&Start calibration");
   } else { p->startCalibration();
    toggleCalAction->setText("&Stop calibration");
   }
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus Project",
                           "Octopus EEG/ERP Recorder v0.9.5");
  }


  // *** AMPLITUDE & SPEED OF THE VISUALS ***

  void slotCntAmp(int x) {
   switch (x) {
    case 0: p->cntAmpX=(8000.0/200.0);cntAmpBG->button(5)->setDown(false);break;
    case 1: p->cntAmpX=(4000.0/200.0);cntAmpBG->button(5)->setDown(false);break;
    case 2: p->cntAmpX=(1600.0/200.0);cntAmpBG->button(5)->setDown(false);break;
    case 3: p->cntAmpX=( 800.0/200.0);cntAmpBG->button(5)->setDown(false);break;
    case 4: p->cntAmpX=( 400.0/200.0);cntAmpBG->button(5)->setDown(false);break;
    case 5: p->cntAmpX=( 200.0/200.0);break; // 200uV
   }
  }

  void slotCntSpeed(int x) {
   switch (x) {
    case 0: p->cntSpeedX=20; cntSpdBG->button(2)->setDown(false); break;
    case 1: p->cntSpeedX=10; cntSpdBG->button(2)->setDown(false); break;
    case 2: p->cntSpeedX= 4; break;
    case 3: p->cntSpeedX= 2; cntSpdBG->button(2)->setDown(false); break;
    case 4: p->cntSpeedX= 1; cntSpdBG->button(2)->setDown(false); break;
   }
  }

 private:
  RecMaster *p; CntFrame *cntFrame; HeadWindow *headWin; QMenuBar *menuBar;
  QAction *toggleCalAction,*rebootAction,*shutdownAction,
          *quitAction,*aboutAction,*testSCAction,*testSquareAction,
          *paraLoadPatAction,*paraClickAction,*paraSquareBurstAction,
          *paraIIDITDAction,*paraIIDITD_ML_Action,*paraIIDITD_MR_Action;
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QPushButton *toggleRecordingButton,*toggleStimulationButton,
              *toggleTriggerButton;
  QButtonGroup *cntAmpBG,*cntSpdBG;
  QVector<QPushButton*> cntAmpButtons,cntSpeedButtons;
};

#endif
