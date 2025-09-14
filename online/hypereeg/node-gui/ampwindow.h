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
#include "gl3dwidget.h"
#include "gllegendframe.h"

class AmpWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit AmpWindow(unsigned int aNo=0,ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
   ampNo=aNo; conf=c;
   setGeometry(ampNo*conf->guiAmpW+conf->guiAmpX,conf->guiAmpY,conf->guiAmpW,conf->guiAmpH);
   setFixedSize(conf->guiAmpW,conf->guiAmpH);

   conf->ampFrameH=conf->guiAmpH-90;
   //conf->ampFrameW=(int)(.66*(float)(conf->guiAmpW));
   conf->ampFrameW=conf->guiAmpW-6;
   if (conf->ampFrameW%2==1) conf->ampFrameW--;

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,this->width()-2,this->height());

   // *** EEG Streaming

   eegWidget=new QWidget(mainTabWidget);
   eegFrame=new EEGFrame(conf,ampNo,eegWidget);
   eegFrame->setGeometry(2,2,conf->ampFrameW,conf->ampFrameH); 
   eegWidget->show(); 

   eegAmpBG=new QButtonGroup(); eegAmpBG->setExclusive(true);
   for (int btnIdx=0;btnIdx<6;btnIdx++) { // EEG Amplification
    dummyButton=new QPushButton(eegWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(100+btnIdx*60,eegWidget->height(),60,20);
    eegAmpBG->addButton(dummyButton,btnIdx);
   }
   eegAmpBG->button(0)->setText("1mV");   eegAmpBG->button(1)->setText("500uV");
   eegAmpBG->button(2)->setText("200uV"); eegAmpBG->button(3)->setText("100uV");
   eegAmpBG->button(4)->setText("50uV");  eegAmpBG->button(5)->setText("20uV");
   eegAmpBG->button(0)->setChecked(true);
   conf->eegAmpX[ampNo]=conf->eegAmpRange[0]; //(1000000.0/100.0);
   connect(eegAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGAmp(int)));

   // *** Channels & Interpolation

   chnWidget=new QWidget(mainTabWidget);
   unsigned int chnIdx,topoX,topoY,a,y,sz; QString chnName;
   QRect cr(0,0,conf->ampFrameW-1,conf->ampFrameH-1);
   a=conf->ampFrameW/16; // guiCellSize
   sz=5*a/6;
   QRect mRect(0,0,sz,sz); QRegion mRegion(mRect,QRegion::Ellipse);
   chnBG=new QButtonGroup(); chnBG->setExclusive(false);
   for (int btnIdx=0;btnIdx<conf->chns.size();btnIdx++) { // Chns Interpolation
    chnIdx=(conf->chns)[btnIdx].physChn; chnName=(conf->chns)[btnIdx].chnName;
    topoX=(conf->chns)[btnIdx].topoX; topoY=(conf->chns)[btnIdx].topoY;
    y=0;
    if (topoY>1) y+=a/2;
    if (topoY>10) y+=a/2;
    QRect cr(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2,sz,sz);
    //QRect tr1(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2-sz/4,sz,sz);
    //QRect tr2(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2+sz/6,sz,sz);
    dummyButton=new QPushButton(chnWidget); dummyButton->setCheckable(true);
    dummyButton->setText(QString::number(chnIdx)+"\n"+chnName);
    dummyButton->setGeometry(cr); dummyButton->setMask(mRegion);
    dummyButton->setStyleSheet("QPushButton { background-color: white; }"
                               "QPushButton { color: black; }"
                               "QPushButton:checked { background-color: yellow; }"
                               "QPushButton:checked { color: red; }");
    dummyButton->setChecked(false);
    chnBG->addButton(dummyButton,btnIdx);
   }
   connect(chnBG,SIGNAL(buttonClicked(int)),this,SLOT(slotChnInter(int)));
   chnWidget->show(); 

   // *** ERP OpenGL View

   head3DWidget=new QWidget(mainTabWidget);
   head3DWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   gl3DContainer=new QWidget(head3DWidget);
   gl3DContainer->setGeometry(2,2,conf->ampFrameW*2/3-2,conf->ampFrameH-60); 
   gl3DWidget=new GL3DWidget(conf,ampNo,gl3DContainer);
//   gl3DWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   //gl3DWidget->setMinimumSize(600,600);
   //gl3DWidget->resize(1600, 1200);  // immediate push
   //gl3DWidget->setGeometry(2,2,conf->ampFrameW,conf->ampFrameH); 
//   gl3DWidget->setAttribute(Qt::WA_NoSystemBackground, true);
//   gl3DWidget->setAutoFillBackground(false);

   erpAmpBG=new QButtonGroup(); erpAmpBG->setExclusive(true);
   for (int btnIdx=0;btnIdx<6;btnIdx++) { // ERP Multiplier
    dummyButton=new QPushButton(head3DWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(100+btnIdx*60,eegWidget->height(),60,20);
    erpAmpBG->addButton(dummyButton,btnIdx);
   }
   erpAmpBG->button(0)->setText("100uV"); erpAmpBG->button(1)->setText("50uV");
   erpAmpBG->button(2)->setText("20uV");  erpAmpBG->button(3)->setText("10uV");
   erpAmpBG->button(4)->setText("5uV");   erpAmpBG->button(5)->setText("2uV");
   erpAmpBG->button(5)->setChecked(true);
   conf->erpAmpX[ampNo]=conf->erpAmpRange[0];
   connect(erpAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotERPAmp(int)));

   clrAvgsButton=new QPushButton("CLR",head3DWidget);
   clrAvgsButton->setGeometry(eegWidget->width()-50,eegWidget->height(),40,20);
   connect(clrAvgsButton,SIGNAL(clicked()),this,SLOT(slotClrAvgs()));

   cModeLabel=new QLabel("CM(RMS)("+dummyString.setNum(conf->headModel[ampNo].cModeThreshold)+"uV2):",head3DWidget);
   cModeLabel->setGeometry(2,conf->ampFrameH-40,120,20); 
   QSlider *cModeSlider=new QSlider(Qt::Horizontal,head3DWidget);
   cModeSlider->setGeometry(122,conf->ampFrameH-40,conf->ampFrameW*2/3-102,20); 
   cModeSlider->setRange(1,500); // in mm because of int - divide by ten
   cModeSlider->setSingleStep(1);
   cModeSlider->setPageStep(10); // step in uV2s..
   cModeSlider->setSliderPosition((int)(conf->headModel[ampNo].cModeThreshold*10.));
   cModeSlider->setEnabled(true);
   connect(cModeSlider,SIGNAL(valueChanged(int)),this,SLOT(slotSetCModeThr(int)));


   paramRLabel=new QLabel("P.Rad("+dummyString.setNum(conf->headModel[ampNo].scalpParamR)+"cm):",head3DWidget);
   paramRLabel->setGeometry(2,conf->ampFrameH-20,120,20); 
   QSlider *paramRSlider=new QSlider(Qt::Horizontal,head3DWidget);
   paramRSlider->setGeometry(122,conf->ampFrameH-20,conf->ampFrameW*2/3-102,20); 
   paramRSlider->setRange(70,500); // in mm because of int - divide by ten
   paramRSlider->setSingleStep(1);
   paramRSlider->setPageStep(10); // step in cms..
   paramRSlider->setSliderPosition(conf->headModel[ampNo].scalpParamR*10);
   paramRSlider->setEnabled(true);
   connect(paramRSlider,SIGNAL(valueChanged(int)),this,SLOT(slotSetScalpParamR(int)));

   gizmoList=new QListWidget(head3DWidget);
   gizmoList->setGeometry(2+conf->ampFrameW*2/3,2,conf->ampFrameW/3,conf->ampFrameH/3); 
   electrodeList=new QListWidget(head3DWidget);
   electrodeList->setGeometry(2+conf->ampFrameW*2/3,2+conf->ampFrameH/3,conf->ampFrameW/3,conf->ampFrameH/3); 
   glLegendFrame=new GLLegendFrame(conf,head3DWidget);
   glLegendFrame->setGeometry(2+conf->ampFrameW*2/3,2+conf->ampFrameH*2/3,conf->ampFrameW/3,conf->ampFrameH/3); 

   // Add all tab widgets

   mainTabWidget->addTab(eegWidget,"EEG Stream");
   mainTabWidget->addTab(chnWidget,"Chn Inter");
   mainTabWidget->addTab(head3DWidget,"3D Head");
   mainTabWidget->show();

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *modelMenu=new QMenu("&Model",menuBar); QMenu *viewMenu=new QMenu("&View",menuBar);
   menuBar->addMenu(modelMenu); menuBar->addMenu(viewMenu);

   // View Menu
   frameToggleAction=new QAction("&Frame",this); viewMenu->addAction(frameToggleAction);
   gridToggleAction=new QAction("Grid",this); viewMenu->addAction(gridToggleAction); viewMenu->addSeparator();
   scalpToggleAction=new QAction("&Scalp",this); viewMenu->addAction(scalpToggleAction);
   skullToggleAction=new QAction("Skull",this); viewMenu->addAction(skullToggleAction);
   brainToggleAction=new QAction("&Brain",this); viewMenu->addAction(brainToggleAction); viewMenu->addSeparator();
   gizmoToggleAction=new QAction("&Gizmo",this); viewMenu->addAction(gizmoToggleAction);
   electrodesToggleAction=new QAction("&Electrodes",this); viewMenu->addAction(electrodesToggleAction);
   connect(frameToggleAction,&QAction::triggered,this,[=]() {
    conf->glFrameOn[ampNo] ? conf->glFrameOn[ampNo]=false : conf->glFrameOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(gridToggleAction,&QAction::triggered,this,[=]() {
    conf->glGridOn[ampNo] ? conf->glGridOn[ampNo]=false : conf->glGridOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(scalpToggleAction,&QAction::triggered,this,[=]() {
    conf->glScalpOn[ampNo] ? conf->glScalpOn[ampNo]=false : conf->glScalpOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(skullToggleAction,&QAction::triggered,this,[=]() {
    conf->glSkullOn[ampNo] ? conf->glSkullOn[ampNo]=false : conf->glSkullOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(brainToggleAction,&QAction::triggered,this,[=]() {
    conf->glBrainOn[ampNo] ? conf->glBrainOn[ampNo]=false : conf->glBrainOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(gizmoToggleAction,&QAction::triggered,this,[=]() {
    conf->glGizmoOn[ampNo] ? conf->glGizmoOn[ampNo]=false : conf->glGizmoOn[ampNo]=true;
    conf->glUpdate();
   });
   connect(electrodesToggleAction,&QAction::triggered,this,[=]() {
    conf->glElectrodesOn[ampNo] ? conf->glElectrodesOn[ampNo]=false : conf->glElectrodesOn[ampNo]=true;
    conf->glUpdate();
   });

   setMenuBar(menuBar);

   setWindowTitle("Octopus Hyper EEG/ERP Amp #"+QString::number(ampNo+1));
  }

  ~AmpWindow() override {}

  ConfParam *conf;
  
  unsigned int ampNo;
 
 public slots:
  void slotEEGAmp(int x) { conf->eegAmpX[ampNo]=conf->eegAmpRange[x]; }

  void slotChnInter(int x) { conf->chns[x].interActive[ampNo]=!conf->chns[x].interActive[ampNo];
  // qDebug() << ampNo << "Chn" << x << conf->chns[x].interActive[ampNo];
  }

  void slotERPAmp(int x) { conf->erpAmpX[ampNo]=conf->erpAmpRange[x]; }

  void slotClrAvgs() { }

  void slotSetCModeThr(int x) {
   conf->headModel[ampNo].cModeThreshold=(float)(x)/10.; // .1 uV2 Resolution
   cModeLabel->setText("CM(RMS)("+dummyString.setNum(conf->headModel[ampNo].cModeThreshold)+"uV2):");
   conf->glUpdateParam();
  }

  void slotSetScalpParamR(int x) {
   conf->headModel[ampNo].scalpParamR=(float)(x)/10.;
   paramRLabel->setText("P.Rad("+dummyString.setNum(conf->headModel[ampNo].scalpParamR)+"cm):");
   conf->glUpdateParam();
  }

 private:
  QString dummyString; QPushButton *dummyButton;
  EEGFrame *eegFrame; GL3DWidget *gl3DWidget; GLLegendFrame *glLegendFrame;
  QMenuBar *menuBar;
  QAction *frameToggleAction,*gridToggleAction, *scalpToggleAction,*skullToggleAction,*brainToggleAction,*gizmoToggleAction,*electrodesToggleAction;
  QTabWidget *mainTabWidget; QWidget *eegWidget,*chnWidget,*head3DWidget;
  QButtonGroup *eegAmpBG,*erpAmpBG,*chnBG;

  QWidget *gl3DContainer;
  QListWidget *gizmoList,*electrodeList;
  QLabel *cModeLabel,*paramRLabel; QSlider *cModeSlider,*paramRSlider;
  QPushButton *clrAvgsButton;
  //QStatusBar *ctrlStatusBar;
};

   // Model Menu
//   loadRealAction=new QAction("&Load Real...",this);
//   saveRealAction=new QAction("&Save Real...",this);
//   saveAvgsAction=new QAction("&Save All Averages",this);

//   loadRealAction->setStatusTip("Load previously saved electrode coordinates..");
//   saveRealAction->setStatusTip("Save measured/real electrode coordinates..");
//   saveAvgsAction->setStatusTip("Save current averages to separate files per event..");

//   connect(loadRealAction,SIGNAL(triggered()),this,SLOT(slotLoadReal()));
//   connect(saveRealAction,SIGNAL(triggered()),this,SLOT(slotSaveReal()));
//   connect(saveAvgsAction,SIGNAL(triggered()),(QObject *)p,SLOT(slotExportAvgs()));

//   modelMenu->addAction(loadRealAction);
//   modelMenu->addAction(saveRealAction);
//   modelMenu->addSeparator();
//   modelMenu->addAction(saveAvgsAction);

   // View Menu
//   toggleFrameAction=new QAction("Cartesian Frame",this);
//   toggleGridAction=new QAction("Cartesian Grid",this);
//   toggleDigAction=new QAction("Digitizer Coords/Frame",this);
//   toggleParamAction=new QAction("Parametric Coords",this);
//   toggleRealAction=new QAction("Measured Coords",this);
//   toggleGizmoAction=new QAction("Gizmos",this);
//   toggleAvgsAction=new QAction("Averages",this);
//   toggleScalpAction=new QAction("MRI/Real Scalp Model",this);
//   toggleSkullAction=new QAction("MRI/Real Skull Model",this);
//   toggleBrainAction=new QAction("MRI/Real Brain Model",this);
//   toggleSourceAction=new QAction("Source Model",this);

//   toggleFrameAction->setStatusTip("Show/hide cartesian XYZ frame.");
//   toggleGridAction->setStatusTip("Show/hide cartesian grid/rulers.");
//   toggleDigAction->setStatusTip("Show/hide digitizer output.");
//   toggleParamAction->setStatusTip("Show/hide parametric/spherical coords/template.");
//   toggleRealAction->setStatusTip("Show/hide measured/real coords.");
//   toggleGizmoAction->setStatusTip("Show/hide loaded gizmo/hint list.");
//   toggleAvgsAction->setStatusTip("Show/hide Event Related Potentials on electrodes.");
//   toggleScalpAction->setStatusTip("Show/hide realistic scalp segmented from MRI data.");
//   toggleSkullAction->setStatusTip("Show/hide realistic skull segmented from MRI data.");
//   toggleBrainAction->setStatusTip("Show/hide realistic brain segmented from MRI data.");
//   toggleBrainAction->setStatusTip("Show/hide source model.");

//   connect(toggleFrameAction,SIGNAL(triggered()),this,SLOT(slotToggleFrame()));
//   connect(toggleGridAction,SIGNAL(triggered()),this,SLOT(slotToggleGrid()));
//   connect(toggleDigAction,SIGNAL(triggered()),this,SLOT(slotToggleDig()));
//   connect(toggleParamAction,SIGNAL(triggered()),this,SLOT(slotToggleParam()));
//   connect(toggleRealAction,SIGNAL(triggered()),this,SLOT(slotToggleReal()));
//   connect(toggleGizmoAction,SIGNAL(triggered()),this,SLOT(slotToggleGizmo()));
//   connect(toggleAvgsAction,SIGNAL(triggered()),this,SLOT(slotToggleAvgs()));
//   connect(toggleScalpAction,SIGNAL(triggered()),this,SLOT(slotToggleScalp()));
//   connect(toggleSkullAction,SIGNAL(triggered()),this,SLOT(slotToggleSkull()));
//   connect(toggleBrainAction,SIGNAL(triggered()),this,SLOT(slotToggleBrain()));
//   connect(toggleSourceAction,SIGNAL(triggered()),this,SLOT(slotToggleSource()));

//   viewMenu->addAction(toggleFrameAction);
//   viewMenu->addAction(toggleGridAction);
//   viewMenu->addAction(toggleDigAction);
//   viewMenu->addSeparator();
//   viewMenu->addAction(toggleParamAction);
//   viewMenu->addAction(toggleRealAction);
//   viewMenu->addSeparator();
//   viewMenu->addAction(toggleGizmoAction);
//   viewMenu->addAction(toggleAvgsAction);
//   viewMenu->addSeparator();
//   viewMenu->addAction(toggleScalpAction);
//   viewMenu->addAction(toggleSkullAction);
//   viewMenu->addAction(toggleBrainAction);
//   viewMenu->addSeparator();
//   viewMenu->addAction(toggleSourceAction);

//   QLabel *gizmoLabel=new QLabel("Gizmo",this);
//   gizmoLabel->setGeometry(p->hwHeight+4,80,(p->hwWidth-p->hwHeight)/2-10,20);
//   QLabel *electrodeLabel=new QLabel("Electrode",this);
//   electrodeLabel->setGeometry(p->hwHeight+(p->hwWidth-p->hwHeight)/2+14,80,(p->hwWidth-p->hwHeight)/2-10,20);

//   gizmoRealCB=new QCheckBox("Gizmo on Real Model",this);
//   gizmoRealCB->setGeometry(p->hwHeight+4,p->hwHeight-286,p->hwWidth-p->hwHeight-20,20);
//   connect(gizmoRealCB,SIGNAL(clicked()),this,SLOT(slotGizmoRealCB()));

//   elecRealCB=new QCheckBox("Electrodes on Real Model",this);
//   elecRealCB->setGeometry(p->hwHeight+4,p->hwHeight-266,p->hwWidth-p->hwHeight-20,20);
//   connect(elecRealCB,SIGNAL(clicked()),this,SLOT(slotElecRealCB()));

//   connect(gizmoList,SIGNAL(currentRowChanged(int)),this,SLOT(slotSelectGizmo(int)));
//   connect(electrodeList,SIGNAL(currentRowChanged(int)),this,SLOT(slotSelectElectrode(int)));
//   connect(electrodeList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,SLOT(slotViewElectrode(QListWidgetItem *)));

//   for (int i=0;i<p->gizmo.size();i++) gizmoList->addItem(p->gizmo[i]->name);

//   QLabel *legendLabel=new QLabel("Event Counts/Legend:",this);
//   legendLabel->setGeometry(p->hwHeight-20,p->hwHeight-240,170,20);
//   legendFrame=new LegendFrame(this,p);
//   legendFrame->setGeometry(p->hwHeight-20,p->hwHeight-220,p->hwWidth-p->hwHeight+10,120);

//   timePtLabel=new QLabel("Localization Time Point ("+dummyString.setNum(p->cp.avgBwd+p->slTimePt*1000/p->sampleRate)+" ms):",this);
//   timePtLabel->setGeometry(p->hwHeight-20,p->hwHeight-80,p->hwWidth-p->hwHeight-10,20);

//   QSlider *timePtSlider=new QSlider(Qt::Horizontal,this);
//   timePtSlider->setGeometry(p->hwHeight-20,p->hwHeight-60,p->hwWidth-p->hwHeight-10,20);
//   timePtSlider->setRange(0,p->cp.avgCount-1);
//   timePtSlider->setSingleStep(1);
//   timePtSlider->setPageStep(1);
//   timePtSlider->setValue(p->slTimePt);
//   timePtSlider->setEnabled(true);
//   connect(timePtSlider,SIGNAL(valueChanged(int)),this,SLOT(slotSetTimePt(int)));

//   clrAvgsButton=new QPushButton("CLR",this);
//   clrAvgsButton->setGeometry(550,height()-24,60,20);
//   connect(clrAvgsButton,SIGNAL(clicked()),(QObject *)p,SLOT(slotClrAvgs()));

   //if (gizmoList->count()>0) {
   // gizmoList->setCurrentRow(0); slotSelectGizmo(0);
   //}
 
// private slots:
//  void slotRepaint() {
//   electrodeList->setCurrentRow(p->curElecInSeq); legendFrame->repaint();
//  }

//  void slotLoadReal() {
//   QString fileName=QFileDialog::getOpenFileName(this,"Load Real Coordset File",".","Object Files (*.orc)");
//   if (!fileName.isEmpty()) { p->loadReal(fileName); }
//   else { qDebug("An error has been occured while loading measured coords!"); }
//  }

//  void slotSaveReal() {
//   QString fileName=QFileDialog::getSaveFileName(this,"Save Real Coordset File",".","Object Files (*.orc)");
//   if (!fileName.isEmpty()) { p->saveReal(fileName+".orc"); }
//   else { qDebug("An error has been occured while saving measured coords!"); }
//  }

//  void slotToggleFrame()     { p->hwFrameV   =(p->hwFrameV)   ? false:true; }
//  void slotToggleGrid()      { p->hwGridV    =(p->hwGridV)    ? false:true; }
//  void slotToggleDig()       { p->hwDigV     =(p->hwDigV)     ? false:true; }
//  void slotToggleParam()     { p->hwParamV   =(p->hwParamV)   ? false:true; }
//  void slotToggleReal()      { p->hwRealV    =(p->hwRealV)    ? false:true; }
//  void slotToggleGizmo()     { p->hwGizmoV   =(p->hwGizmoV)   ? false:true; }
//  void slotToggleAvgs()      { p->hwAvgsV    =(p->hwAvgsV)    ? false:true; }
//  void slotToggleScalp()     { p->hwScalpV   =(p->hwScalpV)   ? false:true; }
//  void slotToggleSkull()     { p->hwSkullV   =(p->hwSkullV)   ? false:true; }
//  void slotToggleBrain()     { p->hwBrainV   =(p->hwBrainV)   ? false:true; }
//  void slotToggleSource()    { p->hwSourceV  =(p->hwSourceV)  ? false:true; }

//  void slotSetTimePt(int x) { p->slTimePt=(float)(x);
//   timePtLabel->setText("Localization Time Point ("+dummyString.setNum(p->cp.avgBwd+x*1000/p->sampleRate)+" ms):");
//   headGLWidget->slotRepaintGL(16); // update avgs
//  }

//  void slotSelectGizmo(int k) { int idx; Gizmo *g=p->gizmo[k];
//   p->currentGizmo=k; electrodeList->clear();
//   for (int i=0;i<g->seq.size();i++) {
//    for (int j=0;j<p->acqChannels.size();j++)
//     if (p->acqChannels[j]->physChn==g->seq[i]-1) { idx=j; break; }
//    dummyString.setNum(p->acqChannels[idx]->physChn+1);
//    electrodeList->addItem(dummyString+" "+p->acqChannels[idx]->name);
//   }
//   p->curElecInSeq=0;
//   for (int j=0;j<p->acqChannels.size();j++)
//    if (p->acqChannels[j]->physChn==g->seq[0]-1) { idx=j; break; }
//   p->currentElectrode=idx;
//   electrodeList->setCurrentRow(p->curElecInSeq);
//   headGLWidget->slotRepaintGL(8);
//  }

//  void slotSelectElectrode(int k) { p->curElecInSeq=k;
//   for (int i=0;i<p->acqChannels.size();i++)
//    if (p->acqChannels[i]->physChn==p->gizmo[p->currentGizmo]->seq[k]-1)
//     { p->currentElectrode=i; break; }
//   headGLWidget->slotRepaintGL(2+16); // update real+avgs
//  }

//  void slotViewElectrode(QListWidgetItem *item) { int idx,pChn;
//   QString s=item->text(); QStringList l=s.split(" "); pChn=l[0].toInt()-1;
//   for (int i=0;i<p->acqChannels.size();i++)
//    if (p->acqChannels[i]->physChn==pChn) { idx=i; break; }
//   float theta=p->acqChannels[idx]->param.y;
//   float phi=p->acqChannels[idx]->param.z;
//   headGLWidget->setView(theta,phi);
//  }

//  void slotGizmoRealCB() {
//   p->gizmoOnReal=gizmoRealCB->isChecked();
//   headGLWidget->slotRepaintGL(8); // update gizmo
//  }

//  void slotElecRealCB() {
//   p->elecOnReal=elecRealCB->isChecked();
//   headGLWidget->slotRepaintGL(16); // update averages
//  }

//  QAction *loadRealAction,*saveRealAction,*saveAvgsAction,
//          *toggleFrameAction,*toggleGridAction,*toggleDigAction,
//          *toggleParamAction,*toggleRealAction,
//          *toggleGizmoAction,*toggleAvgsAction,
//          *toggleScalpAction,*toggleSkullAction,*toggleBrainAction,
//          *toggleSourceAction;

//  QLabel *paramRLabel,*notchLabel,*timePtLabel;
//  QCheckBox *gizmoRealCB,*elecRealCB; QString dummyString;
//  QListWidget *gizmoList,*electrodeList; LegendFrame *legendFrame;

//  QButtonGroup *avgAmpBG; QVector<QPushButton*> avgAmpButtons;
