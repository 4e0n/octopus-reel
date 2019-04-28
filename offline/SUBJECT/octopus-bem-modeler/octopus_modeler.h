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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef OCTOPUS_MODELER_H
#define OCTOPUS_MODELER_H

#include <QtGui>
#include <QFileDialog>
#include "octopus_bem_master.h"
#include "segwidget.h"
#include "genparw.h"
#include "thresholdw.h"
#include "statsegw.h"
#include "reggroww.h"
#include "deformw.h"
#include "octopus_head_glwidget.h"

class BEMModeler : public QMainWindow {
 Q_OBJECT
 public:
  BEMModeler(BEMMaster *sm) : QMainWindow(0) {
   p=sm; w=p->guiWidth; h=p->guiHeight;
   setGeometry(p->guiX,p->guiY,w,h); setFixedSize(w,h);

   p->registerModeler((QObject *)this);

   genParW=new GenParW(p);
   thresholdW=new ThresholdW(p); statSegW=new StatSegW(p);
   regGrowW=new RegGrowW(p); deformW=new DeformW(p);

   // Status Bar
   statusBar=new QStatusBar(this); setStatusBar(statusBar);
   statusBar->show(); statusBar->showMessage("Welcome..");

   // Menu Bar
   menuBar=new QMenuBar(this); menuBar->setFixedWidth(w);
   setMenuBar(menuBar); menuBar->show();

   // Actions
   actionLoadSet_TIFFDir=new QAction("&Load MRI - TIFF Dir..",this);
   actionLoadSet_BinFile=new QAction("&Load MRI - Bin File..",this);
   actionSaveSet_TIFFDir=new QAction("&Save MRI - TIFF Dir..",this);
   actionSaveMesh_OCMFile=new QAction("&Save Mesh (.OCM File)..",this);
   actionAbout=new QAction("&About",this);
   actionQuit=new QAction("&Quit",this);

   actionToggleHist=new QAction("Show/hide Histogram",this);
   actionToggleFrame=new QAction("Show/hide Frame",this);
   actionToggleGrid=new QAction("Show/hide Grid",this);
   actionToggleField=new QAction("Show/hide Field",this);
   actionToggleModel=new QAction("Show/hide Model",this);
   actionToggleScalp=new QAction("Show/hide Scalp",this);
   actionToggleSkull=new QAction("Show/hide Skull",this);
   actionToggleBrain=new QAction("Show/hide Brain",this);

   actionProcess_DeformScalp=new QAction("&Start scalp deformation..",this);
   actionProcess_DeformSkull=new QAction("&Start skull deformation..",this);
   actionProcess_DeformBrain=new QAction("&Start brain deformation..",this);

   actionGenParS=new QAction("Generic Settings",this);
   actionThresholdS=new QAction("Thresholds & Filtering",this);
   actionStatSegS=new QAction("Statistical Segmentation",this);
   actionRegGrowS=new QAction("Region Growing",this);
   actionDeformS=new QAction("Deformable Models",this);

   actionLoadSet_TIFFDir->setStatusTip(
    "Loads TIFF MRI directory (i.e. exported from OsiriX)..");
   actionLoadSet_BinFile->setStatusTip(
    "Loads MRI Binary file in McGill BrainWeb format with extra .ini file");
   actionSaveSet_TIFFDir->setStatusTip(
    "Saves (source) buffer as TIFF MRI directory..");
   actionSaveMesh_OCMFile->setStatusTip(
    "Saves triangulated boundary surface set in Octopus .ocm format");
   actionAbout->setStatusTip("Who is behind this project..");
   actionQuit->setStatusTip("Quit to system, discarding all data..");

   actionToggleHist->setStatusTip("Show/hide volumetric histogram.");
   actionToggleFrame->setStatusTip("Show/hide reference axis.");
   actionToggleGrid->setStatusTip("Show/hide centimeters grid.");
   actionToggleField->setStatusTip("Show/hide the vector field.");
   actionToggleModel->setStatusTip("Show/hide the deformable model itself.");
   actionToggleScalp->setStatusTip("Show/hide scalp computed from model.");
   actionToggleSkull->setStatusTip("Show/hide skull computed from model.");
   actionToggleBrain->setStatusTip("Show/hide brain computed from model.");

   actionProcess_DeformScalp->setStatusTip(
    "Start scalp deformation using the GVF field in the field buffer..");
   actionProcess_DeformSkull->setStatusTip(
    "Start skull deformation using the GVF field in the field buffer..");
   actionProcess_DeformBrain->setStatusTip(
    "Start brain deformation using the GVF field in the field buffer..");

   actionGenParS->setStatusTip("Open/close Generic settings..");
   actionThresholdS->setStatusTip("Open/close Thresholding settings..");
   actionStatSegS->setStatusTip("Open/close Stat. Segmentation settings..");
   actionRegGrowS->setStatusTip("Open/close Floodfilling settings..");
   actionDeformS->setStatusTip("Open/close Deformable Model settings..");

   connect(actionLoadSet_TIFFDir,SIGNAL(triggered()),
           this,SLOT(slotLoadSet_TIFFDir()));
   connect(actionLoadSet_BinFile,SIGNAL(triggered()),
           this,SLOT(slotLoadSet_BinFile()));
   connect(actionSaveSet_TIFFDir,SIGNAL(triggered()),
           this,SLOT(slotSaveSet_TIFFDir()));
   connect(actionSaveMesh_OCMFile,SIGNAL(triggered()),
           this,SLOT(slotSaveMesh_OCMFile()));
   connect(actionAbout,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(actionQuit,SIGNAL(triggered()),p,SLOT(slotQuit()));

   connect(actionToggleHist,SIGNAL(triggered()),this,SLOT(slotToggleHist()));
   connect(actionToggleFrame,SIGNAL(triggered()),this,SLOT(slotToggleFrame()));
   connect(actionToggleGrid,SIGNAL(triggered()),this,SLOT(slotToggleGrid()));
   connect(actionToggleField,SIGNAL(triggered()),this,SLOT(slotToggleField()));
   connect(actionToggleModel,SIGNAL(triggered()),this,SLOT(slotToggleModel()));
   connect(actionToggleScalp,SIGNAL(triggered()),this,SLOT(slotToggleScalp()));
   connect(actionToggleSkull,SIGNAL(triggered()),this,SLOT(slotToggleSkull()));
   connect(actionToggleBrain,SIGNAL(triggered()),this,SLOT(slotToggleBrain()));

   connect(actionProcess_DeformScalp,SIGNAL(triggered()),
           this,SLOT(slotStart_DeformScalp()));
   connect(actionProcess_DeformSkull,SIGNAL(triggered()),
           this,SLOT(slotStart_DeformSkull()));
   connect(actionProcess_DeformBrain,SIGNAL(triggered()),
           this,SLOT(slotStart_DeformBrain()));

   connect(actionGenParS,SIGNAL(triggered()),this,SLOT(slotGenParW()));
   connect(actionThresholdS,SIGNAL(triggered()),this,SLOT(slotThrW()));
   connect(actionStatSegS,SIGNAL(triggered()),this,SLOT(slotStatSegW()));
   connect(actionRegGrowS,SIGNAL(triggered()),this,SLOT(slotRegGrowW()));
   connect(actionDeformS,SIGNAL(triggered()),this,SLOT(slotDeformW()));

   QMenu *fileMenu=new QMenu("&File",menuBar);
   fileMenu->addAction(actionLoadSet_TIFFDir);
   fileMenu->addAction(actionLoadSet_BinFile);
   fileMenu->addSeparator();
   fileMenu->addAction(actionSaveSet_TIFFDir);
   fileMenu->addSeparator();
   fileMenu->addAction(actionSaveMesh_OCMFile);
   fileMenu->addSeparator();
   fileMenu->addAction(actionAbout);
   fileMenu->addSeparator();
   fileMenu->addAction(actionQuit);
   menuBar->addMenu(fileMenu);

   QMenu *viewMenu=new QMenu("&View",menuBar);
   viewMenu->addAction(actionToggleHist);
   viewMenu->addSeparator();
   viewMenu->addAction(actionToggleFrame);
   viewMenu->addAction(actionToggleGrid);
   viewMenu->addSeparator();
   viewMenu->addAction(actionToggleField);
   viewMenu->addAction(actionToggleModel);
   viewMenu->addSeparator();
   viewMenu->addAction(actionToggleScalp);
   viewMenu->addAction(actionToggleSkull);
   viewMenu->addAction(actionToggleBrain);
   menuBar->addMenu(viewMenu);

   QMenu *processMenu=new QMenu("&Process",menuBar);
   processMenu->addAction(actionProcess_DeformScalp);
   processMenu->addAction(actionProcess_DeformSkull);
   processMenu->addAction(actionProcess_DeformBrain);
   menuBar->addMenu(processMenu);

   QMenu *settingsMenu=new QMenu("&Settings",menuBar);
   settingsMenu->addAction(actionGenParS);
   settingsMenu->addAction(actionThresholdS);
   settingsMenu->addAction(actionStatSegS);
   settingsMenu->addAction(actionRegGrowS);
   settingsMenu->addAction(actionDeformS);
   menuBar->addMenu(settingsMenu);

   // Main Tabs
   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(0,36,w,p->guiHeight-60);

   segWidget=new SegWidget(this,p);
   headGLWidget=new HeadGLWidget(this,p);

   mainTabWidget->addTab(segWidget,"Segmentation");
   mainTabWidget->addTab(headGLWidget,"3D Boundary");
   mainTabWidget->show();

   setWindowTitle(
    "Octopus-0.9.5 - Volumetric MR Segmentation/Head (BEM) Modeling Tool");
  }

 public slots:
  void slotShowMsg(QString s,int t) { statusBar->showMessage(s,t); }
  void slotLoaded()    { segWidget->loadUpdate(); }
  void slotProcessed() { segWidget->processUpdate();
                         headGLWidget->slotRepaintGL(1+2+4+8); }
  void slotPreview()   { segWidget->preview(); }

 private slots:
  void slotLoadSet_TIFFDir() {
   QStringList list,filter,fileNames;
   QFileDialog dialog(this);
   dialog.setFileMode(QFileDialog::DirectoryOnly);
   dialog.setViewMode(QFileDialog::List);
   if (dialog.exec()) {
    list=dialog.selectedFiles();
    QDir dir(list[0]); filter << "*.tif";
    fileNames=dir.entryList(filter);
    if (fileNames.size()>0) { p->fileIOName=list[0]; p->loadSet_TIFFDir(); }
    else {
     statusBar->showMessage(
      "No TIFF series exist in the selected directory!",0);
    }
   }
  }

  void slotSaveSet_TIFFDir() {
   QStringList list; QFileDialog dialog(this);
   dialog.setFileMode(QFileDialog::DirectoryOnly);
   dialog.setViewMode(QFileDialog::List);
   if (dialog.exec()) {
    list=dialog.selectedFiles(); QDir dir(list[0]);
    p->fileIOName=list[0]; p->saveSet_TIFFDir();
   }
  }

  void slotLoadSet_BinFile() {
   QString fileName=QFileDialog::getOpenFileName(this,"Open binary File",
                                          ".","RAW Binary Files (*.rawb)");
   if (!fileName.isEmpty()) { fileName.chop(5); // remove .rawb extension..
    p->fileIOName=fileName; p->loadSet_BinFile();
   }
  }

  void slotSaveMesh_OCMFile() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Mesh Object File",
                                          ".","Object Files (*.obf)");
   if (!fileName.isEmpty()) {
    p->fileIOName=fileName; p->saveMesh_OCMFile();
   }
  }

  void slotStart_DeformScalp() { p->processDeformScalp_Start(); }
  void slotStart_DeformSkull() { p->processDeformSkull_Start(); }
  void slotStart_DeformBrain() { p->processDeformBrain_Start(); }

  void slotGenParW() {
   if (genParW->isVisible()) genParW->hide(); else genParW->show();
  }
  void slotThrW() {
   if (thresholdW->isVisible()) thresholdW->hide(); else thresholdW->show();
  }
  void slotStatSegW() {
   if (statSegW->isVisible()) statSegW->hide(); else statSegW->show();
  }
  void slotRegGrowW() {
   if (regGrowW->isVisible()) regGrowW->hide(); else regGrowW->show();
  }
  void slotDeformW() {
   if (deformW->isVisible()) deformW->hide(); else deformW->show();
  }

  void slotToggleHist()  { p->histogramV = p->histogramV ? false : true; }
  void slotToggleFrame() { p->glFrameV   = p->glFrameV   ? false : true; }
  void slotToggleGrid()  { p->glGridV    = p->glGridV    ? false : true; }
  void slotToggleField() { p->glFieldV   = p->glFieldV   ? false : true; }
  void slotToggleModel() { p->glModelV   = p->glModelV   ? false : true; }
  void slotToggleScalp() { p->glScalpV   = p->glScalpV   ? false : true; }
  void slotToggleSkull() { p->glSkullV   = p->glSkullV   ? false : true; }
  void slotToggleBrain() { p->glBrainV   = p->glBrainV   ? false : true; }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus Modeler..",
                           "Octopus Modeler v0.9.5\n"
                           "(c) GNU Copyleft 2007-2009 Barkin Ilhan\n"
                           "Hacettepe University Biophysics Department\n"
                           "barkin@turk.net");
  }

 private:
  BEMMaster *p; int w,h;
  QStatusBar *statusBar; QMenuBar *menuBar; QTabWidget *mainTabWidget;
  QAction *actionLoadSet_TIFFDir,*actionLoadSet_BinFile,
          *actionSaveSet_TIFFDir,*actionSaveMesh_OCMFile,
          *actionAbout,*actionQuit,
          *actionProcess_DeformScalp,*actionProcess_DeformSkull,
          *actionProcess_DeformBrain,*actionToggleHist,*actionGenParS,
          *actionThresholdS,*actionStatSegS,*actionRegGrowS,*actionDeformS,
          *actionToggleFrame,*actionToggleGrid,
          *actionToggleField,*actionToggleModel,
          *actionToggleScalp,*actionToggleSkull,*actionToggleBrain;
  SegWidget *segWidget; HeadGLWidget *headGLWidget;
  GenParW *genParW; ThresholdW *thresholdW; StatSegW *statSegW;
  RegGrowW *regGrowW; DeformW *deformW;
};

#endif
