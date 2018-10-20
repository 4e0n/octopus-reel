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

#include "octopus_loc_master.h"
#include "octopus_head_glwidget.h"
#include "legendframe.h"

class HeadWindow : public QMainWindow {
 Q_OBJECT
 public:
  HeadWindow(QWidget *pnt,LocMaster* lm) : QMainWindow(pnt,Qt::Window) {
   parent=pnt; p=lm; setGeometry(p->hwX,p->hwY,p->hwWidth,p->hwHeight);
   setFixedSize(p->hwWidth,p->hwHeight);

   p->regRepaintHeadWindow(this);

   // *** STATUSBAR ***

   p->hwStatusBar=new QStatusBar(this);
   p->hwStatusBar->setGeometry(0,height()-20,width(),20);
   p->hwStatusBar->show(); setStatusBar(p->hwStatusBar);

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

   loadRealAction->setStatusTip(
    "Load previously saved electrode coordinates..");
   saveRealAction->setStatusTip(
    "Save measured/real electrode coordinates..");

   connect(loadRealAction,SIGNAL(triggered()),this,SLOT(slotLoadReal()));
   connect(saveRealAction,SIGNAL(triggered()),this,SLOT(slotSaveReal()));

   modelMenu->addAction(loadRealAction);
   modelMenu->addAction(saveRealAction);

   // View Menu
   toggleFrameAction=new QAction("Cartesian Frame",this);
   toggleGridAction=new QAction("Cartesian Grid",this);
   toggleParamAction=new QAction("Parametric Coords",this);
   toggleRealAction=new QAction("Measured/Real Coords",this);
   toggleGizmoAction=new QAction("Gizmos",this);
   toggleScalpAction=new QAction("MRI/Real Scalp Model",this);

   toggleFrameAction->setStatusTip(
    "Show/hide cartesian XYZ frame.");
   toggleGridAction->setStatusTip(
    "Show/hide cartesian grid/rulers.");
   toggleParamAction->setStatusTip(
    "Show/hide parametric/spherical coords/template.");
   toggleRealAction->setStatusTip(
    "Show/hide measured/real coords.");
   toggleGizmoAction->setStatusTip(
    "Show/hide loaded gizmo/hint list.");
   toggleScalpAction->setStatusTip(
    "Show/hide realistic scalp segmented from MRI data.");

   connect(toggleFrameAction,SIGNAL(triggered()),this,SLOT(slotToggleFrame()));
   connect(toggleGridAction,SIGNAL(triggered()),this,SLOT(slotToggleGrid()));
   connect(toggleParamAction,SIGNAL(triggered()),this,SLOT(slotToggleParam()));
   connect(toggleRealAction,SIGNAL(triggered()),this,SLOT(slotToggleReal()));
   connect(toggleGizmoAction,SIGNAL(triggered()),this,SLOT(slotToggleGizmo()));
   connect(toggleScalpAction,SIGNAL(triggered()),this,SLOT(slotToggleScalp()));

   viewMenu->addAction(toggleFrameAction);
   viewMenu->addAction(toggleGridAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleParamAction);
   viewMenu->addAction(toggleRealAction);
   viewMenu->addSeparator();
   viewMenu->addAction(toggleGizmoAction);
   viewMenu->addAction(toggleScalpAction);

   paramRLabel=new QLabel("Parametric Radius ("+
                          dummyString.setNum(p->scalpParamR)+" cm):",this);
   paramRLabel->setGeometry(p->hwHeight-20,40,p->hwWidth-p->hwHeight-10,20);

   QSlider *paramRSlider=new QSlider(Qt::Horizontal,this);
   paramRSlider->setGeometry(p->hwHeight-20,60,p->hwWidth-p->hwHeight-10,20);
   paramRSlider->setRange(70,500); // in mm because of int - divide by ten
   paramRSlider->setSingleStep(1);
   paramRSlider->setPageStep(10); // step in cms..
   paramRSlider->setSliderPosition(p->scalpParamR*10);
   paramRSlider->setEnabled(true);
   connect(paramRSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetScalpParamR(int)));

   QLabel *gizmoLabel=new QLabel("Gizmo",this);
   gizmoLabel->setGeometry(p->hwHeight+4,80,
                           (p->hwWidth-p->hwHeight)/2-10,20);
   QLabel *electrodeLabel=new QLabel("Electrode",this);
   electrodeLabel->setGeometry(p->hwHeight+(p->hwWidth-p->hwHeight)/2+14,80,
                               (p->hwWidth-p->hwHeight)/2-10,20);

   gizmoList=new QListWidget(this);
   gizmoList->setGeometry(p->hwHeight-20,100,
                          (p->hwWidth-p->hwHeight)/2-10,p->hwHeight-430);
   electrodeList=new QListWidget(this);
   electrodeList->setGeometry(p->hwHeight+(p->hwWidth-p->hwHeight)/2-20,100,
                              (p->hwWidth-p->hwHeight)/2-10,p->hwHeight-430);

   gizmoRealCB=new QCheckBox("Gizmo on Real Model",this);
   gizmoRealCB->setGeometry(p->hwHeight+4,p->hwHeight-286,
                            p->hwWidth-p->hwHeight-20,20);
   connect(gizmoRealCB,SIGNAL(clicked()),
           this,SLOT(slotGizmoRealCB()));

   connect(gizmoList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectGizmo(int)));
   connect(electrodeList,SIGNAL(currentRowChanged(int)),
           this,SLOT(slotSelectElectrode(int)));
   connect(electrodeList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),
           this,SLOT(slotViewElectrode(QListWidgetItem *)));

   for (int i=0;i<p->gizmo.size();i++) gizmoList->addItem(p->gizmo[i]->name);

   QLabel *legendLabel=new QLabel("Event Counts/Legend:",this);
   legendLabel->setGeometry(p->hwHeight-20,p->hwHeight-260,150,20);
   legendFrame=new LegendFrame(this,p);
   legendFrame->setGeometry(p->hwHeight-20,p->hwHeight-240,
                            p->hwWidth-p->hwHeight+10,200);

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
   p->avgAmpX=100;
   connect(avgAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotAvgAmp(int)));

   saveAvgsButton=new QPushButton("SAV",this);
   saveAvgsButton->setGeometry(480,height()-24,60,20);
   connect(saveAvgsButton,SIGNAL(clicked()),
           (QObject *)p,SLOT(slotExportAvgs()));

   clrAvgsButton=new QPushButton("CLR",this);
   clrAvgsButton->setGeometry(550,height()-24,60,20);
   connect(clrAvgsButton,SIGNAL(clicked()),(QObject *)p,SLOT(slotClrAvgs()));

   headGLWidget=new HeadGLWidget(this,p);

   gizmoList->setCurrentRow(0); slotSelectGizmo(0);
   setWindowTitle("Octopus-GUI - Head & Configuration Window");
  }

 private slots:
  void slotRepaint() {
   electrodeList->setCurrentRow(p->curElecInSeq); legendFrame->repaint();
  }

  void slotLoadReal() {
   QString fileName=QFileDialog::getOpenFileName(this,"Load Real Coordset File",
                                 ".","Object Files (*.orc)");
   if (!fileName.isEmpty()) { p->loadReal(fileName); }
   else { qDebug("An error has been occured while loading measured coords!"); }
  }

  void slotSaveReal() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Real Coordset File",
                                 ".","Object Files (*.orc)");
   if (!fileName.isEmpty()) { p->saveReal(fileName+".orc"); }
   else { qDebug("An error has been occured while saving measured coords!"); }
  }

  void slotToggleFrame()     { p->hwFrameV   =(p->hwFrameV)   ? false:true; }
  void slotToggleGrid()      { p->hwGridV    =(p->hwGridV)    ? false:true; }
  void slotToggleParam()     { p->hwParamV   =(p->hwParamV)   ? false:true; }
  void slotToggleReal()      { p->hwRealV    =(p->hwRealV)    ? false:true; }
  void slotToggleGizmo()     { p->hwGizmoV   =(p->hwGizmoV)   ? false:true; }
  void slotToggleScalp()     { p->hwScalpV   =(p->hwScalpV)   ? false:true; }

  void slotSetScalpParamR(int x) {
   p->scalpParamR=(float)(x)/10.;
   paramRLabel->setText("Parametric Radius ("+
                        dummyString.setNum(p->scalpParamR)+" cm):");
   headGLWidget->slotRepaintGL(2+8+16); // update real+gizmo+avgs
  }

  void slotSelectGizmo(int k) { int idx; Gizmo *g=p->gizmo[k];
   p->currentGizmo=k; electrodeList->clear();
   for (int i=0;i<g->seq.size();i++) {
    for (int j=0;j<p->acqChannels.size();j++)
     if (p->acqChannels[j]->physChn==g->seq[i]-1) { idx=j; break; }
    dummyString.setNum(p->acqChannels[idx]->physChn+1);
    electrodeList->addItem(dummyString+" "+p->acqChannels[idx]->name);
   }
   p->curElecInSeq=0;
   for (int j=0;j<p->acqChannels.size();j++)
    if (p->acqChannels[j]->physChn==g->seq[0]-1) { idx=j; break; }
   p->currentElectrode=idx;
   electrodeList->setCurrentRow(p->curElecInSeq);
   headGLWidget->slotRepaintGL(8);
  }

  void slotSelectElectrode(int k) { p->curElecInSeq=k;
   for (int i=0;i<p->acqChannels.size();i++)
    if (p->acqChannels[i]->physChn==p->gizmo[p->currentGizmo]->seq[k]-1)
     { p->currentElectrode=i; break; }
   headGLWidget->slotRepaintGL(2+16); // update real+avgs
  }

  void slotViewElectrode(QListWidgetItem *item) { int idx,pChn;
   QString s=item->text(); QStringList l=s.split(" "); pChn=l[0].toInt()-1;
   for (int i=0;i<p->acqChannels.size();i++)
    if (p->acqChannels[i]->physChn==pChn) { idx=i; break; }
   float theta=p->acqChannels[idx]->param.y;
   float phi=p->acqChannels[idx]->param.z;
   headGLWidget->setView(theta,phi);
  }

  void slotGizmoRealCB() {
   p->gizmoOnReal=gizmoRealCB->isChecked();
  }

  void slotAvgAmp(int x) {
   switch (x) {
    case 0: p->avgAmpX=  2.0; avgAmpBG->button(5)->setDown(false); break;
    case 1: p->avgAmpX=  5.0; avgAmpBG->button(5)->setDown(false); break;
    case 2: p->avgAmpX= 10.0; avgAmpBG->button(5)->setDown(false); break;
    case 3: p->avgAmpX= 20.0; avgAmpBG->button(5)->setDown(false); break;
    case 4: p->avgAmpX= 50.0; avgAmpBG->button(5)->setDown(false); break;
    case 5: p->avgAmpX=100.0; avgAmpBG->button(5)->setDown(false); break;
   }
  }

 private:
  LocMaster *p; QWidget *parent; HeadGLWidget *headGLWidget;

  QMenuBar *menuBar;
  QAction *loadRealAction,*saveRealAction,
          *toggleFrameAction,*toggleGridAction,
          *toggleParamAction,*toggleRealAction,
          *toggleGizmoAction,*toggleScalpAction;

  QLabel *paramRLabel; QCheckBox *gizmoRealCB; QString dummyString;
  QListWidget *gizmoList,*electrodeList; LegendFrame *legendFrame;

  QButtonGroup *avgAmpBG; QVector<QPushButton*> avgAmpButtons;
  QPushButton *saveAvgsButton,*clrAvgsButton;
};

#endif
