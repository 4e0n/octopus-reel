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

#ifndef HEADWINDOW_H
#define HEADWINDOW_H

#include <QtGui>

#include "octopus_acq_master.h"
#include "octopus_head_glwidget.h"
#include "legendframe.h"

class HeadWindow : public QMainWindow {
 Q_OBJECT
 public:
  HeadWindow(QWidget *p,AcqMaster* acqm) : QMainWindow(p,Qt::Window) {
   parent=p; acqM=acqm; setGeometry(acqM->hwX,acqM->hwY,acqM->hwWidth,acqM->hwHeight);
   //setFixedSize(acqM->hwWidth,acqM->hwHeight);

   acqM->regRepaintHeadWindow(this);

   // *** STATUSBAR ***

   acqM->hwStatusBar=new QStatusBar(this);
   acqM->hwStatusBar->setGeometry(0,height()-20,width(),20);
   acqM->hwStatusBar->show(); setStatusBar(acqM->hwStatusBar);

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   setMenuBar(menuBar);
   QMenu *modelMenu=new QMenu("&Model",menuBar);
   QMenu *viewMenu=new QMenu("&View",menuBar);
   menuBar->addMenu(modelMenu);
   menuBar->addMenu(viewMenu);

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

   paramRLabel=new QLabel("Parametric Radius ("+
                          dummyString.setNum(acqM->scalpParamR)+" cm):",this);
   paramRLabel->setGeometry(acqM->hwHeight-20,40,acqM->hwWidth-acqM->hwHeight-10,20);

   QSlider *paramRSlider=new QSlider(Qt::Horizontal,this);
   paramRSlider->setGeometry(acqM->hwHeight-20,60,acqM->hwWidth-acqM->hwHeight-10,20);
   paramRSlider->setRange(70,500); // in mm because of int - divide by ten
   paramRSlider->setSingleStep(1);
   paramRSlider->setPageStep(10); // step in cms..
   paramRSlider->setSliderPosition(acqM->scalpParamR*10);
   paramRSlider->setEnabled(true);
   connect(paramRSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetScalpParamR(int)));

   QLabel *gizmoLabel=new QLabel("Gizmo",this);
   gizmoLabel->setGeometry(acqM->hwHeight+4,80,
                           (acqM->hwWidth-acqM->hwHeight)/2-10,20);
   QLabel *electrodeLabel=new QLabel("Electrode",this);
   electrodeLabel->setGeometry(acqM->hwHeight+(acqM->hwWidth-acqM->hwHeight)/2+14,80,
                               (acqM->hwWidth-acqM->hwHeight)/2-10,20);

   gizmoList=new QListWidget(this);
   gizmoList->setGeometry(acqM->hwHeight-20,100,
                          (acqM->hwWidth-acqM->hwHeight)/2-10,acqM->hwHeight-430);
   electrodeList=new QListWidget(this);
   electrodeList->setGeometry(acqM->hwHeight+(acqM->hwWidth-acqM->hwHeight)/2-20,100,
                              (acqM->hwWidth-acqM->hwHeight)/2-10,acqM->hwHeight-430);

   notchLabel=new QLabel("Notch Level (RMS) ("+
                    dummyString.setNum(acqM->notchThreshold)+" uV):",this);
   notchLabel->setGeometry(acqM->hwHeight-20,acqM->hwHeight-326,
                           acqM->hwWidth-acqM->hwHeight-10,20);

   QSlider *notchSlider=new QSlider(Qt::Horizontal,this);
   notchSlider->setGeometry(acqM->hwHeight-20,acqM->hwHeight-306,
                            acqM->hwWidth-acqM->hwHeight-10,20);
   notchSlider->setRange(1,100); // in mm because of int - divide by ten
   notchSlider->setSingleStep(1);
   notchSlider->setPageStep(10); // step in uVs..
   notchSlider->setSliderPosition((int)(acqM->notchThreshold*1.));
   notchSlider->setEnabled(true);
   connect(notchSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetNotchThr(int)));

   gizmoRealCB=new QCheckBox("Gizmo on Real Model",this);
   gizmoRealCB->setGeometry(acqM->hwHeight+4,acqM->hwHeight-286,
                            acqM->hwWidth-acqM->hwHeight-20,20);
   connect(gizmoRealCB,SIGNAL(clicked()),this,SLOT(slotGizmoRealCB()));

   elecRealCB=new QCheckBox("Electrodes on Real Model",this);
   elecRealCB->setGeometry(acqM->hwHeight+4,acqM->hwHeight-266,
                            acqM->hwWidth-acqM->hwHeight-20,20);
   connect(elecRealCB,SIGNAL(clicked()),this,SLOT(slotElecRealCB()));

   connect(gizmoList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectGizmo(int)));
   connect(electrodeList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectElectrode(int)));
   connect(electrodeList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),
           this,SLOT(slotViewElectrode(QListWidgetItem *)));

   for (int i=0;i<acqM->gizmo.size();i++) gizmoList->addItem(acqM->gizmo[i]->name);

   QLabel *legendLabel=new QLabel("Event Counts/Legend:",this);
   legendLabel->setGeometry(acqM->hwHeight-20,acqM->hwHeight-240,170,20);
   legendFrame=new LegendFrame(this,acqM);
   legendFrame->setGeometry(acqM->hwHeight-20,acqM->hwHeight-220,
                            acqM->hwWidth-acqM->hwHeight+10,120);

   timePtLabel=new QLabel("Localization Time Point ("+dummyString.setNum(
                          acqM->cp.avgBwd+acqM->slTimePt*1000/acqM->sampleRate
                           )+" ms):",this);
   timePtLabel->setGeometry(acqM->hwHeight-20,acqM->hwHeight-80,
                            acqM->hwWidth-acqM->hwHeight-10,20);

   QSlider *timePtSlider=new QSlider(Qt::Horizontal,this);
   timePtSlider->setGeometry(acqM->hwHeight-20,acqM->hwHeight-60,
                             acqM->hwWidth-acqM->hwHeight-10,20);
   timePtSlider->setRange(0,acqM->cp.avgCount-1);
   timePtSlider->setSingleStep(1);
   timePtSlider->setPageStep(1);
   timePtSlider->setValue(acqM->slTimePt);
   timePtSlider->setEnabled(true);
   connect(timePtSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetTimePt(int)));

   QPushButton *dummyButton;
   avgAmpBG=new QButtonGroup();
   avgAmpBG->setExclusive(true);
   for (int i=0;i<6;i++) { // ERP AMP
    dummyButton=new QPushButton(this); dummyButton->setCheckable(true);
    dummyButton->setGeometry(20+i*60,height()-24,60,20);
    avgAmpBG->addButton(dummyButton,i);
   }
   avgAmpBG->button(0)->setText("2uV");
   avgAmpBG->button(1)->setText("5uV");
   avgAmpBG->button(2)->setText("10uV");
   avgAmpBG->button(3)->setText("20uV");
   avgAmpBG->button(4)->setText("50uV");
   avgAmpBG->button(5)->setText("100uV");
   avgAmpBG->button(5)->setDown(true);
   acqM->avgAmpX=100;
   connect(avgAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotAvgAmp(int)));


   clrAvgsButton=new QPushButton("CLR",this);
   clrAvgsButton->setGeometry(550,height()-24,60,20);
   connect(clrAvgsButton,SIGNAL(clicked()),(QObject *)acqM,SLOT(slotClrAvgs()));

   headGLWidget=new HeadGLWidget(this,acqM);

   //if (gizmoList->count()>0) {
   // gizmoList->setCurrentRow(0); slotSelectGizmo(0);
   //}
 
   setWindowTitle("Octopus-GUI - Head & Configuration Window");
  }

 private slots:
  void slotRepaint() {
   electrodeList->setCurrentRow(acqM->curElecInSeq); legendFrame->repaint();
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

  void slotToggleFrame()     { acqM->hwFrameV   =(acqM->hwFrameV)   ? false:true; }
  void slotToggleGrid()      { acqM->hwGridV    =(acqM->hwGridV)    ? false:true; }
  void slotToggleDig()       { acqM->hwDigV     =(acqM->hwDigV)     ? false:true; }
  void slotToggleParam()     { acqM->hwParamV   =(acqM->hwParamV)   ? false:true; }
  void slotToggleReal()      { acqM->hwRealV    =(acqM->hwRealV)    ? false:true; }
  void slotToggleGizmo()     { acqM->hwGizmoV   =(acqM->hwGizmoV)   ? false:true; }
  void slotToggleAvgs()      { acqM->hwAvgsV    =(acqM->hwAvgsV)    ? false:true; }
  void slotToggleScalp()     { acqM->hwScalpV   =(acqM->hwScalpV)   ? false:true; }
  void slotToggleSkull()     { acqM->hwSkullV   =(acqM->hwSkullV)   ? false:true; }
  void slotToggleBrain()     { acqM->hwBrainV   =(acqM->hwBrainV)   ? false:true; }
  void slotToggleSource()    { acqM->hwSourceV  =(acqM->hwSourceV)  ? false:true; }

  void slotSetScalpParamR(int x) {
   acqM->scalpParamR=(float)(x)/10.;
   paramRLabel->setText("Parametric Radius ("+
                        dummyString.setNum(acqM->scalpParamR)+" cm):");
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
   acqM->currentGizmo=k; electrodeList->clear();
   for (int i=0;i<g->seq.size();i++) {
    for (int j=0;j<acqM->acqChannels.size();j++)
     if (acqM->acqChannels[j]->physChn==g->seq[i]-1) { idx=j; break; }
    dummyString.setNum(acqM->acqChannels[idx]->physChn+1);
    electrodeList->addItem(dummyString+" "+acqM->acqChannels[idx]->name);
   }
   acqM->curElecInSeq=0;
   for (int j=0;j<acqM->acqChannels.size();j++)
    if (acqM->acqChannels[j]->physChn==g->seq[0]-1) { idx=j; break; }
   acqM->currentElectrode=idx;
   electrodeList->setCurrentRow(acqM->curElecInSeq);
   headGLWidget->slotRepaintGL(8);
  }

  void slotSelectElectrode(int k) { acqM->curElecInSeq=k;
   for (int i=0;i<acqM->acqChannels.size();i++)
    if (acqM->acqChannels[i]->physChn==acqM->gizmo[acqM->currentGizmo]->seq[k]-1)
     { acqM->currentElectrode=i; break; }
   headGLWidget->slotRepaintGL(2+16); // update real+avgs
  }

  void slotViewElectrode(QListWidgetItem *item) { int idx,pChn;
   QString s=item->text(); QStringList l=s.split(" "); pChn=l[0].toInt()-1;
   for (int i=0;i<acqM->acqChannels.size();i++)
    if (acqM->acqChannels[i]->physChn==pChn) { idx=i; break; }
   float theta=acqM->acqChannels[idx]->param.y;
   float phi=acqM->acqChannels[idx]->param.z;
   headGLWidget->setView(theta,phi);
  }

  void slotGizmoRealCB() {
   acqM->gizmoOnReal=gizmoRealCB->isChecked();
   headGLWidget->slotRepaintGL(8); // update gizmo
  }

  void slotElecRealCB() {
   acqM->elecOnReal=elecRealCB->isChecked();
   headGLWidget->slotRepaintGL(16); // update averages
  }

  void slotAvgAmp(int x) {
   switch (x) {
    case 0: acqM->avgAmpX=  2.0; avgAmpBG->button(5)->setDown(false); break;
    case 1: acqM->avgAmpX=  5.0; avgAmpBG->button(5)->setDown(false); break;
    case 2: acqM->avgAmpX= 10.0; avgAmpBG->button(5)->setDown(false); break;
    case 3: acqM->avgAmpX= 20.0; avgAmpBG->button(5)->setDown(false); break;
    case 4: acqM->avgAmpX= 50.0; avgAmpBG->button(5)->setDown(false); break;
    case 5: acqM->avgAmpX=100.0; avgAmpBG->button(5)->setDown(false); break;
   }
  }
/*
 protected:
  void resizeEvent(QResizeEvent *event) {
    resizeEvent(event); // Call base class event handler

    //int width = event->size().width();
    //int height = event->size().height();

    // Resize child widgets proportionally
    //label1->setGeometry(width / 4, height / 4, width / 2, 30);
    //label2->setGeometry(width / 4, height / 2, width / 2, 30);
  }
*/

 private:
  AcqMaster *acqM; QWidget *parent; HeadGLWidget *headGLWidget;

  QMenuBar *menuBar;
  QAction *loadRealAction,*saveRealAction,*saveAvgsAction,
          *toggleFrameAction,*toggleGridAction,*toggleDigAction,
          *toggleParamAction,*toggleRealAction,
          *toggleGizmoAction,*toggleAvgsAction,
          *toggleScalpAction,*toggleSkullAction,*toggleBrainAction,
          *toggleSourceAction;

  QLabel *paramRLabel,*notchLabel,*timePtLabel;
  QCheckBox *gizmoRealCB,*elecRealCB; QString dummyString;
  QListWidget *gizmoList,*electrodeList; LegendFrame *legendFrame;

  QButtonGroup *avgAmpBG; QVector<QPushButton*> avgAmpButtons;
  QPushButton *clrAvgsButton;
};

#endif
