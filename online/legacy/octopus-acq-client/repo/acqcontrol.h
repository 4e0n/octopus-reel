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
   connect(rebootAction,SIGNAL(triggered()),(QObject *)acqM,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),(QObject *)acqM,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),(QObject *)acqM,SLOT(slotQuit()));
   sysMenu->addAction(rebootAction);
   sysMenu->addAction(shutdownAction); sysMenu->addSeparator();
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // *** BUTTONS AT THE TOP ***

   toggleRecordingButton=new QPushButton("RECORD",cntWidget);
   toggleRecordingButton->setGeometry(2,mainTabWidget->height()-54,100,20);
   toggleRecordingButton->setCheckable(true);
   connect(toggleRecordingButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotToggleRecording()));

   acqM->timeLabel=new QLabel("Rec.Time: 00:00:00",cntWidget);
   acqM->timeLabel->setGeometry(110,mainTabWidget->height()-52,190,20);

   manualSyncButton=new QPushButton("SYNC",cntWidget);
   manualSyncButton->setGeometry(460,mainTabWidget->height()-54,60,20);
   connect(manualSyncButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotManualSync()));

   manualTrigButton=new QPushButton("PING",cntWidget);
   manualTrigButton->setGeometry(550,mainTabWidget->height()-54,60,20);
   connect(manualTrigButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotManualTrig()));

   toggleNotchButton=new QPushButton("MABPF",cntWidget);
   toggleNotchButton->setGeometry(650,mainTabWidget->height()-54,60,20);
   toggleNotchButton->setCheckable(true); toggleNotchButton->setChecked(true);
   connect(toggleNotchButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotToggleNotch()));

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   acqM->cntSpeedX=4;

   QPushButton *dummyButton;
   cntSpdBG=new QButtonGroup();
   cntSpdBG->setExclusive(true);
   for (int i=0;i<5;i++) { // EEG SPEED
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(mainTabWidget->width()-310+i*60,mainTabWidget->height()-54,60,20);
    cntSpdBG->addButton(dummyButton,i);
   }
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
  QAction *rebootAction,*shutdownAction,*quitAction,*aboutAction;
  QTabWidget *mainTabWidget; QWidget *cntWidget; QPushButton *manualSyncButton,*manualTrigButton;
  QPushButton *toggleRecordingButton,*toggleNotchButton;
  QButtonGroup *cntSpdBG;
  QVector<QPushButton*> cntSpeedButtons;
};

#endif
