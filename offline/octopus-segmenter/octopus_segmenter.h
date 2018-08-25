/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef SEGMENTER_H
#define SEGMENTER_H

#include <QtGui>
#include <QFileDialog>
#include "octopus_seg_master.h"
#include "segwidget.h"
#include "thresholdtb.h"
#include "kmeanstb.h"
#include "octopus_head_glwidget.h"

class Segmenter : public QMainWindow {
 Q_OBJECT
 public:
  Segmenter(SegMaster *sm) : QMainWindow(0) {
   p=sm; w=p->guiWidth; h=p->guiHeight;
   setGeometry(p->guiX,p->guiY,w,h); setFixedSize(w,h);

   p->registerSegmenter((QObject *)this);

   thresholdTB=new ThresholdTB(p);
   kMeansTB=new KMeansTB(p);

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
   actionSaveScalp_ObjFile=new QAction("&Save Scalp Mesh..",this);
   actionSaveSkull_ObjFile=new QAction("&Save Skull Mesh..",this);
   actionSaveBrain_ObjFile=new QAction("&Save Brain Mesh..",this);
   actionLoadThrs=new QAction("&Load Thresholds..",this);
   actionSaveThrs=new QAction("&Save Thresholds..",this);
   actionQuit=new QAction("&Quit",this);
   actionSHistToggle=new QAction("Slice histogram",this);
   actionVHistToggle=new QAction("Volume histogram",this);
   actionThresholdTB=new QAction("Thresholding",this);
   actionKMeansTB=new QAction("K-Means",this);
   actionAbout=new QAction("&About",this);

   actionLoadSet_TIFFDir->setStatusTip(
    "Loads TIFF MRI directory saved from OsiriX..");
   actionLoadSet_BinFile->setStatusTip(
    "Loads MRI Binary file in McGill BrainWeb format with extra .ini file");
   actionSaveSet_TIFFDir->setStatusTip(
    "Saves (source) volume as TIFF MRI directory..");
   actionSaveScalp_ObjFile->setStatusTip(
    "Saves computed scalp mesh in .obj format");
   actionSaveSkull_ObjFile->setStatusTip(
    "Saves computed skull mesh in .obj format");
   actionSaveBrain_ObjFile->setStatusTip(
    "Saves computed brain mesh in .obj format");
   actionLoadThrs->setStatusTip(
    "Loads slice threshold set (.thr file)");
   actionSaveThrs->setStatusTip(
    "Saves slice threshold set (.thr file)");
   actionQuit->setStatusTip("Quit to system, discarding all data..");
   actionSHistToggle->setStatusTip("Slice histogram on/off.");
   actionVHistToggle->setStatusTip("Volume histogram on/off.");
   actionThresholdTB->setStatusTip("Open/close Thresholding Toolbox.");
   actionKMeansTB->setStatusTip("Open/close K-Means Clustering Toolbox.");
   actionAbout->setStatusTip("Who is behind this project..");

   connect(actionLoadSet_TIFFDir,SIGNAL(triggered()),
           this,SLOT(slotLoadSet_TIFFDir()));
   connect(actionLoadSet_BinFile,SIGNAL(triggered()),
           this,SLOT(slotLoadSet_BinFile()));
   connect(actionSaveSet_TIFFDir,SIGNAL(triggered()),
           this,SLOT(slotSaveSet_TIFFDir()));
   connect(actionSaveScalp_ObjFile,SIGNAL(triggered()),
           this,SLOT(slotSaveScalp_ObjFile()));
   connect(actionSaveSkull_ObjFile,SIGNAL(triggered()),
           this,SLOT(slotSaveSkull_ObjFile()));
   connect(actionSaveBrain_ObjFile,SIGNAL(triggered()),
           this,SLOT(slotSaveBrain_ObjFile()));
   connect(actionLoadThrs,SIGNAL(triggered()),
           this,SLOT(slotLoadThrs()));
   connect(actionSaveThrs,SIGNAL(triggered()),
           this,SLOT(slotSaveThrs()));
   connect(actionQuit,SIGNAL(triggered()),p,SLOT(slotQuit()));
   connect(actionSHistToggle,SIGNAL(triggered()),this,SLOT(slotSHistToggle()));
   connect(actionThresholdTB,SIGNAL(triggered()),this,SLOT(slotThrTB()));
   connect(actionKMeansTB,SIGNAL(triggered()),this,SLOT(slotKMeansTB()));
   connect(actionAbout,SIGNAL(triggered()),this,SLOT(slotAbout()));

   QMenu *fileMenu=new QMenu("&File",menuBar);
   fileMenu->addAction(actionLoadSet_TIFFDir);
   fileMenu->addAction(actionLoadSet_BinFile);
   fileMenu->addSeparator();
   fileMenu->addAction(actionSaveSet_TIFFDir);
   fileMenu->addSeparator();
   fileMenu->addAction(actionSaveScalp_ObjFile);
   fileMenu->addAction(actionSaveSkull_ObjFile);
   fileMenu->addAction(actionSaveBrain_ObjFile);
   fileMenu->addSeparator();
   fileMenu->addAction(actionLoadThrs);
   fileMenu->addAction(actionSaveThrs);
   fileMenu->addSeparator();
   fileMenu->addAction(actionQuit);
   menuBar->addMenu(fileMenu);
   QMenu *viewMenu=new QMenu("&View",menuBar);
   viewMenu->addAction(actionSHistToggle);
   viewMenu->addAction(actionVHistToggle);
   menuBar->addMenu(viewMenu);
   QMenu *tbMenu=new QMenu("&Toolbox",menuBar);
   tbMenu->addAction(actionThresholdTB);
   tbMenu->addAction(actionKMeansTB);
   menuBar->addMenu(tbMenu);
   QMenu *helpMenu=new QMenu("&Help",menuBar);
   helpMenu->addAction(actionAbout);
   menuBar->addMenu(helpMenu);

   // Main Tabs
   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(0,36,w,p->fHeight+58);

   segWidget=new SegWidget(this,p);
   headGLWidget=new HeadGLWidget(this,p);

   mainTabWidget->addTab(segWidget,"Segmentation");
   mainTabWidget->addTab(headGLWidget,"3D Boundary");
   mainTabWidget->show();

   setWindowTitle(
    "Octopus-0.9.5 - Volumetric MR Segmentation/Tissue Classification Tool");
  }

 public slots:
  void slotShowMsg(QString s,int t) { statusBar->showMessage(s,t); }
  void slotLoaded()    { segWidget->loadUpdate(); }
  void slotProcessed() { segWidget->processUpdate();
                         headGLWidget->repaintGL(); }
  void slotPreview()   { segWidget->preview(); }

 private slots:
  void slotLoadSet_TIFFDir() {
   QStringList list,filter,fileNames;
   QFileDialog dialog(this);
   dialog.setFileMode(QFileDialog::DirectoryOnly);
   dialog.setViewMode(QFileDialog::List);
   if (dialog.exec()) {
    list=dialog.selectedFiles();
    QDir dir(list[0]);
    filter << "*.jpg"; filter << "*.tif";
    fileNames=dir.entryList(filter);
    if (fileNames.size()>0) { p->loadSet_TIFFDir(list[0]); }
    else {
     statusBar->showMessage(
      "No TIFF series exist in the selected directory!",4000);
    }
   }
  }

  void slotSaveSet_TIFFDir() {
   QStringList list; QFileDialog dialog(this);
   dialog.setFileMode(QFileDialog::DirectoryOnly);
   dialog.setViewMode(QFileDialog::List);
   if (dialog.exec()) {
    list=dialog.selectedFiles(); QDir dir(list[0]);
    p->saveSet_TIFFDir(list[0]);
   }
  }

  void slotLoadSet_BinFile() {
   QString fileName=QFileDialog::getOpenFileName(this,"Open binary File",
                                          ".","RAW Binary Files (*.rawb)");
   if (!fileName.isEmpty()) { fileName.chop(5); // remove .rawb extension..
    p->loadSet_BinFile(fileName);
   }
  }

  void slotSaveScalp_ObjFile() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Mesh Object File",
                                          ".","Object Files (*.obj)");
   if (!fileName.isEmpty()) p->saveScalp_ObjFile(fileName);
  }
  void slotSaveSkull_ObjFile() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Mesh Object File",
                                          ".","Object Files (*.obj)");
   if (!fileName.isEmpty()) p->saveSkull_ObjFile(fileName);
  }
  void slotSaveBrain_ObjFile() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save Mesh Object File",
                                          ".","Object Files (*.obj)");
   if (!fileName.isEmpty()) p->saveBrain_ObjFile(fileName);
  }

  void slotLoadThrs() {
   QString xs; QStringList xsl;
   QString fileName=QFileDialog::getOpenFileName(this,"Open threshold File",
                                          ".","Text Files (*.thr)");
   if (!fileName.isEmpty()) {
    QFile thrFile(fileName); thrFile.open(QIODevice::ReadOnly);
    QTextStream thrStream(&thrFile); xs=thrStream.readAll(); xsl=xs.split(",");
    if (xsl.size()==p->sliceThrs.size()) {
     for (int i=0;i<p->sliceThrs.size();i++) p->sliceThrs[i]=xsl[i].toInt();
    } else {
     statusBar->showMessage(
      "ERROR: Threshold file does not match slice size!",4000);
    }
    thrFile.close();
   }
  }

  void slotSaveThrs() {
   QString fileName=QFileDialog::getSaveFileName(this,"Save threshold File",
                                          ".","Text Files (*.thr)");
   if (!fileName.isEmpty()) {
    QFile thrFile(fileName); thrFile.open(QIODevice::WriteOnly);
    QTextStream thrStream(&thrFile);
    for (int i=0;i<p->sliceThrs.size();i++) {
     thrStream << p->sliceThrs[i];
     if (i<(p->sliceThrs.size()-1)) thrStream << ",";
    } thrStream << "\n";
    statusBar->showMessage(
     "Threshold file has been saved..",4000);
   }
  }

//  void slotSaveSelected() {
//   QString dirName=QFileDialog::getSaveFileName(this,"Save Segmented TIFF Set",
//                                         ".","");
//   if (!dirName.isEmpty()) {
//    p->saveSegmented(dirName);
//   }
//  }

  void slotSaveMesh() {
//    QString fileName=QFileDialog::getSaveFileName(this,"Save 3D Boundary",
//                                         ".","");
//    if (!fileName.isEmpty()) {
//     hv->saveBoundary(fileName);
//    }
  }

  void slotSHistToggle() {}
  void slotVHistToggle() {}

  void slotThrTB() {
   if (thresholdTB->isVisible()) thresholdTB->hide(); else thresholdTB->show();
  }
  void slotKMeansTB() {
   if (kMeansTB->isVisible()) kMeansTB->hide(); else kMeansTB->show();
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus Segmenter..",
                           "Octopus Segmenter v0.9.5\n"
                           "(c) GNU Copyleft 2007-2009 Barkin Ilhan\n"
                           "Hacettepe University Biophysics Department\n"
                           "barkin@turk.net");
  }

 private:
  SegMaster *p; int w,h;
  QStatusBar *statusBar; QMenuBar *menuBar; QTabWidget *mainTabWidget;
  QAction *actionLoadSet_TIFFDir,*actionLoadSet_BinFile,
          *actionSaveSet_TIFFDir,
          *actionSaveScalp_ObjFile,
          *actionSaveSkull_ObjFile,
          *actionSaveBrain_ObjFile,
          *actionLoadThrs,*actionSaveThrs,
          *actionSaveSelected,*actionSaveMesh,*actionQuit,
          *actionSHistToggle,*actionVHistToggle,
          *actionThresholdTB,*actionKMeansTB,
          *actionAbout;
  SegWidget *segWidget; HeadGLWidget *headGLWidget;
  ThresholdTB *thresholdTB;
  KMeansTB *kMeansTB;
};

#endif
