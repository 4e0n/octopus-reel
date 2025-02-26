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

#ifndef ACQCLIENT_H
#define ACQCLIENT_H

#include <QtGui>
#include <QMainWindow>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSlider>

#include "acqmaster.h"
#include "cntframe.h"
#include "headglwidget.h"
#include "legendframe.h"

class AcqClient : public QMainWindow {
 Q_OBJECT
 public:
  AcqClient(AcqMaster *acqm,unsigned int a,QWidget *parent=0) : QMainWindow(parent) {
   acqM=acqm; ampNo=a; setGeometry(acqM->contGuiX+ampNo*(acqM->contGuiW+12),acqM->contGuiY,acqM->contGuiW,acqM->contGuiH);
   setFixedSize(acqM->contGuiW,acqM->contGuiH);

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,this->width()-2,this->height());

   cntWidget=new QWidget(mainTabWidget);
   cntFrame=new CntFrame(cntWidget,acqM,ampNo);
   cntFrame->setGeometry(2,2,acqM->acqFrameW,acqM->acqFrameH); cntWidget->show();
   headGLWidget=new HeadGLWidget(cntWidget,acqM,ampNo);
   headGLWidget->setGeometry(2+acqM->acqFrameW+5,2,acqM->glFrameW,acqM->glFrameH); headGLWidget->show();

   mainTabWidget->addTab(cntWidget,"EEG/ERP"); mainTabWidget->show();

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   QPushButton *dummyButton;
   cntAmpBG=new QButtonGroup(); cntAmpBG->setExclusive(true);
   for (int i=0;i<6;i++) { // EEG AMP
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(100+i*60,cntWidget->height(),60,20);
    cntAmpBG->addButton(dummyButton,i);
   }
   cntAmpBG->button(0)->setText("1mV");
   cntAmpBG->button(1)->setText("500uV");
   cntAmpBG->button(2)->setText("200uV");
   cntAmpBG->button(3)->setText("100uV");
   cntAmpBG->button(4)->setText("50uV");
   cntAmpBG->button(5)->setText("20uV");
   cntAmpBG->button(3)->setDown(true);
   acqM->cntAmpX[ampNo]=(1000000.0/100.0);
   connect(cntAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotCntAmp(int)));

   // *** HEAD & CONFIG WINDOW ***

   avgAmpBG=new QButtonGroup(); avgAmpBG->setExclusive(true);
   for (int i=0;i<6;i++) { // ERP AMP
    dummyButton=new QPushButton(cntWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(acqM->acqFrameW+(acqM->glFrameW-360)/2+i*60,acqM->glFrameH+4,60,20);
    avgAmpBG->addButton(dummyButton,i);
   }
   avgAmpBG->button(0)->setText("2uV");
   avgAmpBG->button(1)->setText("5uV");
   avgAmpBG->button(2)->setText("10uV");
   avgAmpBG->button(3)->setText("20uV");
   avgAmpBG->button(4)->setText("50uV");
   avgAmpBG->button(5)->setText("100uV");
   avgAmpBG->button(5)->setDown(true);
   acqM->avgAmpX[ampNo]=100;

   connect(avgAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotAvgAmp(int)));

//   acqM->regRepaintHeadWindow(this);

   paramRLabel=new QLabel("Param.Radius ("+
                          dummyString.setNum(acqM->scalpParamR[ampNo])+" cm):",cntWidget);
   paramRLabel->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+34,190,20);

   QSlider *paramRSlider=new QSlider(Qt::Horizontal,cntWidget);
   paramRSlider->setGeometry(acqM->acqFrameW+200,acqM->glFrameH+36,acqM->glFrameW-210,20);
   paramRSlider->setRange(70,300); // in mm because of int - divide by ten
   paramRSlider->setSingleStep(1);
   paramRSlider->setPageStep(10); // step in cms..
   paramRSlider->setSliderPosition(acqM->scalpParamR[ampNo]*10);
   paramRSlider->setEnabled(true);
   connect(paramRSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetScalpParamR(int)));

   gizmoRealCB=new QCheckBox("Gizmos",cntWidget); gizmoRealCB->setChecked(true);
   gizmoRealCB->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+70,100,20);
   connect(gizmoRealCB,SIGNAL(clicked()),this,SLOT(slotGizmoRealCB()));
   elecRealCB=new QCheckBox("Electrodes",cntWidget); elecRealCB->setChecked(true);
   elecRealCB->setGeometry(acqM->acqFrameW+acqM->glFrameW/2+15,acqM->glFrameH+70,100,20);
   connect(elecRealCB,SIGNAL(clicked()),this,SLOT(slotElecRealCB()));

   gizmoList=new QListWidget(cntWidget);
   gizmoList->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+100,acqM->glFrameW/2-10,100);
   electrodeList=new QListWidget(cntWidget);
   electrodeList->setGeometry(acqM->acqFrameW+acqM->glFrameW/2+15,acqM->glFrameH+100,acqM->glFrameW/2-10,100);

   notchLabel=new QLabel("Notch Level (RMS) ("+
                    dummyString.setNum(acqM->notchThreshold)+" uV):",cntWidget);
   notchLabel->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+210,190,20);
   QSlider *notchSlider=new QSlider(Qt::Horizontal,cntWidget);
   notchSlider->setGeometry(acqM->acqFrameW+200,acqM->glFrameH+212,acqM->glFrameW-210,20);
   notchSlider->setRange(1,100); // in mm because of int - divide by ten
   notchSlider->setSingleStep(1);
   notchSlider->setPageStep(10); // step in uVs..
   notchSlider->setSliderPosition((int)(acqM->notchThreshold*1.));
   notchSlider->setEnabled(true);
   connect(notchSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetNotchThr(int)));

   connect(gizmoList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectGizmo(int)));
   connect(electrodeList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectElectrode(int)));
   connect(electrodeList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),
           this,SLOT(slotViewElectrode(QListWidgetItem *)));

   for (int i=0;i<acqM->gizmo.size();i++) gizmoList->addItem(acqM->gizmo[i]->name);

   QLabel *legendLabel=new QLabel("Event Counts/Legend:",cntWidget);
   legendLabel->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+240,200,20);
   legendFrame=new LegendFrame(cntWidget,acqM);
   legendFrame->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+266,acqM->glFrameW-4,80);
   timePtLabel=new QLabel("Src. Time Point ("+dummyString.setNum(
                          acqM->cp.avgBwd+acqM->slTimePt*1000/acqM->sampleRate
                           )+" ms):",cntWidget);
   timePtLabel->setGeometry(acqM->acqFrameW+10,acqM->glFrameH+354,200,20);
   QSlider *timePtSlider=new QSlider(Qt::Horizontal,cntWidget);
   timePtSlider->setGeometry(acqM->acqFrameW+200,acqM->glFrameH+356,acqM->glFrameW-210,20);
   timePtSlider->setRange(0,acqM->cp.avgCount-1);
   timePtSlider->setSingleStep(1);
   timePtSlider->setPageStep(1);
   timePtSlider->setValue(acqM->slTimePt);
   timePtSlider->setEnabled(true);
   connect(timePtSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetTimePt(int)));

   clrAvgsButton=new QPushButton("CLR",cntWidget);
   clrAvgsButton->setGeometry(acqM->acqFrameW+acqM->glFrameW-50,acqM->glFrameH+240,40,20);
   connect(clrAvgsButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotClrAvgs()));

   //if (gizmoList->count()>0) {
   // gizmoList->setCurrentRow(0); slotSelectGizmo(0);
   //}
 
   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *modelMenu=new QMenu("&Model",menuBar);
   QMenu *viewMenu=new QMenu("&View",menuBar);
   menuBar->addMenu(modelMenu);
   menuBar->addMenu(viewMenu);
   setMenuBar(menuBar);

   // Model Menu
   loadRealAction=new QAction("&Load Real...",this);
   saveRealAction=new QAction("&Save Real...",this);
   saveAvgsAction=new QAction("&Save All Averages",this);

   loadRealAction->setStatusTip(
    "Load previously saved electrode coordinates..");
   saveRealAction->setStatusTip(
    "Save measured/real electrode coordinates..");
   saveAvgsAction->setStatusTip(
    "Save current averages to separate files per event..");

   connect(loadRealAction,SIGNAL(triggered()),this,SLOT(slotLoadReal()));
   connect(saveRealAction,SIGNAL(triggered()),this,SLOT(slotSaveReal()));
   connect(saveAvgsAction,SIGNAL(triggered()),
           (QObject *)acqM,SLOT(slotExportAvgs()));

   modelMenu->addAction(loadRealAction);
   modelMenu->addAction(saveRealAction);
   modelMenu->addSeparator();
   modelMenu->addAction(saveAvgsAction);

   // View Menu
   toggleFrameAction=new QAction("Cartesian Frame",this);
   toggleGridAction=new QAction("Cartesian Grid",this);
   toggleDigAction=new QAction("Digitizer Coords/Frame",this);
   toggleParamAction=new QAction("Parametric Coords",this);
   toggleRealAction=new QAction("Measured Coords",this);
   toggleGizmoAction=new QAction("Gizmos",this);
   toggleAvgsAction=new QAction("Averages",this);
   toggleScalpAction=new QAction("MRI/Real Scalp Model",this);
   toggleSkullAction=new QAction("MRI/Real Skull Model",this);
   toggleBrainAction=new QAction("MRI/Real Brain Model",this);
   toggleSourceAction=new QAction("Source Model",this);

   toggleFrameAction->setStatusTip("Show/hide cartesian XYZ frame.");
   toggleGridAction->setStatusTip("Show/hide cartesian grid/rulers.");
   toggleDigAction->setStatusTip("Show/hide digitizer output.");
   toggleParamAction->setStatusTip(
    "Show/hide parametric/spherical coords/template.");
   toggleRealAction->setStatusTip(
    "Show/hide measured/real coords.");
   toggleGizmoAction->setStatusTip(
    "Show/hide loaded gizmo/hint list.");
   toggleAvgsAction->setStatusTip(
    "Show/hide Event Related Potentials on electrodes.");
   toggleScalpAction->setStatusTip(
    "Show/hide realistic scalp segmented from MRI data.");
   toggleSkullAction->setStatusTip(
    "Show/hide realistic skull segmented from MRI data.");
   toggleBrainAction->setStatusTip(
    "Show/hide realistic brain segmented from MRI data.");
   toggleBrainAction->setStatusTip(
    "Show/hide source model.");

   connect(toggleFrameAction,SIGNAL(triggered()),this,SLOT(slotToggleFrame()));
   connect(toggleGridAction,SIGNAL(triggered()),this,SLOT(slotToggleGrid()));
   connect(toggleDigAction,SIGNAL(triggered()),this,SLOT(slotToggleDig()));
   connect(toggleParamAction,SIGNAL(triggered()),this,SLOT(slotToggleParam()));
   connect(toggleRealAction,SIGNAL(triggered()),this,SLOT(slotToggleReal()));
   connect(toggleGizmoAction,SIGNAL(triggered()),this,SLOT(slotToggleGizmo()));
   connect(toggleAvgsAction,SIGNAL(triggered()),this,SLOT(slotToggleAvgs()));
   connect(toggleScalpAction,SIGNAL(triggered()),this,SLOT(slotToggleScalp()));
   connect(toggleSkullAction,SIGNAL(triggered()),this,SLOT(slotToggleSkull()));
   connect(toggleBrainAction,SIGNAL(triggered()),this,SLOT(slotToggleBrain()));
   connect(toggleSourceAction,SIGNAL(triggered()),
           this,SLOT(slotToggleSource()));

   viewMenu->addAction(toggleFrameAction);
   viewMenu->addAction(toggleGridAction);
   viewMenu->addAction(toggleDigAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleParamAction);
   viewMenu->addAction(toggleRealAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleGizmoAction);
   viewMenu->addAction(toggleAvgsAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleScalpAction);
   viewMenu->addAction(toggleSkullAction);
   viewMenu->addAction(toggleBrainAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleSourceAction);

   setWindowTitle("Octopus Hyper EEG/ERP Amp #"+QString::number(ampNo+1));
  }

 signals: void scrollData();

 private slots:

  // *** AMPLITUDE & SPEED OF THE VISUALS ***

  void slotCntAmp(int x) {
   switch (x) {
    case 0: acqM->cntAmpX[ampNo]=(1000000.0/1000.0);cntAmpBG->button(3)->setDown(false);break;
    case 1: acqM->cntAmpX[ampNo]=(1000000.0/500.0);cntAmpBG->button(3)->setDown(false);break;
    case 2: acqM->cntAmpX[ampNo]=(1000000.0/200.0);cntAmpBG->button(3)->setDown(false);break;
    case 3: acqM->cntAmpX[ampNo]=(1000000.0/100.0);break;
    case 4: acqM->cntAmpX[ampNo]=(1000000.0/50.0);cntAmpBG->button(3)->setDown(false);break;
    case 5: acqM->cntAmpX[ampNo]=(1000000.0/20.0);cntAmpBG->button(3)->setDown(false);break;
   }
  }

  void slotRepaint() {
   electrodeList->setCurrentRow(acqM->curElecInSeq[ampNo]); legendFrame->repaint();
  }

  void slotLoadReal() {
   QString fileName=QFileDialog::getOpenFileName(this,"Load Real Coordset File",
                                 ".","Object Files (*.orc)");
   if (!fileName.isEmpty()) { acqM->loadReal(fileName); }
   else { qDebug("An error has been occured while loading measured coords!"); }
  }

  void slotSaveReal() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Real Coordset File",
                                 ".","Object Files (*.orc)");
   if (!fileName.isEmpty()) { acqM->saveReal(fileName+".orc"); }
   else { qDebug("An error has been occured while saving measured coords!"); }
  }

  void slotToggleFrame()  { acqM->hwFrameV[ampNo]  = (acqM->hwFrameV[ampNo])  ? false:true; }
  void slotToggleGrid()   { acqM->hwGridV[ampNo]   = (acqM->hwGridV[ampNo])   ? false:true; }
  void slotToggleDig()    { acqM->hwDigV[ampNo]    = (acqM->hwDigV)[ampNo]    ? false:true; }
  void slotToggleParam()  { acqM->hwParamV[ampNo]  = (acqM->hwParamV)[ampNo]  ? false:true; }
  void slotToggleReal()   { acqM->hwRealV[ampNo]   = (acqM->hwRealV)[ampNo]   ? false:true; }
  void slotToggleGizmo()  { acqM->hwGizmoV[ampNo]  = (acqM->hwGizmoV)[ampNo]  ? false:true; }
  void slotToggleAvgs()   { acqM->hwAvgsV[ampNo]   = (acqM->hwAvgsV)[ampNo]   ? false:true; }
  void slotToggleScalp()  { acqM->hwScalpV[ampNo]  = (acqM->hwScalpV)[ampNo]  ? false:true; }
  void slotToggleSkull()  { acqM->hwSkullV[ampNo]  = (acqM->hwSkullV)[ampNo]  ? false:true; }
  void slotToggleBrain()  { acqM->hwBrainV[ampNo]  = (acqM->hwBrainV)[ampNo]  ? false:true; }
  void slotToggleSource() { acqM->hwSourceV[ampNo] = (acqM->hwSourceV)[ampNo] ? false:true; }

  void slotSetScalpParamR(int x) {
   acqM->scalpParamR[ampNo]=(float)(x)/10.;
   paramRLabel->setText("Parametric Radius ("+
                        dummyString.setNum(acqM->scalpParamR[ampNo])+" cm):");
   headGLWidget->slotRepaintGL(2+8+16); // update real+gizmo+avgs
  }

  void slotSetNotchThr(int x) {
   acqM->notchThreshold=(float)(x)/10.; // .1 uV Resolution
   notchLabel->setText("Notch Level (RMS) ("+
                        dummyString.setNum(acqM->notchThreshold)+" uV):");
  }

  void slotSetTimePt(int x) { acqM->slTimePt=(float)(x);
   timePtLabel->setText("Localization Time Point ("+dummyString.setNum(
                          acqM->cp.avgBwd+x*1000/acqM->sampleRate
                           )+" ms):");
   headGLWidget->slotRepaintGL(16); // update avgs
  }

  void slotSelectGizmo(int k) { int idx; Gizmo *g=acqM->gizmo[k];
   acqM->currentGizmo[ampNo]=k; electrodeList->clear();
   for (int i=0;i<g->seq.size();i++) {
    for (int j=0;j<acqM->acqChannels[ampNo].size();j++)
     if (acqM->acqChannels[ampNo][j]->physChn==g->seq[i]-1) { idx=j; break; }
    dummyString.setNum(acqM->acqChannels[ampNo][idx]->physChn+1);
    electrodeList->addItem(dummyString+" "+acqM->acqChannels[ampNo][idx]->name);
   }
   acqM->curElecInSeq[ampNo]=0;
   for (int j=0;j<acqM->acqChannels[ampNo].size();j++)
    if (acqM->acqChannels[ampNo][j]->physChn==g->seq[0]-1) { idx=j; break; }
   acqM->currentElectrode[ampNo]=idx;
   electrodeList->setCurrentRow(acqM->curElecInSeq[ampNo]);
   headGLWidget->slotRepaintGL(8);
  }

  void slotSelectElectrode(int k) { acqM->curElecInSeq[ampNo]=k;
   for (int i=0;i<acqM->acqChannels[ampNo].size();i++)
    if (acqM->acqChannels[ampNo][i]->physChn==acqM->gizmo[acqM->currentGizmo[ampNo]]->seq[k]-1)
     { acqM->currentElectrode[ampNo]=i; break; }
   headGLWidget->slotRepaintGL(2+16); // update real+avgs
  }

  void slotViewElectrode(QListWidgetItem *item) { int idx,pChn;
   QString s=item->text(); QStringList l=s.split(" "); pChn=l[0].toInt()-1;
   for (int i=0;i<acqM->acqChannels[ampNo].size();i++)
    if (acqM->acqChannels[ampNo][i]->physChn==pChn) { idx=i; break; }
   float theta=acqM->acqChannels[ampNo][idx]->param.y;
   float phi=acqM->acqChannels[ampNo][idx]->param.z;
   headGLWidget->setView(theta,phi);
  }

  void slotGizmoRealCB() {
   acqM->gizmoOnReal[ampNo]=gizmoRealCB->isChecked();
   headGLWidget->slotRepaintGL(8); // update gizmo
  }

  void slotElecRealCB() {
   acqM->elecOnReal[ampNo]=elecRealCB->isChecked();
   headGLWidget->slotRepaintGL(16); // update averages
  }

  void slotAvgAmp(int x) {
   switch (x) {
    case 0: acqM->avgAmpX[ampNo]=  2.0; avgAmpBG->button(5)->setDown(false); break;
    case 1: acqM->avgAmpX[ampNo]=  5.0; avgAmpBG->button(5)->setDown(false); break;
    case 2: acqM->avgAmpX[ampNo]= 10.0; avgAmpBG->button(5)->setDown(false); break;
    case 3: acqM->avgAmpX[ampNo]= 20.0; avgAmpBG->button(5)->setDown(false); break;
    case 4: acqM->avgAmpX[ampNo]= 50.0; avgAmpBG->button(5)->setDown(false); break;
    case 5: acqM->avgAmpX[ampNo]=100.0; avgAmpBG->button(5)->setDown(false); break;
   }
  }
 //protected:
  //void resizeEvent(QResizeEvent *event) {
   //int w,h;
   //resizeEvent(event); // Call base class event handler

   //if (acqM->clientRunning) {
   //acqM->contGuiW=w=event->size().width();
   //acqM->contGuiH=h=event->size().height();

   // Resize child widgets proportionally
   //cntFrame->setGeometry(2,2,w-9,h-60);
   //mainTabWidget->setGeometry(1,32,w-2,h-60);
   //cntWidget->setGeometry(0,0,w,h);
   //acqM->guiStatusBar->setGeometry(0,h-20,w,20);
   //acqM->timeLabel->setGeometry(acqM->timeLabel->x(),mainTabWidget->height()-52,
   //                             acqM->timeLabel->width(),acqM->timeLabel->height());
   //menuBar->setFixedWidth(w);
   //for (int i=0;i<6;i++) cntAmpBG->button(i)->setGeometry(mainTabWidget->width()-676+i*60,mainTabWidget->height()-54,
   //                                                       cntAmpBG->button(i)->width(),cntAmpBG->button(i)->height());
   //}
  //}

 private:
  AcqMaster *acqM; CntFrame *cntFrame; HeadGLWidget *headGLWidget; unsigned int ampNo;
  //QWidget *parent;
  QMenuBar *menuBar;
  QAction *loadRealAction,*saveRealAction,*saveAvgsAction,
          *toggleFrameAction,*toggleGridAction,*toggleDigAction,
          *toggleParamAction,*toggleRealAction,
          *toggleGizmoAction,*toggleAvgsAction,
          *toggleScalpAction,*toggleSkullAction,*toggleBrainAction,
          *toggleSourceAction;
  QTabWidget *mainTabWidget; QWidget *cntWidget;
  QButtonGroup *cntAmpBG;
  QVector<QPushButton*> cntAmpButtons;

  QLabel *paramRLabel,*notchLabel,*timePtLabel;
  QCheckBox *gizmoRealCB,*elecRealCB; QString dummyString;
  QListWidget *gizmoList,*electrodeList; LegendFrame *legendFrame;

  QButtonGroup *avgAmpBG; QVector<QPushButton*> avgAmpButtons;
  QPushButton *clrAvgsButton;

};

#endif
