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
#include "../common/tcp_commands.h"

class AmpWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit AmpWindow(unsigned int aNo=0,ConfParam *c=nullptr,QWidget *parent=nullptr):QMainWindow(parent) {
   ampNo=aNo; conf=c;
   setGeometry(ampNo*(conf->guiAmpW+10)+conf->guiAmpX,conf->guiAmpY,conf->guiAmpW,conf->guiAmpH);
   setFixedSize(conf->guiAmpW,conf->guiAmpH);

   // *** TABS & TABWIDGETS ***
   //
   int tabW=conf->guiAmpW; int tabH=conf->guiAmpH;
   if (tabW%2==1) tabW--;

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,tabW,tabH);
   tabH-=100; //mainTabWidget->height()

   // *** Channels & Interpolation
   chnWidget=new QWidget(mainTabWidget); chnWidget->setGeometry(2,2,tabW-4,tabH-4); 

   float w=(float)(chnWidget->width()); //float h=(float)(chnWidget->height());

   unsigned tXmax=0,tYmax=0;
   for (int chnIdx=0;chnIdx<conf->refChns.size();chnIdx++) {
    tXmax<conf->refChns[chnIdx].topoX ? tXmax=conf->refChns[chnIdx].topoX : tXmax;
    tYmax<conf->refChns[chnIdx].topoY ? tYmax=conf->refChns[chnIdx].topoY : tYmax;
   }
   for (int chnIdx=0;chnIdx<conf->bipChns.size();chnIdx++) {
    tXmax<conf->bipChns[chnIdx].topoX ? tXmax=conf->bipChns[chnIdx].topoX : tXmax;
    tYmax<conf->bipChns[chnIdx].topoY ? tYmax=conf->bipChns[chnIdx].topoY : tYmax;
   }
   for (int chnIdx=0;chnIdx<conf->metaChns.size();chnIdx++) {
    tXmax<conf->metaChns[chnIdx].topoX ? tXmax=conf->metaChns[chnIdx].topoX : tXmax;
    tYmax<conf->metaChns[chnIdx].topoY ? tYmax=conf->metaChns[chnIdx].topoY : tYmax;
   }

   float aspect=0.95f; float cellSize=w/(float)(tXmax); int buttonSize=(int)(aspect*cellSize);
   unsigned int sz=(unsigned int)(cellSize*aspect); // guiCellSize
   float cf=(float)(tXmax)/((float)(tXmax)+1.); // Aspect ratio multiplier
   QRect mRect(0,0,sz*cf,sz*cf); QRegion mRegion(mRect,QRegion::Ellipse);

   QString chnName; unsigned int chnIdx=0,topoX=0,topoY=0,yOffset=0;
   refChnBG=new QButtonGroup(); refChnBG->setExclusive(false);
   for (int btnIdx=0;btnIdx<conf->refChns.size();btnIdx++) {
    chnIdx=conf->refChns[btnIdx].physChn; chnName=conf->refChns[btnIdx].chnName;
    topoX=conf->refChns[btnIdx].topoX; topoY=conf->refChns[btnIdx].topoY;
    yOffset=(cellSize/2)*cf;
    if (topoY>1) yOffset+=cellSize/2;
    if (topoY>10) yOffset+=cellSize/2;
    QRect cr(cellSize/2-buttonSize/2+cellSize*(topoX-1),
             cellSize/2-buttonSize/2+cellSize*(topoY-1)+yOffset,buttonSize,buttonSize);
    dummyButton=new QPushButton(chnWidget); dummyButton->setCheckable(true);
    dummyButton->setText(QString::number(chnIdx)+"\n"+chnName);
    QFont f=dummyButton->font(); f.setPointSize(qMax(8,buttonSize/5)); // tune
    f.setBold(true); dummyButton->setFont(f);
    dummyButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    dummyButton->setFocusPolicy(Qt::NoFocus);
    dummyButton->setGeometry(cr); dummyButton->setMask(mRegion);
    dummyButton->setStyleSheet("QPushButton { background-color: white; color: black;"
                               " padding: 0px; margin: 0px; text-align: center; }"
                               "QPushButton:checked { background-color: yellow;"
                               " color: black; }");
    dummyButton->setChecked((bool)conf->refChns[btnIdx].interMode[ampNo]);
    refChnBG->addButton(dummyButton,btnIdx);
   }
   connect(refChnBG,SIGNAL(buttonClicked(int)),this,SLOT(slotRefChnInter(int)));

   bipChnBG=new QButtonGroup(); bipChnBG->setExclusive(false);
   for (int btnIdx=0;btnIdx<conf->bipChns.size();btnIdx++) {
    chnIdx=conf->bipChns[btnIdx].physChn; chnName=conf->bipChns[btnIdx].chnName;
    topoX=conf->bipChns[btnIdx].topoX; topoY=conf->bipChns[btnIdx].topoY;
    yOffset=(cellSize/2)*cf;
    if (topoY>1) yOffset+=cellSize/2;
    if (topoY>10) yOffset+=cellSize/2;
    QRect cr(cellSize/2-buttonSize/2+cellSize*(topoX-1),
             cellSize/2-buttonSize/2+cellSize*(topoY-1)+yOffset,buttonSize,buttonSize);
    dummyButton=new QPushButton(chnWidget); dummyButton->setCheckable(true);
    dummyButton->setText(QString::number(chnIdx)+"\n"+chnName);
    QFont f=dummyButton->font(); f.setPointSize(qMax(8,buttonSize/5)); // tune
    f.setBold(true); dummyButton->setFont(f);
    dummyButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    dummyButton->setFocusPolicy(Qt::NoFocus);
    dummyButton->setGeometry(cr); dummyButton->setMask(mRegion);
    dummyButton->setStyleSheet("QPushButton { background-color: white; color: black;"
                               " padding: 0px; margin: 0px; text-align: center; }"
                               "QPushButton:checked { background-color: yellow;"
                               " color: black; }");
    dummyButton->setChecked((bool)conf->bipChns[btnIdx].interMode[ampNo]);
    bipChnBG->addButton(dummyButton,btnIdx);
   }
   connect(bipChnBG,SIGNAL(buttonClicked(int)),this,SLOT(slotBipChnInter(int)));

   // *** EEG Streaming
   conf->sweepFrameW=tabW-10; conf->sweepFrameH=tabH-80;
   eegWidget=new QWidget(mainTabWidget); eegWidget->setGeometry(2,2,tabW-4,tabH-4); 
   eegFrame=new EEGFrame(conf,ampNo,eegWidget);
   eegFrame->setGeometry(6,6,conf->sweepFrameW,conf->sweepFrameH); 

   eegAmpBG=new QButtonGroup(); eegAmpBG->setExclusive(true);
   for (int btnIdx=0;btnIdx<6;btnIdx++) { // EEG Multiplier
    dummyButton=new QPushButton(eegWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(10+btnIdx*60,tabH-12,60,40);
    eegAmpBG->addButton(dummyButton,btnIdx);
   }
   eegAmpBG->button(0)->setText("1mV");   eegAmpBG->button(1)->setText("500uV");
   eegAmpBG->button(2)->setText("200uV"); eegAmpBG->button(3)->setText("100uV");
   eegAmpBG->button(4)->setText("50uV");  eegAmpBG->button(5)->setText("20uV");
   eegAmpBG->button(0)->setChecked(true);
   conf->eegAmpX[ampNo]=conf->eegAmpRange[0]; //(1000000.0/100.0);
   connect(eegAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGAmp(int)));

   mainTabWidget->addTab(chnWidget,"Channels");
   mainTabWidget->addTab(eegWidget,"TimeDomain EEG");
   mainTabWidget->show();

   setWindowTitle("Octopus Hyper EEG/ERP Amp #"+QString::number(ampNo+1));
  }

  ~AmpWindow() override {}

  ConfParam *conf;
  
  unsigned int ampNo;
 
 public slots:
  void slotEEGAmp(int x) { conf->eegAmpX[ampNo]=conf->eegAmpRange[x]; }
  void slotRefChnInter(int x) { applyChnInterChange(conf->refChns,refChnBG,x); }
  void slotBipChnInter(int x) { applyChnInterChange(conf->bipChns,bipChnBG,x); }

 private:
  void applyChnInterChange(QVector<GUIChnInfo> &chns,QButtonGroup *bg,int x) {
   if (x<0 || x>=chns.size()) return;
   if (ampNo>=(unsigned int)chns[x].interMode.size()) return;
   chns[x].interMode[ampNo]++;
   chns[x].interMode[ampNo]%=3;
   QString cmd=CMD_ACQ_COMPCHAN+"=";
   cmd.append(QString::number(ampNo+1)+",");
   cmd.append(QString::number(chns[x].type)+",");
   cmd.append(QString::number(chns[x].physChn)+",");
   cmd.append(QString::number(chns[x].interMode[ampNo]));
   QString commResponse=conf->commandToDaemon(conf->acqPPCommSocket,cmd);
   if (commResponse.isEmpty()) { qWarning() << "Empty COMPCHAN reply."; return; }
   QStringList sList=commResponse.split(",",Qt::KeepEmptyParts);
   if (sList.size()!=chns.size()) {
    qWarning() << "Bad COMPCHAN reply size:" << sList.size() << "expected:" << chns.size();
    return;
   }
   for (int idx=0;idx<chns.size();++idx) {
    chns[idx].interMode[ampNo]=sList[idx].toInt();
    QPushButton *b=qobject_cast<QPushButton*>(bg->button(idx));
    if (!b) continue;
    if (chns[idx].interMode[ampNo]==0) {
     b->setStyleSheet("QPushButton { background-color: white; color: black; }");
     b->setChecked(false);
    } else if (chns[idx].interMode[ampNo]==1) {
     b->setStyleSheet("QPushButton:checked { background-color: yellow; color: black; }");
     b->setChecked(true);
    } else {
     b->setStyleSheet("QPushButton:checked { background-color: red; color: white; }");
     b->setChecked(true);
    }
   }
  }

  QString dummyString; QPushButton *dummyButton;
  EEGFrame *eegFrame;

  QTabWidget *mainTabWidget; QWidget *eegWidget,*chnWidget;
  QButtonGroup *eegAmpBG,*erpAmpBG,*refChnBG,*bipChnBG;
};
