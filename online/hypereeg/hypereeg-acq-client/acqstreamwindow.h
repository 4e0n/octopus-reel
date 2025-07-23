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

#ifndef ACQSTREAMWINDOW_H
#define ACQSTREAMWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QMenu>
#include <QMenuBar>
#include <QSlider>
#include <QCheckBox>
#include <QListWidget>
#include "confparam.h"
#include "eegframe.h"

class AcqStreamWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit AcqStreamWindow(unsigned int a=0,ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
   ampNo=a; conf=c;
   setGeometry(ampNo*conf->guiStrmW+conf->guiStrmX,conf->guiStrmY,conf->guiStrmW,conf->guiStrmH);
   setFixedSize(conf->guiStrmW,conf->guiStrmH);

   conf->eegFrameH=conf->guiStrmH-90; conf->eegFrameW=(int)(.66*(float)(conf->guiStrmW));
   if (conf->eegFrameW%2==1) conf->eegFrameW--;
   conf->glFrameW=(int)(.33*(float)(conf->guiStrmW));
   if (conf->glFrameW%2==1) { conf->glFrameW--; } conf->glFrameH=conf->glFrameW;

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,this->width()-2,this->height());

   eegWidget=new QWidget(mainTabWidget);
   eegFrame=new EEGFrame(conf,ampNo,eegWidget);
   eegFrame->setGeometry(2,2,conf->eegFrameW,conf->eegFrameH); eegWidget->show(); 
 
   mainTabWidget->addTab(eegWidget,"EEG/ERP"); mainTabWidget->show();

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   ampBG=new QButtonGroup(); ampBG->setExclusive(true);
   for (int btnIdx=0;btnIdx<6;btnIdx++) { // EEG Amplification
    dummyButton=new QPushButton(eegWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(100+btnIdx*60,eegWidget->height(),60,20);
    ampBG->addButton(dummyButton,btnIdx);
   }
   ampBG->button(0)->setText("1mV");   ampBG->button(1)->setText("500uV");
   ampBG->button(2)->setText("200uV"); ampBG->button(3)->setText("100uV");
   ampBG->button(4)->setText("50uV");  ampBG->button(5)->setText("20uV");
   ampBG->button(3)->setDown(true);
   conf->ampX[ampNo]=(1000000.0/100.0);
   connect(ampBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGAmp(int)));

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *modelMenu=new QMenu("&Model",menuBar); QMenu *viewMenu=new QMenu("&View",menuBar);
   menuBar->addMenu(modelMenu); menuBar->addMenu(viewMenu);
   setMenuBar(menuBar);

   setWindowTitle("Octopus Hyper EEG/ERP Amp #"+QString::number(ampNo+1));
  }

  ~AcqStreamWindow() override {}

  ConfParam *conf;
  
  unsigned int ampNo;
 
 public slots:
  void slotEEGAmp(int x) { conf->ampX[ampNo]=conf->ampRange[x]; }

 private:
  QString dummyString; QPushButton *dummyButton;
  EEGFrame *eegFrame;
  QMenuBar *menuBar; QTabWidget *mainTabWidget; QWidget *eegWidget;
  QButtonGroup *ampBG;
  QVector<QPushButton*> ampButtons;
};

#endif
