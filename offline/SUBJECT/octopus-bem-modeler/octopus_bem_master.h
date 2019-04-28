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

#ifndef OCTOPUS_BEM_MASTER_H
#define OCTOPUS_BEM_MASTER_H

#include <QObject>
#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QDir>
#include <QVector>
#include <QStatusBar>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <cmath>

#include "../../../common/mrivolume.h"
#include "../../../common/mrislice.h"
#include "../../../common/mesh.h"
#include "../../../common/vec3.h"
#include "../../../common/vol3.h"

//const int GUI_X=1224;
//const int GUI_Y=100;
const int GUI_FW=576;
const int GUI_FH=768;
const int GUI_X=124;
const int GUI_Y=50;
//const int GUI_FW=147*3;
//const int GUI_FH=196*3;

const int NUM_BUFFERS=5; // General purpose buffers
const int NUM_BOUNDS=3;	 // Scalp, skull, and brain mask and boundaries

const int MESH_RANK=5;

const int GVF_ITER=30;
const float GVF_MU=0.2;

const int DEF_ITER=1500;
const float DEF_ALPHA=0.6;
const float DEF_BETA=0.;
const float DEF_TAU=1.0;

const float MODEL_RX=9.0;
const float MODEL_RY=10.5;
const float MODEL_RZ=10.0;
//const float MODEL_RX=7.0;
//const float MODEL_RY=7.5;
//const float MODEL_RZ=7.0;

const float VOXEL_X=0.125;
const float VOXEL_Y=0.125;
const float VOXEL_Z=0.125;

const int LOADVOLUME_TIFFDIR = 0x01;
const int SAVEVOLUME_TIFFDIR = 0x02;
const int LOADVOLUME_BINFILE = 0x03;
const int SAVEMESH_OCMFILE   = 0x04;

const int COMPUTE_COPY        = 0x10;
const int COMPUTE_NORM        = 0x11;
const int COMPUTE_EDGE        = 0x12;
const int COMPUTE_THRESHOLD   = 0x13;
const int COMPUTE_FILTER      = 0x14;
const int COMPUTE_KMEANS      = 0x15;
const int COMPUTE_FLOODFILL   = 0x16;
const int COMPUTE_SEGSKULL    = 0x17;
const int COMPUTE_GVFFIELD    = 0x18;
const int COMPUTE_DEFORMSCALP = 0x19;
const int COMPUTE_DEFORMSKULL = 0x20;
const int COMPUTE_DEFORMBRAIN = 0x21;

const int MOUSEMODE_DRAW     = 0x01;
const int MOUSEMODE_POINT    = 0x02;
const int MOUSEMODE_SEED     = 0x03;

class BEMMaster : public QThread {
 Q_OBJECT
 public:
  BEMMaster(QApplication *app) : QThread() {
   application=app; fWidth=GUI_FW; fHeight=GUI_FH; xSize=ySize=zSize=0;
   guiX=GUI_X; guiY=GUI_Y; guiWidth=2*fWidth+26+68; guiHeight=fHeight+134;
   mrSetExists=processing=false; numBuffers=NUM_BUFFERS; numBounds=NUM_BOUNDS;
   buffer.resize(numBuffers); mask.resize(numBounds); bound.resize(numBounds);

   histogramV=glFrameV=glGridV=glFieldV=glScalpV=true;

   curBufferL=srcX=0; curBufferR=dstX=1; thrValue=100; f3dRadius=1;
   curFrame=0; brushSize=10; fldGran=10; preGC=false; setKernel3D();
   thrBin=thrFill=thrInv=previewMode=false; kMeansCCount=6; floodBoundary=3;
   seedPoint.resize(3); seedPoint[0]=seedPoint[1]=seedPoint[2]=0.;

   // Set 3D Edge Detection Kernel
   e3d.resize(27); for (int i=0;i<27;i++) e3d[i]=-1./27.; e3d[13]=26./27.;

   gvfIter=GVF_ITER; defIter=DEF_ITER;
   gvfMu=GVF_MU; defAlpha=DEF_ALPHA; defBeta=DEF_BETA; defTau=DEF_TAU;

   model=new Mesh(MESH_RANK); model->setRadius(MODEL_RX,MODEL_RY,MODEL_RZ);
   defScalp=new Mesh(model); defSkull=new Mesh(model); defBrain=new Mesh(model);

   mouseMode=MOUSEMODE_DRAW; pOffset=0; framePts.resize(3);

   voxelX=VOXEL_X; voxelY=VOXEL_Y; voxelZ=VOXEL_Z;

   // Fields
   kernel=new Vol3<float>(); kernel->setGaussian(2.0); // Wide
   potField=new Vol3<float>(); vecField=new Vol3<Vec3>();
  }

  // *** Utility routines ***
 
  void regRepaintGL(QObject *sh) {
   connect(this,SIGNAL(repaintGL(int)),sh,SLOT(slotRepaintGL(int)));
  }

  void registerModeler(QObject *s) { modeler=s; // Main GUI Events
   connect(this,SIGNAL(signalShowMsg(QString,int)),
           modeler,SLOT(slotShowMsg(QString,int)));
   connect(this,SIGNAL(signalLoaded()),modeler,SLOT(slotLoaded()));
   connect(this,SIGNAL(signalProcessed()),modeler,SLOT(slotProcessed()));
   connect(this,SIGNAL(signalPreview()),modeler,SLOT(slotPreview()));
  }

  void showMsg(QString s,int t) { emit signalShowMsg(s,t); }

  MRIVolume *findVol(int offset) {
   if (offset<numBuffers) return &(buffer[offset]);
   else if (offset>=numBuffers &&
            offset<(numBuffers+numBounds)) return &(mask[offset-numBuffers]);
   else if (offset>=(numBuffers+numBounds))
    return &(bound[offset-numBuffers-numBounds]);
   else return 0;
  }

  void bgStart(int job) {
   if (!processing) { cmd=job; processing=true; start(HighestPriority); }
   else showMsg("Please wait until I finish my current job!",4000);
  }

  // *** Background jobs ***

  void loadSet_TIFFDir()          { bgStart(LOADVOLUME_TIFFDIR ); }
  void saveSet_TIFFDir()          { bgStart(SAVEVOLUME_TIFFDIR ); }
  void loadSet_BinFile()          { bgStart(LOADVOLUME_BINFILE ); }
  void saveMesh_OCMFile()         { bgStart(SAVEMESH_OCMFILE   ); }
  void processNormStart()         { bgStart(COMPUTE_NORM       ); }
  void processCopyStart()         { bgStart(COMPUTE_COPY       ); }
  void processThrStart()          { bgStart(COMPUTE_THRESHOLD  ); }
  void processFltStart()          { bgStart(COMPUTE_FILTER     ); }
  void processEdgeStart()         { bgStart(COMPUTE_EDGE       ); }
  void processKMStart()           { bgStart(COMPUTE_KMEANS     ); }
  void processFillStart()         { bgStart(COMPUTE_FLOODFILL  ); }
  void processSkullStart()        { bgStart(COMPUTE_SEGSKULL   ); }
  void processComputeGVFField()   { bgStart(COMPUTE_GVFFIELD   ); }
  void processDeformScalp_Start() { bgStart(COMPUTE_DEFORMSCALP); }
  void processDeformSkull_Start() { bgStart(COMPUTE_DEFORMSKULL); }
  void processDeformBrain_Start() { bgStart(COMPUTE_DEFORMBRAIN); }

  // *** General purpose callback ***

  void setSeed() { showMsg("Please select seedpoint..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=-1; }
  void setNAS() { showMsg("Please select Nasion..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=0; }
  void setRPA() { showMsg("Please select Right Pre-Aurical Point..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=1; }
  void setLPA() { showMsg("Please select Left Pre-Aurical Point..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=2; }

  void clearSlice() {
   for (int i=0;i<numBuffers;i++) buffer[i].slice[curSlice]->clear();
   for (int i=0;i<numBounds;i++) mask[i].slice[curSlice]->clear();
   for (int i=0;i<numBounds;i++) bound[i].slice[curSlice]->clear();
   emit signalProcessed();
  }

  void setKernel3D() {
   // Construct moving average filter kernel of diameter 2*r+1
   int i,j,k,count; int n=2*f3dRadius+1; float r2; f3d.resize(0);
   for (count=0,k=0;k<n;k++) for (j=0;j<n;j++) for (i=0;i<n;i++) {
    r2=sqrt((i-f3dRadius)*(i-f3dRadius)+
            (j-f3dRadius)*(j-f3dRadius)+
            (k-f3dRadius)*(k-f3dRadius));
    if (r2<=f3dRadius) { f3d.append(1.); count++; } else f3d.append(0.);
   } for (i=0;i<f3d.size();i++) f3d[i]/=(float)(count);
  }

  // *** PREVIEWS ***

  void setThrValue(int x) { thrValue=x; MRIVolume *vol=findVol(srcX);
   if (previewMode) {
    vol->slice[curSlice]->threshold(&previewSlice,thrValue,
                          thrBin,thrFill,thrInv); emit signalPreview();
   }
  }

  void setFltValue(int r) { f3dRadius=r; setKernel3D();
   MRIVolume *vol=findVol(srcX);
   if (previewMode) {
    vol->slice[curSlice]->filter(&previewSlice,&f3d); emit signalPreview();
   }
  }

  void coRegSelect(QPoint m) { QString resultStr,px,py,pz;
   if (pOffset==-1) { // Seedpoint
    seedPoint[0]=m.x(); seedPoint[1]=m.y(); seedPoint[2]=curSlice;
    resultStr="Selected voxel is ("+
              px.setNum(seedPoint[0])+","+
              py.setNum(seedPoint[1])+","+
              pz.setNum(seedPoint[2])+").";
   } else { // Coregistration triangle
    float x=(float)(m.x()); float y=(float)(m.y()); float z=(float)(curSlice); 
    framePts[pOffset][0]=(x-(float)(xSize/2))*voxelX;
    framePts[pOffset][1]=(y-(float)(ySize/2))*voxelY;
    framePts[pOffset][2]=(z-(float)(zSize/2))*voxelZ;
    resultStr="Selected point is ("+
              px.setNum(framePts[pOffset][0])+","+
              py.setNum(framePts[pOffset][1])+","+
              pz.setNum(framePts[pOffset][2])+").";
   } showMsg(resultStr,0); mouseMode=MOUSEMODE_DRAW;
  }

  void copyBuffer(int src,int dst) { MRISlice *s,*d;
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   for (int i=0;i<zSize;i++) {
    s=sVol->slice[i]; d=dVol->slice[i]; d->data=s->data.copy();
   }
  }

  void copyBufferMasked(int src,MRIVolume* mask,int dst) { MRISlice *s,*d,*m;
   int i,j,k; MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   for (k=0;k<zSize;k++) {
    s=sVol->slice[k]; d=dVol->slice[k]; m=mask->slice[k];
    for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
     if (qGray(m->data.pixel(i,j))>0)
      d->data.setPixel(i,j,qGray(s->data.pixel(i,j)));
     else d->data.setPixel(i,j,0);
   }
  }

  void fill3D(MRIVolume *srcVol,MRIVolume *dstVol,
              QVector<int> *sp,int thr) { fillSrc=srcVol; fillDst=dstVol;
   for (int i=0;i<dstVol->slice.size();i++) dstVol->slice[i]->clear();
   stack=rewindCount=0; rewinding=rewinded=false; floodThr=thr;
   fillR3D((*sp)[0],(*sp)[1],(*sp)[2]);
  }

  // *** PUBLIC VARIABLES ***

  bool mrSetExists,previewMode,thrBin,thrFill,thrInv;

  int numBuffers,numBounds,
      fWidth,fHeight,guiX,guiY,guiWidth,guiHeight,xSize,ySize,zSize,
      curBufferL,curBufferR,curSlice,srcX,dstX,curFrame,thrValue,f3dRadius,
      mouseMode,pOffset,brushSize,kMeansCCount,floodBoundary;

  float gvfMu,defTau,defAlpha,defBeta; int gvfIter,defIter;

  Vol3<float> *kernel,*potField; Vol3<Vec3> *vecField;

  Mesh *model,*defScalp,*defSkull,*defBrain;

  QVector<Vec3> framePts; QVector<int> seedPoint;
  MRISlice previewSlice; QMutex mutex; int fillStack;
  QVector<MRIVolume> buffer,mask,bound;
  QVector<float> f3d,e3d; QString fileIOName;

  bool histogramV,glFrameV,glGridV,glModelV,glFieldV,
       glScalpV,glSkullV,glBrainV,preGC;

  int fldGran; float voxelX,voxelY,voxelZ;

 signals:

  void signalShowMsg(QString,int); void signalLoaded();
  void signalProcessed(); void signalPreview(); void repaintGL(int);

 public slots:

  void slotQuit() { application->quit(); }

 protected:
  void run() {
   switch (cmd) {
    case LOADVOLUME_TIFFDIR:  tLoadVolume_TIFFDir();        break;
    case SAVEVOLUME_TIFFDIR:  tSaveVolume_TIFFDir();        break;
    case LOADVOLUME_BINFILE:  tLoadVolume_BinFile();        break;
    case SAVEMESH_OCMFILE:    tSaveMesh_OCMFile();          break;
    case COMPUTE_COPY:        tCopyBuffer(srcX,dstX);       break;
    case COMPUTE_NORM:        tComputeNorm(srcX,dstX);      break;
    case COMPUTE_THRESHOLD:   tComputeThreshold(srcX,dstX); break;
    case COMPUTE_FILTER:      tComputeFilter(srcX,dstX);    break;
    case COMPUTE_EDGE:        tComputeEdge(srcX,dstX);      break;
    case COMPUTE_KMEANS:      tComputeKMeans(srcX,dstX);    break;
    case COMPUTE_FLOODFILL:   tComputeFloodFill(srcX,dstX); break;
    case COMPUTE_SEGSKULL:    tComputeSkull(srcX,dstX);     break;
    case COMPUTE_GVFFIELD:    tComputeGVFField(srcX,dstX);  break;
    case COMPUTE_DEFORMSCALP: tComputeDeform(0);            break;
    case COMPUTE_DEFORMSKULL: tComputeDeform(1);            break;
    case COMPUTE_DEFORMBRAIN: tComputeDeform(2);            break;
   }
  }

 private:
  void tLoadVolume_TIFFDir() { int i,j;
   QImage s1,s2; QStringList filter; QDir dir;
   QStringList fileNames;
   bool loadError; MRISlice *dummySlice; QVector<QRgb> gsTable;

   dir.setPath(fileIOName);
   filter << "*.tif";
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);

   for (i=0;i<256;i++) gsTable.append(qRgb(i,i,i));

   showMsg("Preloading set and checking for possible errors..",0);
   loadError=false; zSize=0; for (i=0;i<fileNames.size();i++) {
    s1.load(fileNames[i]);
    s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
    if (i==0) { xSize=s1.width(); ySize=s1.height(); }
    if (xSize==0 || ySize==0) {
     showMsg("ERROR: Null image!",0); loadError=true; break;
    }
    if (s1.width()!=xSize || s1.height()!=ySize) {
     showMsg("ERROR: Different size of slice encountered!!",0);
     loadError=true; break;
    } zSize++;
   } if (loadError) return;

   previewSlice=s2.copy(); mrSetExists=false;

   // Reset/truncate previously existing buffers..
   for (i=0;i<numBuffers;i++) {
    for (j=0;j<buffer[i].slice.size();j++) delete buffer[i].slice[j];
    buffer[i].slice.resize(0);
   }
   for (i=0;i<numBounds;i++) {
    for (j=0;j<mask[i].slice.size();j++) delete mask[i].slice[j];
    for (j=0;j<bound[i].slice.size();j++) delete bound[i].slice[j];
    mask[i].slice.resize(0); bound[i].slice.resize(0);
   }

   // Main loading loop..
   for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);

    for (j=0;j<numBuffers;j++) { // Fill all buffers with original set..
     dummySlice=new MRISlice(s2.copy()); buffer[j].append(dummySlice);
    }
    for (j=0;j<numBounds;j++) { // Fill all buffers with original set..
     dummySlice=new MRISlice(s2.copy()); mask[j].append(dummySlice);
     dummySlice=new MRISlice(s2.copy()); bound[j].append(dummySlice);
    }

    for (j=0;j<numBuffers;j++) {
     buffer[j].xSize=xSize; buffer[j].ySize=ySize; buffer[j].zSize=zSize;
    }
    for (j=0;j<numBounds;j++) {
     mask[j].xSize=xSize; mask[j].ySize=ySize; mask[j].zSize=zSize;
     bound[j].xSize=xSize; bound[j].ySize=ySize; bound[j].zSize=zSize;
    }

    showMsg("No error seems to exist. Now loading/allocating memory (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   }


   // Load scalp, skull, and brain masks if available
   dir.setPath(fileIOName+"/SCALPMASK");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Scalpmask found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     mask[0].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No scalpmask exists..",0); processing=false;
   }

   dir.setPath(fileIOName+"/SKULLMASK");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Skullmask found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     mask[1].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No skullmask exists..",0); processing=false;
   }

   dir.setPath(fileIOName+"/BRAINMASK");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Brainmask found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     mask[2].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No brainmask exists..",0); processing=false;
   }

   // Load scalp, skull, and brain boundaries if available
   dir.setPath(fileIOName+"/SCALPBOUND");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Scalp boundary found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     bound[0].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No scalp boundary exists, creating new one..",0); processing=false;
   }

   dir.setPath(fileIOName+"/SKULLBOUND");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Skull boundary found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     bound[1].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No skull boundary exists, creating new one..",0); processing=false;
   }

   dir.setPath(fileIOName+"/BRAINBOUND");
   fileNames=dir.entryList(filter); // Filenames
   for (i=0;i<fileNames.size();i++)
    fileNames[i]=dir.absoluteFilePath(fileNames[i]);
   if (fileNames.size()==zSize) {
    for (i=0;i<zSize;i++) { s1.load(fileNames[i]);
    showMsg("Brain boundary found.. loading it ("+
            dummyString.setNum(i*100/zSize)+")..",0);
     s2=s1.convertToFormat(QImage::Format_Indexed8,gsTable);
     bound[2].slice[i]->data=s2.copy();
    }
   } else {
    showMsg("No brain boundary exists, creating new one..",0); processing=false;
   }

   // Default seed point is center..
   seedPoint[0]=xSize/2; seedPoint[1]=ySize/2; seedPoint[2]=zSize/2;
   mrSetExists=true; emit signalLoaded();

   showMsg("Slices have been loaded successfully..",4000); processing=false;
  }

  void tSaveVolume_TIFFDir() { QString s; QStringList filter; QDir dir;
   MRISlice *dummySlice; QStringList fileNames;

   dir.setPath(fileIOName);
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=buffer[0].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving slices (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/SCALPMASK"); dir.setPath(fileIOName+"/SCALPMASK");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=mask[0].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving scalpmask (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/SKULLMASK"); dir.setPath(fileIOName+"/SKULLMASK");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=mask[1].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving skullmask (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/BRAINMASK"); dir.setPath(fileIOName+"/BRAINMASK");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=mask[2].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving brainmask (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/SCALPBOUND");dir.setPath(fileIOName+"/SCALPBOUND");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=bound[0].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving scalp boundary (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/SKULLBOUND");dir.setPath(fileIOName+"/SKULLBOUND");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=bound[1].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving skull boundary (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }

   dir.mkpath(fileIOName+"/BRAINBOUND");dir.setPath(fileIOName+"/BRAINBOUND");
   fileNames.clear();
   for (int i=0;i<zSize;i++) {
    if (i<10) s="00"; else if (i<100) s="0"; else if (i<1000) s="";
    fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
   }
   for (int i=0;i<zSize;i++) {
    dummySlice=bound[2].slice[i];
    dummySlice->data.save(fileNames[i]);
    showMsg("Saving brain boundary (%"+dummyString.setNum(i*100/zSize)+")..",0);
   }
   showMsg("Slices have been saved successfully..",0); processing=false;
  }

  void tLoadVolume_BinFile() { QFile file;
   QStringList fileNames; fileNames.append(fileIOName);
   file.setFileName(fileNames[0]+".info"); file.open(QIODevice::ReadOnly);
   QTextStream infoStream(&file);
   infoStream >> xSize; infoStream >> ySize; infoStream >> zSize; file.close();

   file.setFileName(fileNames[0]+".rawb"); file.open(QIODevice::ReadOnly);
   QDataStream rawbStream(&file);

   QVector<quint8> rawbData; quint8 x;
   while (!file.atEnd()) { rawbStream >> x; rawbData.append(x); }
   
   rawbData.resize(0); mrSetExists=true; emit signalLoaded();
   showMsg("Slices have been loaded successfully..",4000);
   processing=false;
  }

  void tSaveMesh_OCMFile() { QString s;
   QStringList fileNames; fileNames.append(fileIOName);
   QFile scalpFile; scalpFile.setFileName(fileNames[0]+".obj");
   scalpFile.open(QIODevice::WriteOnly); QTextStream scalpStream(&scalpFile);

   scalpStream << "# OBJ Generated by Octopus BEM Modeler v0.9.5\n\n";
//   scalpStream << "# Vertex count = " << scalpCoord.size() << "\n";
//   scalpStream << "# Face count = " << scalpIndex.size() << "\n";
   scalpStream << "\n# VERTICES\n\n"; 

   scalpStream << "c ";
   for (int i=0;i<framePts.size();i++)
    scalpStream << framePts[i][0] << " "
                << framePts[i][1] << " "
                << framePts[i][2] << " "; scalpStream << "\n\n";

//   for (int i=0;i<scalpCoord.size();i++)
//    scalpStream << "v " << scalpCoord[i].x << " "
//                        << scalpCoord[i].y << " "
//                        << scalpCoord[i].z << "\n";

   scalpStream << "\n# FACES\n\n"; 

//   for (int i=0;i<scalpIndex.size();i++)
//    scalpStream << "f " << scalpIndex[i][0] << " "
//                        << scalpIndex[i][1] << " "
//                        << scalpIndex[i][2] << "\n";

   scalpFile.close(); showMsg("Scalp mesh has been saved successfully..",4000);
   processing=false;
  }

  void tComputeNorm(int src,int dst) { MRIVolume *dVol=findVol(dst);
   copyBuffer(src,dst); dVol->normalize(); emit signalProcessed();
   showMsg("Normalization is finished..",0); processing=false;
  }
   
  void tCopyBuffer(int src,int dst) { MRIVolume *dVol=findVol(dst);
   copyBuffer(src,dst); dVol->update(); emit signalProcessed();
   showMsg("Copying is finished..",0); processing=false;
  }

  void tComputeThreshold(int src,int dst) { MRIVolume *dVol=findVol(dst);
   copyBuffer(src,dst); dVol->threshold(thrValue,thrBin,thrFill,thrInv);
   emit signalProcessed();
   showMsg("Threshold computation is finished..",0); processing=false;
  }

  void tComputeFilter(int src,int dst) {
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   filter3D(sVol,dVol); emit signalProcessed();
   showMsg("Filtering is finished..",0); processing=false;
  }

  void tComputeEdge(int src,int dst) {
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   edge3D(sVol,dVol); emit signalProcessed();
   showMsg("Edge detection is finished..",0); processing=false;
  }
 
  void tComputeKMeans(int src,int dst) {
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   kMeans3D(sVol,dVol); emit signalProcessed();
   showMsg("Clustering is finished..",0); processing=false;
  }

  void tComputeFloodFill(int src,int dst) {
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   copyBuffer(src,dst);
   fill3D(sVol,dVol,&seedPoint,floodBoundary);

   // Fill3D

   emit signalProcessed();
   showMsg("Floodfilling is finished..",0); processing=false;
  }

  void tComputeSkull(int src,int dst) {
   int i,j,k,w,h,d; QImage *srcImage,*dstImage,*mskImage,*brnImage;
   if (mask[0].slice.size()) { copyBuffer(src,dst);
    w=buffer[dst].xSize; h=buffer[dst].ySize; d=buffer[dst].zSize;
    for (k=0;k<d;k++) for (j=0;j<h;j++) for (i=0;i<w;i++) {
     srcImage=&(buffer[src].slice[k]->data);
     dstImage=&(buffer[dst].slice[k]->data);
     mskImage=&(mask[0].slice[k]->data);
     brnImage=&(mask[2].slice[k]->data);
     if (qGray(mskImage->pixel(i,j))==255 &&
         qGray(brnImage->pixel(i,j))==0)
      dstImage->setPixel(i,j,qGray(srcImage->pixel(i,j)));
     else dstImage->setPixel(i,j,0);
    }
   } emit signalProcessed();
   showMsg("Masking of skull is finished..",0); processing=false;
  }

  void tComputeGVFField(int src,int dst) {
   MRIVolume *sVol=findVol(src); MRIVolume *dVol=findVol(dst);
   potField->setFromMRI(sVol);
   if (preGC) convolve(potField,kernel); // Pre-convolution with Gaussian
   potField->normalize(); computeGVF(vecField,potField,gvfMu,gvfIter,dVol);
   processing=false; showMsg("Potential Field computation is finished..",0);
  }

  void tComputeDeform(int x) { Vec3 v0;
   switch (x) {
    case 0: deform(vecField,defScalp,defAlpha,defBeta,defTau,defIter); break;
    case 1: deform(vecField,defSkull,defAlpha,defBeta,defTau,defIter); break;
    case 2: deform(vecField,defBrain,defAlpha,defBeta,defTau,defIter); break;
   }
   processing=false; showMsg("Deformation is finished..",0);
  }

  Vec3 lrc(Vec3 v0,Vec3 v1,Vec3 v2,Vec3 v3) { // Local Radius of Curvature
   Vec3 n,c,s,sn,st; float rMin=0.5,rMax=3.,lm,r,ff,ee,f2;
   n=(Vec3::cross(v1-v0,v2-v0)+
      Vec3::cross(v2-v0,v3-v0)+
      Vec3::cross(v3-v0,v1-v0)); n.normalize();
   c=(v1+v2+v3)*(1./3.); s=c-v0; sn=(s*n)*n; st=s-sn;

   lm=((v0-v1).norm()+(v0-v2).norm()+(v0-v3).norm())/3.;
   r=(lm*lm)/(2.*sn.norm());

   ff=6./(1./rMin-1./rMax); ee=0.5*(1./rMin+1./rMax);
   f2=0.5*(1.+tanh(ff*(1./r-ee)));
   
   return 0.5*(0.5*st+f2*sn);
  }

  void deform(Vol3<Vec3> *vf,Mesh *dst,
              float alpha,float beta,float tau,int iter) {
   int count,i; Vec3 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,del,del2,res; SNeigh *ss;
   Vec3 offset=Vec3((float)(vf->xSize/2)*voxelX,
                    (float)(vf->ySize/2)*voxelY,
                    (float)(vf->zSize/2)*voxelZ);
   int vx,vy,vz; // offset in mesh
   Mesh *src=new Mesh(model); dst->takeOver(src);

   for (count=0;count<iter;count++) {
    for (i=0;i<src->sv.count();i++) { ss=&(src->sn[i]); v0=src->sv[i].r+offset;
     v1=src->sv[ss->v[0]].r+offset;
     v2=src->sv[ss->v[1]].r+offset;
     v3=src->sv[ss->v[2]].r+offset;
     v4=src->sv[src->sn[ss->v[0]].v[0]].r+offset;
     v5=src->sv[src->sn[ss->v[0]].v[1]].r+offset;
     v6=src->sv[src->sn[ss->v[1]].v[0]].r+offset;
     v7=src->sv[src->sn[ss->v[1]].v[1]].r+offset;
     v8=src->sv[src->sn[ss->v[2]].v[0]].r+offset;
     v9=src->sv[src->sn[ss->v[2]].v[1]].r+offset;
     del=v0.del(v1,v2,v3); del2=v0.del2(v1,v2,v3,v4,v5,v6,v7,v8,v9);
//     vx=(int)((offset[0]-src->sv[i].r[0])*(1./voxelX)); // Rot180 around z
//     vy=(int)((offset[1]-src->sv[i].r[1])*(1./voxelY));
//     vz=(int)((src->sv[i].r[2]+offset[2])*(1./voxelZ));
     vx=(int)(v0[0]*(1./voxelX));
     vy=(int)(v0[1]*(1./voxelY));
     vz=(int)(v0[2]*(1./voxelZ));
//     printf("%d %d %d\n",vx,vy,vz);
     res=v0+tau*(alpha*del-beta*del2+vf->data[vz][vy][vx]+lrc(v0,v1,v2,v3));
     // Check bounds..
     if (res[0]<voxelX) res[0]=voxelX;
     if (res[1]<voxelY) res[1]=voxelY;
     if (res[2]<voxelZ) res[2]=voxelZ;
     if (res[0]>2.*offset[0]-2.*voxelX) res[0]=2.*offset[0]-2.*voxelX;
     if (res[1]>2.*offset[1]-2.*voxelY) res[1]=2.*offset[1]-2.*voxelY;
     if (res[2]>2.*offset[2]-2.*voxelZ) res[2]=2.*offset[2]-2.*voxelZ;
//     printf("%2.2f %2.2f %2.2f\n",res[0],res[1],res[2]);
     dst->sv[i].r=res;
    }
    mutex.lock();
     for (int i=0;i<src->sv.size();i++) {
      dst->sv[i].r=dst->sv[i].r-offset; src->sv[i].r=dst->sv[i].r;
     }
     dst->updateFromSimplex();
    mutex.unlock(); emit repaintGL(1+2+4+8+16);
    showMsg("Deformable Model Iteration #"+dummyString.setNum(count),0);
   } delete src;
  }

  void computeGVF(Vol3<Vec3> *fld,Vol3<float> *vol,float mu,int iter,
                  MRIVolume *dVol) {
   int i,j,k,count,xSize,ySize,zSize; float dX,dY,dZ;
   fld->resize(vol->xSize-1,vol->ySize-1,vol->zSize-1);
   xSize=fld->xSize; ySize=fld->ySize; zSize=fld->zSize;

   Vol3<Vec3> f(xSize,ySize,zSize); Vol3<Vec3> c(xSize,ySize,zSize);
   Vol3<Vec3> u(xSize,ySize,zSize); Vol3<Vec3> lu(xSize,ySize,zSize);
   Vol3<float> g(xSize,ySize,zSize); Vol3<float> b(xSize,ySize,zSize);

   // Edge map (vol) is expected to be already normalized..

   // Compute edge map gradient
   //  Neumann boundary condition: zero normal derivative at boundary

   f.initNeumann(vol);

   // Compute initial parameters
   for (k=0;k<zSize-1;k++) for (j=0;j<ySize-1;j++) for (i=0;i<xSize-1;i++) {
    dX=u.data[k][j][i][0]=f.data[k][j][i][0]; // Initial GVF Vector
    dY=u.data[k][j][i][1]=f.data[k][j][i][1];
    dZ=u.data[k][j][i][2]=f.data[k][j][i][2];

    g.data[k][j][i]=mu; b.data[k][j][i]=dX*dX+dY*dY+dZ*dZ; // Sq. of grad.mag.

    c.data[k][j][i]=
     Vec3(b.data[k][j][i]*dX,b.data[k][j][i]*dY,b.data[k][j][i]*dZ);
   }

   // Iterative GVF Solution 
   for (count=0;count<iter;count++) { // ++ update GVF ++
    lu.updateNeumann(&u); u.updateGVF(&b,&g,&c,&u,&lu);
    showMsg("GVF Iteration #"+dummyString.setNum(count),0);
    potField->getX(&u); potField->normalize();
    potField->setToMRI(dVol);
    // Copy the intermediate/result over our own..
    mutex.lock();
     for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
      fld->data[k][j][i]=u.data[k][j][i];
    mutex.unlock();
    emit signalProcessed(); emit repaintGL(1+2+4+8+16); // Reconstruct field..
   }
  }

  void convolve(Vol3<float> *krn,Vol3<float> *vol) {
   int x,y,z,i,j,k,X,Y,Z,n; float sum;
   if (krn->xSize==krn->ySize && krn->ySize==krn->zSize && krn->xSize%2) {
    n=krn->xSize/2;
    for (z=0;z<vol->zSize;z++) {
     for (y=0;y<vol->ySize;y++) for (x=0;x<vol->xSize;x++) {
      for (sum=0.,k=-n;k<=n;k++) for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) {
       Z=z+k; Y=y+j; X=x+i;
       if (X>=0 && Y>=0 && Z>=0 &&
           X<vol->xSize && Y<vol->ySize && Z<vol->zSize) {
        sum += vol->data[Z][Y][X] * krn->data[k+n][j+n][i+n];
       } vol->data[z][y][x]=sum;
      }
     } showMsg("Convolving ("+dummyString.setNum(100*z/vol->zSize)+"%).",0);
    }
   } else showMsg("ERROR: Destination not suitable for convolution!",0);
  }

  void filter3D(MRIVolume *src,MRIVolume *dst) { // Filter using f3d kernel
   int x,y,z,i,j,k,w,h,d,X,Y,Z,t,n,idx; QImage *srcImage,*dstImage;
   n=(int)(pow((float)(f3d.size()),1./3.))/2;
   w=dst->xSize; h=dst->ySize; d=dst->zSize;
   for (z=0;z<d;z++) for (y=0;y<h;y++) for (x=0;x<w;x++) {
    t=idx=0; dstImage=&(dst->slice[z]->data);
    for (k=-n;k<=n;k++) for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) {
     Z=z+k; Y=y+j; X=x+i;
     if (X>=0 && Y>=0 && Z>=0 && X<w && Y<h && Z<d) {
      srcImage=&(src->slice[Z]->data);
      t+=(int)((float)(qGray(srcImage->pixel(X,Y)))*f3d[idx]);
     } idx++;
    } if (t<=0 && t>255) qDebug("%d",t); dstImage->setPixel(x,y,(quint8)(t));
    showMsg("Filtering.. ("+dummyString.setNum(100*z/d)+"%).",0);
   }
  }

  void edge3D(MRIVolume *src,MRIVolume *dst) { // Detect Edges using e3d kernel
   int x,y,z,i,j,k,w,h,d,X,Y,Z,t,n,idx; QImage *srcImage,*dstImage;
   n=(int)(pow((float)(e3d.size()),1./3.))/2;
   w=dst->xSize; h=dst->ySize; d=dst->zSize;
   for (z=0;z<d;z++) for (y=0;y<h;y++) for (x=0;x<w;x++) {
    t=idx=0; dstImage=&(dst->slice[z]->data);
    for (k=-n;k<=n;k++) for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) {
     Z=z+k; Y=y+j; X=x+i;
     if (X>=0 && Y>=0 && Z>=0 && X<w && Y<h && Z<d) {
      srcImage=&(src->slice[Z]->data);
      t+=(int)((float)(qGray(srcImage->pixel(X,Y)))*e3d[idx]);
     } idx++;
    } if (t<=0 && t>255) qDebug("%d",t); dstImage->setPixel(x,y,(quint8)(t));
    showMsg("Detecting edges.. ("+dummyString.setNum(100*z/d)+"%).",0);
   }
  }

  void kMeans3D(MRIVolume*src,MRIVolume* dst) {
   // kMeans Seg with homogeneous initial centroids..
   QImage *srcImage,*dstImage,*maskImage;
   QVector<quint8> map,prvCent,curCent,deltas;
   QVector<int> sums,counts; quint8 offset=0,pix; bool iteration=true;
   int iter=0,maxd,current,i,j,k,m,w,h,d,pixMin=255,pixMax=0;
   w=dst->xSize; h=dst->ySize; d=dst->zSize; map.resize(w*h*d);

   // Construct centroids homogeneously within (mix,max) grayscale range
   if (mask[0].slice.size()) {
    for (k=0;k<d;k++) {
     srcImage=&(src->slice[k]->data);
     maskImage=&(mask[0].slice[k]->data);
     for (j=0;j<h;j++) for (i=0;i<w;i++) {
      if (qGray(maskImage->pixel(i,j))==255) {
       if (qGray(srcImage->pixel(i,j))<pixMin)
        pixMin=qGray(srcImage->pixel(i,j));
       if (qGray(srcImage->pixel(i,j))>pixMax)
        pixMax=qGray(srcImage->pixel(i,j));
      }
     }
    }
   } else {
    for (k=0;k<d;k++) { srcImage=&(src->slice[k]->data);
     for (j=0;j<h;j++) for (i=0;i<w;i++) {
      if (qGray(srcImage->pixel(i,j))<pixMin)
       pixMin=qGray(srcImage->pixel(i,j));
      if (qGray(srcImage->pixel(i,j))>pixMax)
       pixMax=qGray(srcImage->pixel(i,j));
     }
    }
   }

   prvCent.resize(kMeansCCount); curCent.resize(kMeansCCount);
   sums.resize(kMeansCCount); counts.resize(kMeansCCount);
   quint8 delta=(int)((float)(pixMax-pixMin)/(float)(curCent.size()+1));
   for (i=0;i<curCent.size();i++) curCent[i]=pixMin+delta*(quint8)(i+1); 

   // Main iteration
   while (iteration) {
    // Assign current pixels to nearest centroid..
    if (mask[0].slice.size()) {
     for (k=0;k<d;k++) {
      srcImage=&(src->slice[k]->data);
      maskImage=&(mask[0].slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) {
       if (qGray(maskImage->pixel(i,j))==255) {
        pix=qGray(srcImage->pixel(i,j)); deltas.resize(0);
        for (m=0;m<curCent.size();m++) deltas.append(abs(pix-curCent[m]));
        maxd=1000;
        for (m=0;m<curCent.size();m++) { // Take the smallest one..
         if (deltas[m]<maxd) { maxd=deltas[m]; offset=m; }
        } map[k*h*w+j*w+i]=offset;
       }
      }
     }
    } else {
     for (k=0;k<d;k++) { srcImage=&(src->slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) {
       pix=qGray(srcImage->pixel(i,j)); deltas.resize(0);
       for (m=0;m<curCent.size();m++) deltas.append(abs(pix-curCent[m]));
       maxd=1000;
       for (m=0;m<curCent.size();m++) { // Take the smallest one..
        if (deltas[m]<maxd) { maxd=deltas[m]; offset=m; }
       } map[k*h*w+j*w+i]=offset;
      }
     }
    }

    for (i=0;i<prvCent.size();i++) prvCent[i]=curCent[i]; // Copy to prev..

    // Compute updated ones..
    for (i=0;i<curCent.size();i++) sums[i]=counts[i]=0;

    if (mask[0].slice.size()) {
     for (k=0;k<d;k++) {
      srcImage=&(src->slice[k]->data);
      maskImage=&(mask[0].slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) {
       if (qGray(maskImage->pixel(i,j))==255) {
        current=map[k*h*w+j*w+i]; pix=qGray(srcImage->pixel(i,j));
        sums[current]+=pix; counts[current]++;
       }
      }
     }
    } else {
     for (k=0;k<d;k++) { srcImage=&(src->slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) {
       current=map[k*h*w+j*w+i]; pix=qGray(srcImage->pixel(i,j));
       sums[current]+=pix; counts[current]++;
      }
     }
    }

    for (i=0;i<curCent.size();i++)
     if (counts[i]>0) curCent[i]=sums[i]/counts[i];

    iteration=false;
    for (i=0;i<curCent.size();i++)
     if (curCent[i]!=prvCent[i]) { iteration=true; break; }

    if (mask[0].slice.size()) {
     for (k=0;k<d;k++) {
      dstImage=&(dst->slice[k]->data);
      maskImage=&(mask[0].slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) { // Set final slice
       // Maximized centroid
       if (qGray(maskImage->pixel(i,j))==255) {
        pix=curCent[map[k*h*w+j*w+i]]; dstImage->setPixel(i,j,pix);
       }
      }
     }
    } else {
     for (k=0;k<d;k++) { dstImage=&(dst->slice[k]->data);
      for (j=0;j<h;j++) for (i=0;i<w;i++) { // Set final slice
       // Maximized centroid
       pix=curCent[map[k*h*w+j*w+i]]; dstImage->setPixel(i,j,pix);
      }
     }
    } emit signalProcessed();

    showMsg("K-Means Iteration ("+dummyString.setNum(iter)+").",0);
    iter++;
   }
  }

  void fillR3D(int xx,int yy,int zz) {
   int x,y,z,col,mark; stack++;
   if (!rewinding && stack<STK_DEPTH) { // Not grown so much.. no problem yet.
    x=xx; y=yy; z=zz;
hh: for (int k=-1;k<=1;k++) for (int j=-1;j<=1;j++) for (int i=-1;i<=1;i++) {
     if ((x+i)>1 && (y+j)>1 && (z+k)>1 && // .. clipping
         (x+i)<(xSize-2) && (y+j)<(ySize-2) && (z+k)<(zSize-2)) {
      col=qGray(fillSrc->slice[z+k]->data.pixel(x+i,y+j));
      mark=qGray(fillDst->slice[z+k]->data.pixel(x+i,y+j));
      if (col>=floodThr && mark!=255) { // Premark && Color/gray level suits?
       fillDst->slice[z+k]->data.setPixel(x+i,y+j,255);
//       qDebug("Filled pixel %d %d %d",x+i,y+j,z+k);
       fillR3D(x+i,y+j,z+k);

       if (rewinded) { rewinding=rewinded=false; x=xRec; y=yRec; z=zRec;
        qDebug("Rewinded.. S=%d  %d,%d,%d",stack,x,y,z); goto hh;
       } else if (rewinding) {
        if (stack==2) { rewinding=false; rewinded=true; } stack--; return;
       }
      }
     }
    } // for
   } else { // Danger of stack overflow, needs rewinding..
    qDebug("Stack deepness=60000.. Rewind#%d started..",rewindCount);
    rewindCount++; xRec=xx; yRec=yy; zRec=zz; rewinding=true; rewinded=false;
   } stack--;
  }

  QApplication *application; QObject *modeler; QPainter kp; QImage ki;
  QString dummyString; bool processing; int cmd;

  MRIVolume *fillSrc,*fillDst;
  int floodThr,stack,rewindCount,xRec,yRec,zRec;
  bool rewinding,rewinded;
};

#endif
