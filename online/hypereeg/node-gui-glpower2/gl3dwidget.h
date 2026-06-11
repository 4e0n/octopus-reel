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

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QRect>
#include <QMutex>
#include <QColor>
#include <cmath>
#include <vector>
#include <algorithm>
#include <GL/glu.h>
#include "confparam.h"
#include "../../../common/coord3d.h"
#include "../../../common/vec3.h"

const int DSIZE=1000;

const float CAMERA_DISTANCE=40.;
const float CAMERA_FOV=70.;
const float MIN_Z=4.;
const float MAX_Z=100.;
const float ERP_ZOOM_Z=26.;
const float ELECTRODE_RADIUS=0.4;
const float ELECTRODE_HEIGHT=0.25;

class GL3DWidget:public QFrame {
 Q_OBJECT
 public:
  enum CenterMode {
   UseMean=0,
   UseMedian=1
  };

  explicit GL3DWidget(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr):QFrame(parent) {
   conf=c; ampNo=a;
   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    setGeometry(20+ampNo*(conf->frameW+20),20,conf->frameW,conf->frameH);
   } else {
    const int perRow=conf->guiMaxAmpPerLine;
    const int row=int(ampNo)/perRow;
    const int col=int(ampNo)%perRow;
    setGeometry(20+col*(conf->frameW+20),20+row*(conf->frameH+20),conf->frameW,conf->frameH);
   }
   glImage=QImage(conf->frameW,conf->frameH,QImage::Format_RGB32);
   glImage.fill(Qt::white);
  }

  void setCenterMode(CenterMode m) {
   centerMode=m;
   refStatsInit=false;
   bipStatsInit=false;
  }

  void refreshImage() {
   std::vector<float> refVals,bipVals;
   refVals.reserve(conf->refChns.size());
   bipVals.reserve(conf->bipChns.size());

   auto validDataIndex=[&](int dataIdx)->bool {
    if (ampNo>=unsigned(conf->curGLData.size())) return false;
    if (dataIdx<0 || dataIdx>=conf->curGLData[int(ampNo)].size()) return false;
    return true;
   };

   for (int i=0;i<conf->refChns.size();++i) {
    if (validDataIndex(i)) refVals.push_back(conf->curGLData[int(ampNo)][i]);
   }
   for (int i=0;i<conf->bipChns.size();++i) {
    const int dataIdx=int(conf->refChnCount)+i;
    if (validDataIndex(dataIdx)) bipVals.push_back(conf->curGLData[int(ampNo)][dataIdx]);
   }

   if (refVals.empty() && bipVals.empty()) {
    glImage.fill(Qt::white);
    update();
    return;
   }

   const float alpha=0.15f;
   float refCenterNow=0.0f,refStdNow=1.0f;
   float bipCenterNow=0.0f,bipStdNow=1.0f;

   if (!refVals.empty()) {
    refCenterNow=computeCenter(refVals); refStdNow=computeStd(refVals,refCenterNow);
    if (!refStatsInit) {
     refSmoothCenter=refCenterNow; refSmoothStd=refStdNow; refStatsInit=true;
    } else {
     refSmoothCenter=(1.0f-alpha)*refSmoothCenter+alpha*refCenterNow;
     refSmoothStd=(1.0f-alpha)*refSmoothStd+alpha*refStdNow;
    }
   }

   if (!bipVals.empty()) {
    bipCenterNow=computeCenter(bipVals); bipStdNow=computeStd(bipVals,bipCenterNow);
    if (!bipStatsInit) {
     bipSmoothCenter=bipCenterNow; bipSmoothStd=bipStdNow; bipStatsInit=true;
    } else {
     bipSmoothCenter=(1.0f-alpha)*bipSmoothCenter+alpha*bipCenterNow;
     bipSmoothStd=(1.0f-alpha)*bipSmoothStd+alpha*bipStdNow;
    }
   }

   glImage.fill(Qt::white);
   QPainter p(&glImage);
   p.setRenderHint(QPainter::Antialiasing,true); p.setPen(Qt::black);
   p.drawRect(0,0,conf->frameW-1,conf->frameH-1); p.drawText(10,20,QString("AMP %1").arg(ampNo+1));

   unsigned tXmax=0,tYmax=0;

   auto updateMaxima=[&](const QVector<GUIChnInfo> &chs) {
    for (const auto &ch:chs) {
     if ((unsigned)ch.topoX>tXmax) tXmax=ch.topoX;
     if ((unsigned)ch.topoY>tYmax) tYmax=ch.topoY;
    }
   };

   updateMaxima(conf->refChns); updateMaxima(conf->bipChns); updateMaxima(conf->metaChns);

   if (tXmax==0||tYmax==0) { p.end(); update(); return; }

   const float w=float(conf->frameW); const float aspect=0.95f;
   const float cellSize=w/float(tXmax); const int markerSize=qMax(12,int(aspect*cellSize));
   const float cf=float(tXmax)/(float(tXmax)+1.0f);

   auto drawChannel=[&](const GUIChnInfo &ch,int dataIdx,float center,float spread) {
    if (!validDataIndex(dataIdx)) return;

    const float rawValue=conf->curGLData[int(ampNo)][dataIdx];
    const float z=(rawValue-center)/qMax(spread,1e-6f);
    const QColor col=paletteColor(z);

    const unsigned topoX=unsigned(ch.topoX); const unsigned topoY=unsigned(ch.topoY);

    unsigned int yOffset=(cellSize/2)*cf;
    if (topoY>1) yOffset+=cellSize/2;
    if (topoY>10) yOffset+=cellSize/2;

    const int x=int(cellSize/2-markerSize/2+cellSize*(topoX-1));
    const int y=int(cellSize/2-markerSize/2+cellSize*(topoY-1)+yOffset);

    QRect cr(x,y,markerSize,markerSize);

    p.setBrush(col); p.setPen(Qt::black); p.drawEllipse(cr);

    QFont f=p.font(); f.setPointSize(qMax(6,markerSize/5)); f.setBold(true); p.setFont(f);
    p.drawText(cr,Qt::AlignCenter,ch.chnName);
   };

   for (int i=0;i<conf->refChns.size();++i) {
    drawChannel(conf->refChns[i],i,refSmoothCenter,refSmoothStd);
   }

   for (int i=0;i<conf->bipChns.size();++i) {
    drawChannel(conf->bipChns[i],int(conf->refChnCount)+i,bipSmoothCenter,bipSmoothStd);
   }

   // No meta -- and probably won't be.

   p.end();
   update();
  }

  QImage glImage;

 protected:
  void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   QPainter mainPainter(this);
   mainPainter.drawImage(0,0,glImage);
  }

 private:
  float computeCenter(const std::vector<float> &vals) const {
   if (vals.empty()) return 0.0f;
   return (centerMode==UseMedian) ? computeMedian(vals):computeMean(vals);
  }

  static float computeMean(const std::vector<float> &vals) {
   if (vals.empty()) return 0.0f;
   double sum=0.0;
   for (float v:vals) sum+=v;
   return float(sum/double(vals.size()));
  }

  static float computeMedian(std::vector<float> vals) {
   if (vals.empty()) return 0.0f;

   const size_t n=vals.size(); const size_t mid=n/2;

   std::nth_element(vals.begin(),vals.begin()+mid,vals.end());
   float med=vals[mid];

   if ((n%2)==0) {
    const auto maxIt=std::max_element(vals.begin(),vals.begin()+mid);
    med=0.5f*(med+*maxIt);
   }
   return med;
  }

  static float computeStd(const std::vector<float> &vals,float center) {
   if (vals.empty()) return 1.0f;
   double var=0.0;
   for (float v:vals) { const double d=double(v)-double(center); var+=d*d; }
   double s=std::sqrt(var/double(vals.size()));
   if (s<1e-6) s=1e-6;
   return float(s);
  }

  static QColor paletteColor(float z) {
   float t=(z+8.0f)/16.0f; // found after some trial & error
   if (t<0.0f) t=0.0f;
   if (t>1.0f) t=1.0f;

   const int idx=qBound(0,int(t*255.0f),255);

   if (idx<128) return QColor(2*idx,255,0); // green -> yellow
   return QColor(255,255-2*(idx-128),0);    // yellow -> red
  }

  ConfParam *conf=nullptr; unsigned int ampNo=0;

  //CenterMode centerMode=UseMedian; // change to UseMean for testing
  CenterMode centerMode=UseMean;

  bool refStatsInit=false; float refSmoothCenter=0.0f; float refSmoothStd=1.0f;
  bool bipStatsInit=false; float bipSmoothCenter=0.0f; float bipSmoothStd=1.0f;





  void setXRotation(int angle) { normalizeAngle(&angle); if (angle!=xRot) { xRot=angle; updateGL(); } }
  void setYRotation(int angle) { normalizeAngle(&angle); if (angle!=yRot) { yRot=angle; updateGL(); } }
  void setZRotation(int angle) { normalizeAngle(&angle); if (angle!=zRot) { zRot=angle; updateGL(); } }
  void setCameraLocation(float z) {
   if (z<MIN_Z) zTrans=MIN_Z; else if (z>=MAX_Z) zTrans=MAX_Z; else zTrans=z;
   updateGL();
  }

  // *** BASIC SHAPE LIBRARY ***

  void arrow(float totalL,float headL,float baseW,float headW) {
   GLUquadricObj* q=gluNewQuadric(); glPushMatrix();
    gluCylinder(q,baseW,baseW,totalL-headL,10,1);
    glTranslatef(0,0,totalL-headL); // Original is in +Z direction
    gluCylinder(q,headW,0,headL,10,1);
   glPopMatrix(); gluDeleteQuadric(q);
  }

  void electrode(float r,float theta,float phi,float elecR,float elecH) {
   GLUquadricObj* q=gluNewQuadric(); glPushMatrix();
    glRotatef(phi,0,0,1); glRotatef(theta,0,1,0);
    glPushMatrix();
     glTranslatef(0.0,0.0,r-elecH*2./5.);
     glPushMatrix();
      glPushMatrix(); // Bottom
       glRotatef(180,1,0,0);
       glPushMatrix(); gluCylinder(q,elecR,0.,0.001,40,1); glPopMatrix();
      glPopMatrix();

      glPushMatrix(); gluCylinder(q,elecR,elecR,elecH,40,1); glPopMatrix();

      glPushMatrix(); // Top
       glTranslatef(0.,0.,elecH);
       glPushMatrix(); gluCylinder(q,elecR,0.,0.001,40,1); glPopMatrix();
      glPopMatrix();
     glPopMatrix();
    glPopMatrix();
   glPopMatrix(); gluDeleteQuadric(q);
  }

  void sphereXYZ(float x,float y,float z,float elecR) {
   GLUquadricObj* q=gluNewQuadric();
   glPushMatrix();
    glTranslatef(x,y,z);
    gluSphere(q,elecR,10,10);
   glPopMatrix(); gluDeleteQuadric(q);
  }

  void dipoleArrow(float rs,float rc,float lc) {
   GLUquadricObj* q1=gluNewQuadric(); // Base sphere
   GLUquadricObj* q2=gluNewQuadric(); // Orientation cylinder
   glPushMatrix();
    glPushMatrix();
     gluSphere(q1,rs,10,10);
     glTranslatef(0.,0.,rs*0.75);
     gluCylinder(q2,rc,rc,lc,10,1);
    glPopMatrix();
    glPushMatrix();
     glTranslatef(0,0,rs*0.75+lc); // Original is in +Z direction
     gluCylinder(q2,rc,0.,.001,10,1);
    glPopMatrix();
   glPopMatrix(); gluDeleteQuadric(q1); gluDeleteQuadric(q2);
  }

  void electrodeBorder(bool mode,int chn,float r,float th,float phi,float eR) {
   float elecR; if (chn==conf->headModel[ampNo].currentElectrode) elecR=eR*3; else elecR=eR;
   if (mode) QGLWidget::qglColor(QColor(255,255,255,224)); else QGLWidget::qglColor(QColor(0,0,0,224));
   glPushMatrix(); glRotatef(phi,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix(); glTranslatef(0.,0.,r+ELECTRODE_HEIGHT*3./5.+0.0015);
     glPushMatrix(); glBegin(GL_POLYGON); for (float i=0.;i<2.*M_PI;i+=M_PI/40.) glVertex3f(elecR*cos(i),elecR*sin(i),0.); glEnd(); glPopMatrix();
    glPopMatrix();
   glPopMatrix();
  }

  void electrodeAvg(int chn,int idx,float r,float th,float ph,float eR) { float elecR,zPt,c,m;
   QVector<QVector<float>> *evts=&(conf->chnInfo[chn].avgData[ampNo]); //int sz=data->size();
//   QColor evtColor=conf->acqEvents[idx]->color;
   //qDebug() << chn << idx << r << th << ph << eR << sz;
   //    for (int t=0;t<sz;t++)
   //     qDebug() << (*data)[t];
   if (chn==conf->headModel[ampNo].currentElectrode) elecR=eR*3; else elecR=eR;
   glPushMatrix();
    glRotatef(ph,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix();
     glTranslatef(0.0,0.0,r+ELECTRODE_HEIGHT*3./5.+0.002);
     glPushMatrix();
      glBegin(GL_LINES);
       qglColor(QColor(96,96,96,224)); glVertex3f(0.,-elecR,0.002); glVertex3f(0., elecR,0.002); // Amp Zeroline
      glEnd();
     glPopMatrix();
    glPopMatrix();
   glPopMatrix();
  }

  // *** LIST OBJECTS ***

  GLuint makeFrame() { GLuint list=glGenLists(1); // Right-hand Orthogonal Axis
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    qglColor(QColor(0,0,255,128)); glPushMatrix(); arrow(20.,1.5,0.1,0.3); glPopMatrix(); // Z-axis -- blended blue
    qglColor(QColor(255,0,0,128)); glPushMatrix(); glRotatef(90.,0,1,0); arrow(20.,1.5,0.1,0.3); glPopMatrix(); // X-axis -- blended red
    qglColor(QColor(0,255,0,128)); glPushMatrix(); glRotatef(-90.,1,0,0); arrow(20.,1.5,0.1,0.3); glPopMatrix(); // Y-axis -- blended green
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeGrid() { GLuint list=glGenLists(2); // XY-plane Grid
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glBegin(GL_LINES);
     qglColor(QColor(96,96,96,72)); // X-grid, Y-grid, in Blended gray
     for (int x=-15;x<16;x++) {
      glVertex3f((float)x,-15.,0.); glVertex3f((float)x, 15.,0.); glVertex3f(-15.,(float)x,0.); glVertex3f( 15.,(float)x,0.);
     }
    glEnd();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeParametric() { float r,h; // Make parametric electrode/cap set..
   GLuint list=glGenLists(4);
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    for (int chnIdx=0;chnIdx<conf->chnInfo.size();chnIdx++) {
     r=ELECTRODE_RADIUS; h=ELECTRODE_HEIGHT;
     if (chnIdx==conf->headModel[ampNo].currentElectrode) { r=r*3; qglColor(QColor(255,255,255,144)); } // Hilite
     else qglColor(conf->chnInfo[chnIdx].cmColor);
     //if (ampNo==0)
     // qDebug() << conf->headModel[ampNo].scalpParamR << conf->chnInfo[chnIdx].param.y << conf->chnInfo[chnIdx].param.z << r << h;
     electrode(conf->headModel[ampNo].scalpParamR,conf->chnInfo[chnIdx].param.y,conf->chnInfo[chnIdx].param.z,r,h); // theta,phi
    } glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeGizmo() { int v0,v1,v2;
   float v0c0S,v0c1S,v0c2S, v1c0S,v1c1S,v1c2S, v2c0S,v2c1S,v2c2S;
   float v0c0C,v0c1C,v0c2C, v1c0C,v1c1C,v1c2C, v2c0C,v2c1C,v2c2C;
   GLuint list=glGenLists(6);
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glBegin(GL_TRIANGLES);
     for (int gizIdx=0;gizIdx<conf->glGizmo.size();gizIdx++) {
      if (gizIdx==conf->headModel[ampNo].currentGizmo) qglColor(QColor(255,0,0,96));
      else qglColor(QColor(0,0,255,96)); // Hilite (red) vs. Red or blue - default cap color
      for (int triIdx=0;triIdx<conf->glGizmo[gizIdx].tri.size();triIdx++) { v0=v1=v2=0;
       for (int chnIdx=0;chnIdx<conf->chnInfo.size();chnIdx++) {
        if (conf->chnInfo[chnIdx].type==0) {
         if (conf->chnInfo[chnIdx].physChn-1==conf->glGizmo[gizIdx].tri[triIdx][0]) v0=chnIdx;
         if (conf->chnInfo[chnIdx].physChn-1==conf->glGizmo[gizIdx].tri[triIdx][1]) v1=chnIdx;
         if (conf->chnInfo[chnIdx].physChn-1==conf->glGizmo[gizIdx].tri[triIdx][2]) v2=chnIdx;
	}
       }
       //qDebug() << gizIdx << "(" << v0 << "," << v1 << "," << v2 << ")";
       if (conf->headModel[ampNo].gizmoOnReal) {
        // Realistic Head
        v0c0S=conf->chnInfo[v0].real[0]; v0c1S=conf->chnInfo[v0].real[1]; v0c2S=conf->chnInfo[v0].real[2];
        v1c0S=conf->chnInfo[v1].real[0]; v1c1S=conf->chnInfo[v1].real[1]; v1c2S=conf->chnInfo[v1].real[2];
        v2c0S=conf->chnInfo[v2].real[0]; v2c1S=conf->chnInfo[v2].real[1]; v2c2S=conf->chnInfo[v2].real[2];
        glVertex3f(v0c0S,v0c1S,v0c2S); glVertex3f(v1c0S,v1c1S,v1c2S); glVertex3f(v2c0S,v2c1S,v2c2S);
       } else {
        // Parametric Head
        v0c0S=v1c0S=v2c0S=conf->headModel[ampNo].scalpParamR;
        v0c1S=conf->chnInfo[v0].param.y; v0c2S=conf->chnInfo[v0].param.z; // theta,phi
        v1c1S=conf->chnInfo[v1].param.y; v1c2S=conf->chnInfo[v1].param.z;
        v2c1S=conf->chnInfo[v2].param.y; v2c2S=conf->chnInfo[v2].param.z;
        v0c0C=v0c0S*cos(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.);
        v0c1C=v0c0S*sin(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.); v0c2C=v0c0S*cos(2.*M_PI*v0c1S/360.);
        v1c0C=v1c0S*cos(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.);
        v1c1C=v1c0S*sin(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.); v1c2C=v1c0S*cos(2.*M_PI*v1c1S/360.);
        v2c0C=v2c0S*cos(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.);
        v2c1C=v2c0S*sin(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.); v2c2C=v2c0S*cos(2.*M_PI*v2c1S/360.);
        glVertex3f(v0c0C,v0c1C,v0c2C); glVertex3f(v1c0C,v1c1C,v1c2C); glVertex3f(v2c0C,v2c1C,v2c2C);
       }
      }
     }
    glEnd();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeHeadShell(Obj3D* obj,int listNo,QColor col) {
   QVector<Coord3D>* c=&(obj->c); QVector<QVector<unsigned int> >* idx=&(obj->idx);
   GLuint list=glGenLists(listNo);
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glPushMatrix();
    const QColor c=col;
    QGLWidget::qglColor(c);
//     glTranslatef(0.,conf->glScalpNasion,0.);glRotatef(-conf->glScalpCzAngle,1,0,0);glTranslatef(0.,-conf->glScalpNasion,0.);
     glBegin(GL_TRIANGLES);
      for (int i=0;i<idx->size();i++) {
       glVertex3f((*c)[(*idx)[i][0]].x,(*c)[(*idx)[i][0]].y,(*c)[(*idx)[i][0]].z);
       glVertex3f((*c)[(*idx)[i][1]].x,(*c)[(*idx)[i][1]].y,(*c)[(*idx)[i][1]].z);
       glVertex3f((*c)[(*idx)[i][2]].x,(*c)[(*idx)[i][2]].y,(*c)[(*idx)[i][2]].z);
      }
     glEnd();
    glPopMatrix();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  void normalizeAngle(int *angle) {
   while (*angle < 0) *angle += 360*16;
   while (*angle > 360*16) *angle -= 360*16;
  }

  unsigned int ampNo;
  int xRot,yRot,zRot; float zTrans,frameAlpha,frameBeta,frameGamma; QPoint eventPos;
  QWidget *parent; QPainter painter;
  GLuint frame,grid,scalp,skull,brain,gizmo,dig,parametric,realistic,avgs;






};


/*

class GL3DWidget : public QGLWidget {
 Q_OBJECT
 public:
  explicit GL3DWidget(ConfParam *c=nullptr,unsigned int a=0,QWidget *p=nullptr) : QGLWidget(QGLFormat(QGL::SampleBuffers),p) {
   conf=c;
   parent=p; ampNo=a; xRot=yRot=zRot=0; zTrans=CAMERA_DISTANCE;
   setMouseTracking(true); setAutoFillBackground(false);
  }

  QSize sizeHint() const override        { return QSize(parent->width(),parent->height()); }
  QSize minimumSizeHint() const override { return QSize(parent->width(),parent->height()); }

  ~GL3DWidget() {
   makeCurrent();
   if (conf->headModel[ampNo].brainLoaded) glDeleteLists(brain,10);
   if (conf->headModel[ampNo].skullLoaded) glDeleteLists(skull,9);
   if (conf->headModel[ampNo].scalpLoaded) glDeleteLists(scalp,8);
//   glDeleteLists(avgs,7);
   if (conf->glGizmoLoaded) glDeleteLists(gizmo,6);
//   glDeleteLists(realistic,5);
   glDeleteLists(parametric,4);
//   if (acqM->digExists[ampNo]) glDeleteLists(dig,3);
   glDeleteLists(grid,2);
   glDeleteLists(frame,1);
  }

  void setView(float theta,float phi) {
   setXRotation(0.); setYRotation(-16.*(-90.+theta)); setZRotation(-16*phi);
   setCameraLocation(ERP_ZOOM_Z);
   updateGL();
  }
  
  ConfParam *conf;

 public slots:
  void glUpdateSlot() { updateGL(); }
  void glUpdateParamSlot(unsigned int a) {
   if (a==ampNo) {
    if (conf->glGizmoLoaded) { glDeleteLists(gizmo,6); gizmo=makeGizmo(); }
    glDeleteLists(parametric,4); parametric=makeParametric();
    updateGL();
   }
  }

 protected:
  void initializeGL() override {
   qDebug() << "GL VENDOR:" << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
   qDebug() << "RENDERER:"  << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
   qDebug() << "VERSION:"   << reinterpret_cast<const char*>(glGetString(GL_VERSION));
   frame=makeFrame(); grid=makeGrid();
//   if (acqM->digExists[ampNo]) dig=makeDigitizer();
   parametric=makeParametric();
//   realistic=makeRealistic();
   if (conf->glGizmoLoaded) gizmo=makeGizmo();
//   avgs=makeAverages();
   if (conf->headModel[ampNo].scalpLoaded) scalp=makeHeadShell(&(conf->headModel[ampNo].scalp), 8,QColor(255,255,255, 64));
   if (conf->headModel[ampNo].skullLoaded) skull=makeHeadShell(&(conf->headModel[ampNo].skull), 9,QColor(  0,255,  0, 96));
   if (conf->headModel[ampNo].brainLoaded) brain=makeHeadShell(&(conf->headModel[ampNo].brain),10,QColor(255, 64, 64,128));

   static GLfloat ambientLight[4]={0.45,0.45,0.45,1.}; glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   static GLfloat diffuseLight[4]={0.45,0.45,0.45,1.}; glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);

   glLightf(GL_LIGHT0,GL_SPOT_CUTOFF,50.); glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,40.); // 80-degree cone

   static GLfloat matAmbient[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_AMBIENT,matAmbient);
   static GLfloat matDiffuse[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_DIFFUSE,matDiffuse);
   static GLfloat matSpecular[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_SPECULAR,matSpecular);

   //glClearColor(QColor(120,120,150,255)); // Bluish White
   glClearColor(120/255.f,120/255.f,150/255.f,1.0f);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_MULTISAMPLE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
// glEnable(GL_CULL_FACE); // leave CULL_FACE off until you verify geometry
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(CAMERA_FOV, 1.0, 0.5, 300.0);
   glMatrixMode(GL_MODELVIEW);

//   glEnable(GL_POLYGON_SMOOTH);
//   glShadeModel(GL_SMOOTH);
//   glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE); glEnable(GL_MULTISAMPLE); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
//   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
//   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
//   glMatrixMode(GL_PROJECTION);
// //  glLoadIdentity(); gluPerspective(CAMERA_FOV,(float)(conf->sweepFrameW)/(float)(conf->sweepFrameH),1.,300.);
//       gluPerspective(CAMERA_FOV, 1.0, 0.5, 300.0);  // safe placeholder
//   glMatrixMode(GL_MODELVIEW);
   connect(conf,SIGNAL(glUpdate()),this,SLOT(glUpdateSlot())); 
   connect(conf,SIGNAL(glUpdateParam(unsigned int)),this,SLOT(glUpdateParamSlot(unsigned int))); 
  }

//  void paintGL() override {
//    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    // camera
//    glLoadIdentity();
//    gluLookAt(zTrans,0,0,  0,0,0,  0,0,1);
//
//    // light position needs w = 1
//    Vec3 lp(zTrans,0,0);
//    lp.rotX(2.*M_PI*xRot/360./16.); lp.rotY(2.*M_PI*yRot/360./16.); lp.rotZ(2.*M_PI*zRot/360./16.);
//    GLfloat lightPos[4] = { GLfloat(lp[0]), GLfloat(lp[1]), GLfloat(lp[2]), 1.0f };
//    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
//
//    // draw something big
//    glCallList(grid);
//    glCallList(frame);
//  }

  void paintGL() override {
   makeCurrent(); // glMatrixMode(GL_MODELVIEW);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   glLoadIdentity(); gluLookAt(zTrans,0.,0.,0.,0.,0.,0.,0.,1.);
   glRotated(xRot/16.,1.,0.,0.); glRotated(yRot/16.,0.,1.,0.); glRotated(zRot/16.,0.,0.,1.);

   glPushMatrix();
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
    glMaterialf(GL_FRONT,GL_SHININESS,5.);

    static GLfloat lightPos[4]; Vec3 lp(zTrans,0.,0.);
    lp.rotX(2.*M_PI*xRot/360./16.); lp.rotY(2.*M_PI*yRot/360./16.); lp.rotZ(2.*M_PI*zRot/360./16.);
    lightPos[0]=lp[0]; lightPos[1]=lp[1]; lightPos[2]=lp[2]; lightPos[3]=1.0f; glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

    static GLfloat spotDir[3]; Vec3 sd=lp; sd.normalize();
    spotDir[0]=-sd[0]; spotDir[1]=-sd[1]; spotDir[2]=-sd[2];
    glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,spotDir);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    if (conf->glFrameOn[ampNo]) glCallList(frame);
    if (conf->glGridOn[ampNo])  glCallList(grid);
    if (conf->headModel[ampNo].scalpLoaded && conf->glScalpOn[ampNo]) glCallList(scalp);
    if (conf->headModel[ampNo].skullLoaded && conf->glSkullOn[ampNo]) glCallList(skull);
    if (conf->headModel[ampNo].brainLoaded && conf->glBrainOn[ampNo]) glCallList(brain);
    if (conf->glGizmoLoaded && conf->glGizmoOn[ampNo]) glCallList(gizmo);
    if (conf->glElectrodesOn[ampNo]) glCallList(parametric);

//    glDisable(GL_CULL_FACE);
//     if (conf->glGizmoLoaded && conf->glGizmoOn[ampNo]) glCallList(gizmo);
//     if (acqM->hwAvgsV[ampNo]) glCallList(avgs);
//    glEnable(GL_CULL_FACE);

    glDisable(GL_LIGHTING);
//     if (acqM->digExists[ampNo] && acqM->hwDigV[ampNo]) glCallList(dig);
    glEnable(GL_LIGHTING);
   glPopMatrix();
  }

//  void resizeGL(int width,int height) { int side=qMin(width,height);
//   glViewport((width-side)/2,(height-side)/2,side,side);
//   glMatrixMode(GL_PROJECTION); glLoadIdentity(); // glOrtho(-0.5,+0.5,+0.5,-0.5,4.0,15.0);
//   gluPerspective(CAMERA_FOV,(float)(conf->sweepFrameW)/(float)(conf->sweepFrameH),2.,120.);
//   glMatrixMode(GL_MODELVIEW);
//  }

  void resizeGL(int w,int h) override {
   const qreal dpr=devicePixelRatioF();
   GLint vw=GLint(std::lround(w*dpr));
   GLint vh=GLint(std::lround(h*dpr));
   glViewport(0,0,vw,vh);

   qDebug() << "[resizeGL] widget size =" << w << "x" << h
            << " dpr=" << dpr << " -> viewport =" << vw << "x" << vh;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(CAMERA_FOV, double(vw)/double(vh), 0.5, 300.0);
   glMatrixMode(GL_MODELVIEW);
  }

//void paintGL() override {
//    glDisable(GL_SCISSOR_TEST); // avoid Qt scissor corner case
//    glClearColor(0.05f,0.05f,0.08f,1.0f);
//    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
//
//    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
//    qDebug() << "[paintGL] viewport =" << vp[2] << "x" << vp[3];
//    // draw your grid/frame...
//}

  void mousePressEvent(QMouseEvent *event) { eventPos=event->pos(); }

  void mouseMoveEvent(QMouseEvent *event) {
   int dx=event->x()-eventPos.x(); int dy=event->y()-eventPos.y();
   if (event->buttons() & Qt::LeftButton) { setYRotation(yRot+8*dy); setZRotation(zRot+8*dx); }
   else if (event->buttons() & Qt::RightButton) { setCameraLocation(zTrans-(float)(dy)/8.0); }
   else if (event->buttons() & Qt::MiddleButton) { ; //    setYRotation(xRot+8*dx);
   } eventPos=event->pos();
  }


//  glEnable(GL_NORMALIZE);
//  glMaterialfv(GL_FRONT, GL_DIFFUSE, logoDiffuseColor);
//  glFrontFace(GL_CCW);
//  glCullFace(GL_BACK);
//  glEdgeFlag(GL_FALSE);
//  glPolygonMode(GL_BACK,GL_LINES);
//  glEnable(GL_DEPTH_TEST);
//  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
//  glMaterialfv(GL_FRONT,GL_SPECULAR,matSpecular);
//  glDisable(GL_LIGHT0);
//  glDisable(GL_COLOR_MATERIAL);

*/
