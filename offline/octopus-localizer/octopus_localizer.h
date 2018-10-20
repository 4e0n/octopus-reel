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

#ifndef OCTOPUS_LOCALIZER_H
#define OCTOPUS_LOCALIZER_H

#include <QtGui>

#include "octopus_loc_master.h"
#include "headwindow.h"
#include "cntframe.h"

class Localizer : public QMainWindow {
 Q_OBJECT
 public:
  Localizer(LocMaster *lm,QWidget *parent=0) : QMainWindow(parent) {
   p=lm; setGeometry(p->guiX,p->guiY,p->guiWidth,p->guiHeight);
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
   menuBar->addMenu(sysMenu); menuBar->addMenu(testMenu);
   setMenuBar(menuBar);

   // System Menu
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   aboutAction->setStatusTip("About Octopus-ReEL..");
   quitAction->setStatusTip("Quit Octopus-ReEL");
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),(QObject *)p,SLOT(slotQuit()));
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // Test Menu
   testSCAction=new QAction("Sine-Cosine Test",this);
   testSquareAction=new QAction("Square-wave Test",this);
   connect(testSCAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotTestSineCosine()));
   connect(testSquareAction,SIGNAL(triggered()),
           (QObject *)p,SLOT(slotTestSquare()));
   testMenu->addAction(testSCAction);
   testMenu->addAction(testSquareAction);

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

   setWindowTitle("Octopus EEG/ERP Localizer v0.9.5");
  }

 private slots:

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL",
                           "Octopus EEG/ERP Source Localization Tool\n"
                           "(c) 2007 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                           "to redistribute it under conditions of GPL v3.\n");
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
  LocMaster *p; CntFrame *cntFrame; HeadWindow *headWin; QMenuBar *menuBar;
  QAction *toggleCalAction,*rebootAction,*shutdownAction,
          *quitAction,*aboutAction,*testSCAction,*testSquareAction;
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QButtonGroup *cntAmpBG,*cntSpdBG;
  QVector<QPushButton*> cntAmpButtons,cntSpeedButtons;
};

#endif
