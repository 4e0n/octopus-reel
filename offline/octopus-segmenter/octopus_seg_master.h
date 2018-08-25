/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_SEG_MASTER_H
#define OCTOPUS_SEG_MASTER_H

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

#include "octopus_mri_volume.h"
#include "octopus_mri_slice.h"
#include "coord3d.h"

const int NUM_BUFFERS=5;

const int GUI_X=1024;
const int GUI_Y=0;
const int GUI_FW=576;
const int GUI_FH=768;

//const int GUI_X=0;
//const int GUI_Y=0;
//const int GUI_FW=3*576/4;
//const int GUI_FH=3*768/4;

const int LOADVOLUME_TIFFDIR = 0x01;
const int SAVEVOLUME_TIFFDIR = 0x02;
const int LOADVOLUME_BINFILE = 0x03;
const int SAVESCALP_OBJFILE  = 0x04;
const int SAVESKULL_OBJFILE  = 0x05;
const int SAVEBRAIN_OBJFILE  = 0x06;
const int COMPUTE_COPY       = 0x10;
const int COMPUTE_NORM       = 0x11;
const int COMPUTE_EDGE       = 0x12;
const int COMPUTE_THRESHOLD  = 0x13;
const int COMPUTE_FILTER     = 0x14;
const int COMPUTE_KMEANS     = 0x15;
const int COMPUTE_SCALP_SURF = 0x21;
const int COMPUTE_SKULL_SURF = 0x22;
const int COMPUTE_BRAIN_SURF = 0x23;
const int COMPUTE_SURFACE4   = 0x24;

const int MOUSEMODE_DRAW     = 0x01;
const int MOUSEMODE_POINT    = 0x02;
const int MOUSEMODE_SEED     = 0x03;

class SegMaster : public QThread {
 Q_OBJECT
 public:
  SegMaster(QApplication *app) : QThread() {
   application=app; fWidth=GUI_FW; fHeight=GUI_FH; xSize=ySize=zSize=0;
   guiX=GUI_X; guiY=GUI_Y;
   guiWidth=2*fWidth+26+28; guiHeight=fHeight+124; mrSetExists=processing=false;
   numBuffers=NUM_BUFFERS; volume.resize(numBuffers);

   curVolumeL=0; curVolumeR=1; srcX=curVolumeL; dstX=curVolumeR; curFrame=0;

   thrValue=100; fltValue=2;
   thrBin=thrFill=thrInv=thrPS=previewMode=false;

   brushSize=10;

   mouseMode=MOUSEMODE_DRAW; pOffset=0;
   framePts.resize(3);

   // Construct moving average kernel of diameter 2*x+1
   int count=0; int n=2*fltValue+1; QPixmap kPix(n,n); kPix.fill(Qt::black);
   kp.begin(&kPix); kp.setPen(Qt::white); kp.setBrush(Qt::white);
    kp.drawEllipse(0,0,kPix.width()-1,kPix.height()-1);
   kp.end(); ki=kPix.toImage().convertToFormat(QImage::Format_Indexed8,
                                               Qt::ColorOnly);
   kernel.resize(0); count=0;
   for (int j=0;j<n;j++) for (int i=0;i<n;i++)
    if (qGray(ki.pixel(i,j))>0) count++;
   for (int j=0;j<n;j++) for (int i=0;i<n;i++)
    kernel.append((float)(qGray(ki.pixel(i,j)))/(float)(255*count));
  }

  void registerSegmenter(QObject *s) { // Main GUI Events
   segmenter=s;
   connect(this,SIGNAL(signalShowMsg(QString,int)),
           segmenter,SLOT(slotShowMsg(QString,int)));
   connect(this,SIGNAL(signalLoaded()),segmenter,SLOT(slotLoaded()));
   connect(this,SIGNAL(signalProcessed()),segmenter,SLOT(slotProcessed()));
   connect(this,SIGNAL(signalPreview()),segmenter,SLOT(slotPreview()));
  }

  void showMsg(QString s,int t) { emit signalShowMsg(s,t); }


  void loadSet_TIFFDir(QString dirName) {
   if (!processing) {
    QStringList filter; QDir dir(dirName);
    filter << "*.jpg"; filter << "*.tif";
    fileNames=dir.entryList(filter); // Filenames
    for (int i=0;i<fileNames.size();i++)
     fileNames[i]=dir.absoluteFilePath(fileNames[i]);
    cmd=LOADVOLUME_TIFFDIR; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until loading finishes!",4000);
   }
  }

  void saveSet_TIFFDir(QString dirName) { QString s;
   if (!processing) {
    QStringList filter; QDir dir(dirName); fileNames.clear();
    for (int i=0;i<zSize;i++) {
     if (i<10) s="00"; else if (i<100) s="0";
     fileNames.append(dir.absoluteFilePath(s+dummyString.setNum(i)+".tif"));
    }
    cmd=SAVEVOLUME_TIFFDIR; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until saving finishes!",4000);
   }
  }

  void loadSet_BinFile(QString fileName) {
   if (!processing) {
    fileNames.clear(); fileNames.append(fileName);
    cmd=LOADVOLUME_BINFILE; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until loading finishes!",4000);
   }
  }

  void saveScalp_ObjFile(QString fileName) { QString s;
   if (!processing) { fileNames.clear(); fileNames.append(fileName);
    cmd=SAVESCALP_OBJFILE; processing=true; start(HighestPriority);
   } else {
    showMsg("Still saving mesh.. Please wait until it finishes!",4000);
   }
  }

  void saveSkull_ObjFile(QString fileName) { QString s;
   if (!processing) { fileNames.clear(); fileNames.append(fileName);
    cmd=SAVESKULL_OBJFILE; processing=true; start(HighestPriority);
   } else {
    showMsg("Still saving mesh.. Please wait until it finishes!",4000);
   }
  }

  void saveBrain_ObjFile(QString fileName) { QString s;
   if (!processing) { fileNames.clear(); fileNames.append(fileName);
    cmd=SAVEBRAIN_OBJFILE; processing=true; start(HighestPriority);
   } else {
    showMsg("Still saving mesh.. Please wait until it finishes!",4000);
   }
  }


  // *** PREVIEWED ONES ***

  void setThrValue(int x) { thrValue=x;
   if (previewMode) {
    MRISlice *s=volume[srcX].slice[curSlice];
    threshold(s,&previewSlice,thrValue,thrBin,thrFill,thrInv);
    emit signalPreview();
   }
  }

  void setFltValue(int x) {
   MRISlice *s; fltValue=x;

   // Construct moving average kernel of diameter 2*x+1
   int count=0; int n=2*fltValue+1; QPixmap kPix(n,n); kPix.fill(Qt::black);
   kp.begin(&kPix); kp.setPen(Qt::white); kp.setBrush(Qt::white);
    kp.drawEllipse(0,0,kPix.width()-1,kPix.height()-1);
   kp.end(); ki=kPix.toImage().convertToFormat(QImage::Format_Indexed8,
                                               Qt::ColorOnly);
   kernel.resize(0); count=0;
   for (int j=0;j<n;j++) for (int i=0;i<n;i++)
    if (qGray(ki.pixel(i,j))>0) count++;
   for (int j=0;j<n;j++) for (int i=0;i<n;i++)
     kernel.append((float)(qGray(ki.pixel(i,j)))/(float)(255*count));

   if (previewMode) {
    s=volume[srcX].slice[curSlice];

    filter2D(s,&previewSlice,&kernel); emit signalPreview();
   }
  }

  void selection(QPoint m) { QString resultStr,px,py,pz;
   float x=(float)(m.x()); float y=(float)(m.y()); float z=(float)(curSlice); 
   framePts[pOffset].x=(x-(float)(xSize/2))*0.032;
   framePts[pOffset].y=(y-(float)(ySize/2))*0.032;
   framePts[pOffset].z=(z-(float)(zSize/2))*0.25;

   resultStr="Selected point is ("+
             px.setNum(framePts[pOffset].x)+","+
             py.setNum(framePts[pOffset].y)+","+
             pz.setNum(framePts[pOffset].z)+").";
   showMsg(resultStr,0);

   mouseMode=MOUSEMODE_DRAW;
  }

  // *** SERVED VARIABLES ***

  int numBuffers,fWidth,fHeight,guiX,guiY,guiWidth,guiHeight,xSize,ySize,zSize;
  QMutex m; bool mrSetExists,previewMode; MRISlice previewSlice;
  QVector<MRIVolume> volume; QVector<quint8> sliceThrs;

  int curVolumeL,curVolumeR,curSlice,srcX,dstX,curFrame;

  int thrValue,fltValue;
  bool thrBin,thrFill,thrInv,thrPS;

  QVector<float> kernel;
  QVector<QVector<QPoint> > scalpCoord2D;
  QVector<QVector<QPoint> > skullCoord2D;
  QVector<QVector<QPoint> > brainCoord2D;
  QVector<Coord3D> scalpCoord; QVector<QVector<int> > scalpIndex;
  QVector<Coord3D> skullCoord; QVector<QVector<int> > skullIndex;
  QVector<Coord3D> brainCoord; QVector<QVector<int> > brainIndex;

  int mouseMode,pOffset; QVector<Coord3D> framePts;
  int brushSize,top,bottom;

 signals:
  void signalShowMsg(QString,int);
  void signalLoaded(); void signalProcessed(); void signalPreview();


 public slots:
  void slotQuit() { application->quit(); }


  // *** THREAD STARTERS FOR VOLUME2VOLUME PROCESSES ***

  void slotNormStart() {
   if (!processing) {
    cmd=COMPUTE_NORM; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until per-slice normalization finishes!",4000);
   }
  }

  void slotCopyStart() {
   if (!processing) {
    cmd=COMPUTE_COPY; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until copying finishes!",4000);
   }
  }

  void slotEdgeStart() {
   if (!processing) {
    cmd=COMPUTE_EDGE; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until edge computation finishes!",4000);
   }
  }

  void slotKMeansStart() {
   if (!processing) {
    cmd=COMPUTE_KMEANS; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until clustering finishes!",4000);
   }
  }

  void slotThrStart() {
   if (!processing) {
    cmd=COMPUTE_THRESHOLD; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until thresholding finishes!",4000);
   }
  }

  void slotFltStart() {
   if (!processing) {
    cmd=COMPUTE_FILTER; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until filtering finishes!",4000);
   }
  }

  void slotSrf1Start() {
   if (!processing) {
    cmd=COMPUTE_SCALP_SURF; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until surface computation finishes!",4000);
   }
  }
  void slotSrf2Start() {
   if (!processing) {
    cmd=COMPUTE_SKULL_SURF; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until surface computation finishes!",4000);
   }
  }
  void slotSrf3Start() {
   if (!processing) {
    cmd=COMPUTE_BRAIN_SURF; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until surface computation finishes!",4000);
   }
  }
  void slotSrf4Start() {
   if (!processing) {
    cmd=COMPUTE_SURFACE4; processing=true; start(HighestPriority);
   } else {
    showMsg("Please wait until surface computation finishes!",4000);
   }
  }

  void slotSelP1() {
   showMsg("Please select Nasion..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=0;
  }

  void slotSelP2() {
   showMsg("Please select Left Pre-Auricular Point..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=1;
  }

  void slotSelP3() {
   showMsg("Please select Right Pre-Auricular Point..",0);
   mouseMode=MOUSEMODE_POINT; pOffset=2;
  }

  void slotClrSlice() {
   volume[srcX].slice[curSlice]->clr(); emit signalProcessed();
  }

  // *** VOLUME2VOLUME PROCESS ADJUSTMENTS ***

  void slotThrBinSet(int x)  { thrBin=(bool)(x); }
  void slotThrFillSet(int x) { thrFill=(bool)(x); }
  void slotThrInvSet(int x)  { thrInv=(bool)(x); }
  void slotThrPSSet(int x)   { thrPS=(bool)(x); }
  void slotThrSetForSlice()  { sliceThrs[curSlice]=thrValue; }


 protected:
  void run() {
   switch (cmd) {
    case LOADVOLUME_TIFFDIR: tLoadVolume_TIFFDir();        break;
    case SAVEVOLUME_TIFFDIR: tSaveVolume_TIFFDir();        break;
    case LOADVOLUME_BINFILE: tLoadVolume_BinFile();        break;
    case SAVESCALP_OBJFILE:  tSaveScalp_ObjFile();         break;
    case SAVESKULL_OBJFILE:  tSaveSkull_ObjFile();         break;
    case SAVEBRAIN_OBJFILE:  tSaveBrain_ObjFile();         break;
    case COMPUTE_NORM:       tComputeNorm(srcX,dstX);      break;
    case COMPUTE_COPY:       tComputeCopy(srcX,dstX);      break;
    case COMPUTE_EDGE:       tComputeEdge(srcX,dstX);      break;
    case COMPUTE_THRESHOLD:  tComputeThreshold(srcX,dstX); break;
    case COMPUTE_FILTER:     tComputeFilter(srcX,dstX);    break;
    case COMPUTE_KMEANS:     tComputeKMeans(srcX,dstX);    break;
    case COMPUTE_SCALP_SURF: tComputeScalpSurface(srcX);   break;
    case COMPUTE_SKULL_SURF: tComputeSkullSurface(srcX);   break;
    case COMPUTE_BRAIN_SURF: tComputeBrainSurface(srcX);   break;
    case COMPUTE_SURFACE4:   tComputeSurface4(srcX);       break;
   }
  }


 private:

  // *** SLICE-BASED ROUTINES OVER VOLUMES ***

  void normalize(MRISlice *src,MRISlice *dst) { int lim,avg,count,pix,w,h;
   QImage *s=&(src->data); QImage *d=&(dst->data); lim=45; avg=count=0;
   w=s->width(); h=s->height();
   for (int j=5;j<lim;j++) for (int i=5;i<lim;i++) {
    avg+=qGray(s->pixel(i,j));
    avg+=qGray(s->pixel(w-1-i,j));
    avg+=qGray(s->pixel(i,h-1-j));
    avg+=qGray(s->pixel(w-1-i,h-1-j)); count+=4;
   } avg=(int)((float)(avg)/(float)(count));

   if (avg) { // Prevent div-by-zero..
    for (int j=0;j<h;j++) for (int i=0;i<w;i++) {
     pix=qGray(s->pixel(i,j)); pix=(int)((float)(pix)-(float)(avg));
     if (pix<0) pix=0;
     d->setPixel(i,j,pix);
    }
   } dst->update();
  }

  // Generic square convolution filter
  void filter2D(MRISlice *src,MRISlice *dst,QVector<float> *k) {
   int x,y,i,j,w,h,X,Y,t,index; int n=sqrt(k->size())/2;
   QImage *s=&(src->data); QImage *d=&(dst->data);
   w=s->width(); h=s->height();
   for (y=0;y<h;y++) for (x=0;x<w;x++)
    if (y>=src->yMin && y<src->yMax && x>=src->xMin && x<src->xMax) {
     t=index=0;
     for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) { Y=y+j; X=x+i;
      if (Y>=src->yMin && Y<src->yMax && X>=src->xMin && X<src->xMax)
       t+=(int)((float)(qGray(s->pixel(X,Y)))*(*k)[index]);
      index++;
     } d->setPixel(x,y,t);
    } else d->setPixel(x,y,0);
   dst->update();
  }

  void edge2D(MRISlice *src,MRISlice *dst) {
   int x,y,X,Y,w,h,i,j,t; int pix;
   int eMat[9]={-1,-1,-1,-1,8,-1,-1,-1,-1};
//   int eMat[9]={0,-1,0,-1,4,-1,0,-1,0};
   QImage *s=&(src->data); QImage *d=&(dst->data);
   w=s->width(); h=s->height();
   for (y=0;y<h;y++) for (x=0;x<w;x++)
    if (y>=src->yMin && y<src->yMax && x>=src->xMin && x<src->xMax) {
     for (t=0,j=-1;j<=1;j++) for (i=-1;i<=1;i++) { Y=y+j; X=x+i;
      t+=qGray(s->pixel(X,Y))*eMat[3*(j+1)+i+1];
     }
     pix=(int)(256.0+(float)(t)/(float)(9.0));
     if (pix>0 && pix<=255) pix=255; else pix=0; d->setPixel(x,y,pix);
    } else d->setPixel(x,y,0);
   dst->update();
  }

  void threshold(MRISlice *src,MRISlice *dst,
                int thr,bool bin,bool fill,bool inv) {
   quint8 pix; int x,y,fx,lx,w,h;
   QImage *s=&(src->data); QImage *d=&(dst->data);
   w=s->width(); h=s->height();
   if (fill) { ;
    for (y=0;y<h;y++) {
     x=0; while ((x<w) && qGray(s->pixel(x,y))<thr) {
      d->setPixel(x,y,0); x++; } fx=x+1;
     x=w-1; while ((x>=0) && qGray(s->pixel(x,y))<thr) {
      d->setPixel(x,y,0); x--; } lx=x-1;
     for (x=fx;x<lx;x++) { d->setPixel(x,y,255); }
    }
   } else {
    if (inv) {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(s->pixel(x,y));
       if (pix<=thr) d->setPixel(x,y,255); else d->setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(s->pixel(x,y));
       if (pix<=thr) d->setPixel(x,y,pix); else d->setPixel(x,y,0);
      }
     }
    } else {
     if (bin) {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(s->pixel(x,y));
       if (pix>=thr) d->setPixel(x,y,255); else d->setPixel(x,y,0);
      }
     } else {
      for (y=0;y<h;y++) for (x=0;x<w;x++) {
       pix=qGray(s->pixel(x,y));
       if (pix>=thr) d->setPixel(x,y,pix); else d->setPixel(x,y,0);
      }
     }
    }
   } dst->update();
  }

  void mask(MRISlice *src,MRISlice *dst,MRISlice *mask,quint8 thr) {
   int x,y,w,h;
   QImage *s=&(src->data); QImage *d=&(dst->data); QImage *m=&(mask->data);
   w=s->width(); h=s->height();
   for (y=0;y<h;y++) for (x=0;x<w;x++)
    if (y>=src->yMin && y<src->yMax && x>=src->xMin && x<src->xMax) {
     if (qGray(m->pixel(x,y)>=thr)) d->setPixel(x,y,qGray(s->pixel(x,y)));
     else d->setPixel(x,y,0);
    } else d->setPixel(x,y,0);
   dst->update();
  }

  void scalpSurface(MRISlice *dst) {
   dst->calcScalpCoords((float)(ySize),64);
  }
  void skullSurface(MRISlice *dst) {
   dst->calcSkullCoords((float)(ySize),64);
  }
  void brainSurface(MRISlice *dst) {
   dst->calcBrainCoords((float)(ySize),64);
  }





  void constructScalp(int src) { Coord3D c; QVector<QPoint> *cs;
   MRISlice *slice; float xx,yy,zz,xa,ya; int v0,v1,v2,v3;
   scalpIndex.resize(0); scalpCoord.resize(0); scalpCoord2D.resize(0);

   scalpCoord2D.resize(zSize);
   for (int k=0;k<zSize;k++) { cs=&(volume[src].slice[k]->scalpCoord); 
    for (int n=0;n<cs->size();n++) scalpCoord2D[k].append((*cs)[n]);
   }

   for (int i=zSize-1;i>=0;i--) { slice=volume[src].slice[i]; // Find Top
    if (slice->xMax>5 && slice->yMax>5) { top=i; break; }
   }
   for (int i=0;i<zSize;i++) { slice=volume[src].slice[i]; // Find bottom
    if (slice->xMax>5 && slice->yMax>5) { bottom=i; break; }
   }

   for (int k=bottom;k<=top;k++) { cs=&(volume[src].slice[k]->scalpCoord); 
    for (int n=0;n<cs->size();n++) {
     xx=(float)((*cs)[n].x()-xSize/2)*0.032;
     yy=(float)((*cs)[n].y()-ySize/2)*0.032;
     zz=(float)(k-zSize/2)*0.25;
     c.x=xx*cos(M_PI)-yy*sin(M_PI);
     c.y=xx*sin(M_PI)+yy*cos(M_PI); c.z=zz; scalpCoord.append(c);
    }
   }

   int n=scalpCoord.size()/(top-bottom+1); QVector<int> dummy(3);

   for (int k=bottom;k<top;k++) {
    slice=volume[src].slice[k];
    for (int i=0;i<n;i++) {
     v0=k*n+i; v1=k*n+((i+1)%n);
     v2=(k+1)*n+i; v3=(k+1)*n+((i+1)%n);
     dummy[0]=v0; dummy[1]=v2; dummy[2]=v1; scalpIndex.append(dummy);
     dummy[0]=v1; dummy[1]=v2; dummy[2]=v3; scalpIndex.append(dummy);
    }
   }

   // Close Apex
   cs=&(volume[src].slice[top]->scalpCoord); // Topmost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=((float)(xSize/2)-xa)*0.032;
   c.y=((float)(ySize/2)-ya)*0.032;
   c.z=(float)(top-zSize/2)*0.25+0.25/2; scalpCoord.append(c);
   printf("%2.2f %d %2.2f\n",ya,ySize,c.y);
   dummy[0]=scalpCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Topmost weave
    dummy[1]=dummy[0]-1-i; dummy[2]=dummy[0]-1-(i+1)%n;
    scalpIndex.append(dummy);
   }
   // Close Bottom
   cs=&(volume[src].slice[bottom]->scalpCoord); // Bottommost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=(xa-(float)(xSize/2))*0.032;
   c.y=(ya-(float)(ySize/2))*0.032;
   c.z=-(float)(zSize/2-bottom)*0.25-0.25/2.; scalpCoord.append(c);
   dummy[0]=scalpCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Bottommost weave
    dummy[1]=i; dummy[2]=(i+1)%n;
    scalpIndex.append(dummy);
   }
  }

  void constructSkull(int src) { Coord3D c; QVector<QPoint> *cs;
   MRISlice *slice; float xx,yy,zz,xa,ya; int v0,v1,v2,v3;
   skullIndex.resize(0); skullCoord.resize(0); skullCoord2D.resize(0);

   skullCoord2D.resize(zSize);
   for (int k=0;k<zSize;k++) { cs=&(volume[src].slice[k]->skullCoord); 
    for (int n=0;n<cs->size();n++) skullCoord2D[k].append((*cs)[n]);
   }

   for (int i=zSize-1;i>=0;i--) { slice=volume[src].slice[i]; // Find Top
    if (slice->xMax>5 && slice->yMax>5) { top=i; break; }
   }
   for (int i=0;i<zSize;i++) { slice=volume[src].slice[i]; // Find bottom
    if (slice->xMax>5 && slice->yMax>5) { bottom=i; break; }
   }

   for (int k=bottom;k<=top;k++) { cs=&(volume[src].slice[k]->skullCoord); 
    for (int n=0;n<cs->size();n++) {
     xx=(float)((*cs)[n].x()-xSize/2)*0.032;
     yy=(float)((*cs)[n].y()-ySize/2)*0.032;
     zz=(float)(k-zSize/2)*0.25;
     c.x=xx*cos(M_PI)-yy*sin(M_PI);
     c.y=xx*sin(M_PI)+yy*cos(M_PI); c.z=zz; skullCoord.append(c);
    }
   }

   int n=skullCoord.size()/(top-bottom+1); QVector<int> dummy(3);

   for (int k=bottom;k<top;k++) {
    slice=volume[src].slice[k];
    for (int i=0;i<n;i++) {
     v0=k*n+i; v1=k*n+((i+1)%n);
     v2=(k+1)*n+i; v3=(k+1)*n+((i+1)%n);
     dummy[0]=v0; dummy[1]=v2; dummy[2]=v1; skullIndex.append(dummy);
     dummy[0]=v1; dummy[1]=v2; dummy[2]=v3; skullIndex.append(dummy);
    }
   }

   // Close Apex
   cs=&(volume[src].slice[top]->skullCoord); // Topmost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=((float)(xSize/2)-xa)*0.032;
   c.y=((float)(ySize/2)-ya)*0.032;
   c.z=(float)(top-zSize/2)*0.25+0.25/2; skullCoord.append(c);
   printf("%2.2f %d %2.2f\n",ya,ySize,c.y);
   dummy[0]=skullCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Topmost weave
    dummy[1]=dummy[0]-1-i; dummy[2]=dummy[0]-1-(i+1)%n;
    skullIndex.append(dummy);
   }
   // Close Bottom
   cs=&(volume[src].slice[bottom]->skullCoord); // Bottommost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=(xa-(float)(xSize/2))*0.032;
   c.y=(ya-(float)(ySize/2))*0.032;
   c.z=-(float)(zSize/2-bottom)*0.25-0.25/2.; skullCoord.append(c);
   dummy[0]=skullCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Bottommost weave
    dummy[1]=i; dummy[2]=(i+1)%n;
    skullIndex.append(dummy);
   }
  }

  void constructBrain(int src) { Coord3D c; QVector<QPoint> *cs;
   MRISlice *slice; float xx,yy,zz,xa,ya; int v0,v1,v2,v3;
   brainIndex.resize(0); brainCoord.resize(0); brainCoord2D.resize(0);

   brainCoord2D.resize(zSize);
   for (int k=0;k<zSize;k++) { cs=&(volume[src].slice[k]->brainCoord); 
    for (int n=0;n<cs->size();n++) brainCoord2D[k].append((*cs)[n]);
   }

   for (int i=zSize-1;i>=0;i--) { slice=volume[src].slice[i]; // Find Top
    if (slice->xMax>5 && slice->yMax>5) { top=i; break; }
   }
   for (int i=0;i<zSize;i++) { slice=volume[src].slice[i]; // Find bottom
    if (slice->xMax>5 && slice->yMax>5) { bottom=i; break; }
   }

   for (int k=bottom;k<=top;k++) { cs=&(volume[src].slice[k]->brainCoord); 
    for (int n=0;n<cs->size();n++) {
     xx=(float)((*cs)[n].x()-xSize/2)*0.032;
     yy=(float)((*cs)[n].y()-ySize/2)*0.032;
     zz=(float)(k-zSize/2)*0.25;
     c.x=xx*cos(M_PI)-yy*sin(M_PI);
     c.y=xx*sin(M_PI)+yy*cos(M_PI); c.z=zz; brainCoord.append(c);
    }
   }

   int n=brainCoord.size()/(top-bottom+1); QVector<int> dummy(3);

   for (int k=bottom;k<top;k++) {
    slice=volume[src].slice[k];
    for (int i=0;i<n;i++) {
     v0=k*n+i; v1=k*n+((i+1)%n);
     v2=(k+1)*n+i; v3=(k+1)*n+((i+1)%n);
     dummy[0]=v0; dummy[1]=v2; dummy[2]=v1; brainIndex.append(dummy);
     dummy[0]=v1; dummy[1]=v2; dummy[2]=v3; brainIndex.append(dummy);
    }
   }

   // Close Apex
   cs=&(volume[src].slice[top]->brainCoord); // Topmost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=((float)(xSize/2)-xa)*0.032;
   c.y=((float)(ySize/2)-ya)*0.032;
   c.z=(float)(top-zSize/2)*0.25+0.25/2; brainCoord.append(c);
   printf("%2.2f %d %2.2f\n",ya,ySize,c.y);
   dummy[0]=brainCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Topmost weave
    dummy[1]=dummy[0]-1-i; dummy[2]=dummy[0]-1-(i+1)%n;
    brainIndex.append(dummy);
   }
   // Close Bottom
   cs=&(volume[src].slice[bottom]->brainCoord); // Bottommost slice
   xa=0.; ya=0.;
   for (int i=0;i<cs->size();i++) {
    xa+=(float)(*cs)[i].x(); ya+=(*cs)[i].y();
   } xa/=(float)cs->size(); ya/=(float)cs->size();
   c.x=(xa-(float)(xSize/2))*0.032;
   c.y=(ya-(float)(ySize/2))*0.032;
   c.z=-(float)(zSize/2-bottom)*0.25-0.25/2.; brainCoord.append(c);
   dummy[0]=brainCoord.size()-1; // Apex Index
   for (int i=0;i<n;i++) { // Bottommost weave
    dummy[1]=i; dummy[2]=(i+1)%n;
    brainIndex.append(dummy);
   }
  }

  // =======================================================================

  int tLoadVolume_TIFFDir() {
   bool loadError; MRISlice *slice; QVector<QRgb> gsTable;
   QImage *s=new QImage(); QImage *s2=new QImage();
   for (int i=0;i<256;i++) gsTable.append(qRgb(i,i,i));

   showMsg("Preloading set and checking for possible errors..",0);
   loadError=false; zSize=0; for (int i=0;i<fileNames.size();i++) {
    s->load(fileNames[i]);
    *s2=s->convertToFormat(QImage::Format_Indexed8,gsTable);
    if (i==0) { xSize=s->width(); ySize=s->height(); }
    if (xSize==0 || ySize==0) {
     showMsg("ERROR: Null image!",4); loadError=true; break;
    }
    if (s->width()!=xSize || s->height()!=ySize) {
     showMsg("ERROR: Different size of slice encountered!!",4);
     loadError=true; break;
    } zSize++;
   } if (loadError) { delete s; return -1; }

   previewSlice=s2->copy();

   // Reset/truncate previously existing volumes..
   for (int i=0;i<numBuffers;i++) {
    for (int j=0;j<volume[i].slice.size();j++) {
     volume[i].slice[j]->scalpCoord.resize(0);
     volume[i].slice[j]->histogram.resize(0);
     delete volume[i].slice[j];
    }
    volume[i].clear();
   }

   // Main loading loop..
   for (int i=0;i<zSize;i++) {
    s->load(fileNames[i]);
    *s2=s->convertToFormat(QImage::Format_Indexed8,gsTable);
    for (int j=0;j<numBuffers;j++) { // Fill all buffers with original set..
     slice=new MRISlice(s2->copy()); volume[j].append(slice);
    } showMsg("No error seems to exist. Now loading/allocating memory (%"+
              dummyString.setNum(i*100/zSize)+")..",0);
   } sliceThrs.resize(zSize);

   delete s; delete s2;
   mrSetExists=true; emit signalLoaded();
   showMsg("Slices have been loaded successfully..",4000);
   processing=false; return 0;
   return 0;
  }

  int tSaveVolume_TIFFDir() {
   MRISlice *s;
   for (int i=0;i<zSize;i++) {
    s=volume[srcX].slice[i]; s->data.save(fileNames[i]);
    showMsg("Saving slices (%"+
             dummyString.setNum(i*100/zSize)+")..",0);
   } showMsg("Slices have been saved successfully..",4000);
   processing=false; return 0;
  }

  int tLoadVolume_BinFile() {
   QFile file;
   file.setFileName(fileNames[0]+".info"); file.open(QIODevice::ReadOnly);
   QTextStream infoStream(&file);
   infoStream >> xSize; infoStream >> ySize; infoStream >> zSize; file.close();

   file.setFileName(fileNames[0]+".rawb"); file.open(QIODevice::ReadOnly);
   QDataStream rawbStream(&file);

   QVector<quint8> rawbData; quint8 x;
   while (!file.atEnd()) { rawbStream >> x; rawbData.append(x); }
   
   rawbData.resize(0); mrSetExists=true; emit signalLoaded();
   showMsg("Slices have been loaded successfully..",4000);
   processing=false; return 0;
  }

  int tSaveScalp_ObjFile() {
   QFile scalpFile; scalpFile.setFileName(fileNames[0]+".obj");
   scalpFile.open(QIODevice::WriteOnly); QTextStream scalpStream(&scalpFile);

   scalpStream << "# OBJ Generated by Octopus-Segmenter v0.9.5\n\n";
   scalpStream << "# Vertex count = " << scalpCoord.size() << "\n";
   scalpStream << "# Face count = " << scalpIndex.size() << "\n";
   scalpStream << "\n# VERTICES\n\n"; 

   scalpStream << "c ";
   for (int i=0;i<framePts.size();i++)
    scalpStream << framePts[i].x << " "
                << framePts[i].y << " "
                << framePts[i].z << " "; scalpStream << "\n\n";

   for (int i=0;i<scalpCoord.size();i++)
    scalpStream << "v " << scalpCoord[i].x << " "
                        << scalpCoord[i].y << " "
                        << scalpCoord[i].z << "\n";

   scalpStream << "\n# FACES\n\n"; 

   for (int i=0;i<scalpIndex.size();i++)
    scalpStream << "f " << scalpIndex[i][0] << " "
                        << scalpIndex[i][1] << " "
                        << scalpIndex[i][2] << "\n";

   scalpFile.close();
   showMsg("Scalp mesh has been saved successfully..",4000);
   processing=false; return 0;
  }

  int tSaveSkull_ObjFile() {
   QFile skullFile; skullFile.setFileName(fileNames[0]+".obj");
   skullFile.open(QIODevice::WriteOnly); QTextStream skullStream(&skullFile);

   skullStream << "# OBJ Generated by Octopus-Segmenter v0.9.5\n\n";
   skullStream << "# Vertex count = " << skullCoord.size() << "\n";
   skullStream << "# Face count = " << skullIndex.size() << "\n";
   skullStream << "\n# VERTICES\n\n"; 

   for (int i=0;i<skullCoord.size();i++)
    skullStream << "v " << skullCoord[i].x << " "
                        << skullCoord[i].y << " "
                        << skullCoord[i].z << "\n";

   skullStream << "\n# FACES\n\n"; 

   for (int i=0;i<skullIndex.size();i++)
    skullStream << "f " << skullIndex[i][0] << " "
                        << skullIndex[i][1] << " "
                        << skullIndex[i][2] << "\n";

   skullFile.close();
   showMsg("Skull mesh has been saved successfully..",4000);
   processing=false; return 0;
  }

  int tSaveBrain_ObjFile() {
   QFile brainFile; brainFile.setFileName(fileNames[0]+".obj");
   brainFile.open(QIODevice::WriteOnly); QTextStream brainStream(&brainFile);

   brainStream << "# OBJ Generated by Octopus-Segmenter v0.9.5\n\n";
   brainStream << "# Vertex count = " << brainCoord.size() << "\n";
   brainStream << "# Face count = " << brainIndex.size() << "\n";
   brainStream << "\n# VERTICES\n\n"; 

   for (int i=0;i<brainCoord.size();i++)
    brainStream << "v " << brainCoord[i].x << " "
                        << brainCoord[i].y << " "
                        << brainCoord[i].z << "\n";

   brainStream << "\n# FACES\n\n"; 

   for (int i=0;i<brainIndex.size();i++)
    brainStream << "f " << brainIndex[i][0] << " "
                        << brainIndex[i][1] << " "
                        << brainIndex[i][2] << "\n";

   brainFile.close();
   showMsg("Brain mesh has been saved successfully..",4000);
   processing=false; return 0;
  }

//
//   file.setFileName("electrode.obj");
//   file.open(QIODevice::ReadOnly);
//   stream.setDevice(&file);
//   elecVertexCount=elecIndexCount=0;
//   while (!stream.atEnd()) {
//    dummyStr=stream.readLine();
//    dummySL=dummyStr.split(" ");
//    if (dummySL[0]=="v") {
//     elecVerticesX.append(dummySL[1].toDouble());
//     elecVerticesY.append(dummySL[2].toDouble());
//     elecVerticesZ.append(dummySL[3].toDouble());
//     elecVertexCount++;
//    } else if (dummySL[0]=="f") {
//     dummySL2=dummySL[1].split("/"); elecIndicesA.append(dummySL2[0].toInt()-1);
//     dummySL2=dummySL[2].split("/"); elecIndicesB.append(dummySL2[0].toInt()-1);
//     dummySL2=dummySL[3].split("/"); elecIndicesC.append(dummySL2[0].toInt()-1);
//     elecIndexCount++;
//    }
//   stream.setDevice(0);
//   file.close();

  void tComputeNorm(int src,int dst) {
   MRISlice *s,*d;
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; normalize(s,d);
    showMsg("Normalizing volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Normalization is finished..",4000); processing=false;
  }

  void tComputeCopy(int src,int dst) {
   MRISlice *s,*d;
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; d->data=s->data.copy();
    showMsg("Copying volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Copying is finished..",4000); processing=false;
  }

//    QImage *sImg=&((volume[srcX].slice[curSlice])->data);
  void tComputeEdge(int src,int dst) {
   MRISlice *s,*d;
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; edge2D(s,d);
    showMsg("Detecting edges (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Edge detection is finished..",4000); processing=false;
  }


  void tComputeThreshold(int src,int dst) {
   MRISlice *s,*d;
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i];
    if (thrPS) threshold(s,d,sliceThrs[i],thrBin,thrFill,thrInv);
    else threshold(s,d,thrValue,thrBin,thrFill,thrInv);
    showMsg("Thresholding volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Threshold computation is finished..",4000); processing=false;
  }

  void tComputeFilter(int src,int dst) {
   MRISlice *s,*d; QPainter kp; QImage ki;

   // Apply it to all slices
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; filter2D(s,d,&kernel);
    showMsg("Filtering volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Filtering is finished..",4000); processing=false;
  }
 
  void tComputeKMeans(int src,int dst) {
   MRISlice *s,*d; QPainter kp; QImage ki;

   // Apply it to all slices
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; // kMeans2D(s,d);
    showMsg("Clustering volume slices in implicit 2D mode (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Clustering is finished..",4000); processing=false;
  }

  void tComputeMasked(int src,int dst,int msk) {
   MRISlice *s,*d,*m;
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; d=volume[dst].slice[i]; m=volume[msk].slice[i];
    mask(s,d,m,1);
    showMsg("Masking volume (%"+dummyString.setNum(i*100/zSize)+")..",0);
   } emit signalProcessed();
   showMsg("Masking is finished..",4000); processing=false;
  }


  void tComputeScalpSurface(int src) {
   MRISlice *s;
   // Apply it to all slices
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; scalpSurface(s);
    showMsg("Filtering volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } constructScalp(src); emit signalProcessed();
   showMsg("Scalp Surface computation is finished..",4000); processing=false;
  }

  void tComputeSkullSurface(int src) {
   MRISlice *s;
   // Apply it to all slices
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; skullSurface(s);
    showMsg("Filtering volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } constructSkull(src); emit signalProcessed();
   showMsg("Skull Surface computation is finished..",4000); processing=false;
  }
  void tComputeBrainSurface(int src) {
   MRISlice *s;
   // Apply it to all slices
   for (int i=0;i<zSize;i++) {
    s=volume[src].slice[i]; brainSurface(s);
    showMsg("Filtering volume (%"+
            dummyString.setNum(i*100/zSize)+")..",0);
   } constructBrain(src); emit signalProcessed();
   showMsg("Brain Surface computation is finished..",4000); processing=false;
  }
  void tComputeSurface4(int src) {
   int x=src; x++;
   emit signalProcessed();
   showMsg("Surface computation is finished..",4000); processing=false;
  }


  QApplication *application; QObject *segmenter;
  QString dummyString; QStringList fileNames; bool processing; int cmd;
  QPainter kp; QImage ki;
};

#endif
