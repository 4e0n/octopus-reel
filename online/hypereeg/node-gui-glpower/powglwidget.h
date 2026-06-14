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

#include <QtOpenGL/QGLWidget>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QColor>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <GL/glu.h>
#include "confparam.h"

const float GLPOWER_CAMERA_DISTANCE = 70.0f; // 50–70
const float GLPOWER_CAMERA_FOV      = 45.0f; // 30–45

const float GLPOWER_HEAD_RADIUS=11.0f;
const float GLPOWER_ELECTRODE_RADIUS=0.3f;

class PowGLWidget:public QGLWidget {
 Q_OBJECT
 public:
  enum DisplayMode {ModePower=0,ModeCorrelation=1};

  explicit PowGLWidget(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr,DisplayMode m=ModePower):
           QGLWidget(QGLFormat(QGL::SampleBuffers),parent) {
   conf=c; ampNo=a; mode=m;
   selectedBand=ConfParam::POWER_ALPHA;
   setMouseTracking(true);
  }

  void refreshImage() { updateGL(); }

  void rebuildInterpolation() {
   buildProjectedElectrodes();
   buildScalpInterpolation();
   buildBrainInterpolation();
   updateGL();
  }

 signals:
  void viewChanged();

 protected:
  void initializeGL() override {
   qDebug() << "[GLPOWER] GL_VENDOR:"   << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
   qDebug() << "[GLPOWER] GL_RENDERER:" << reinterpret_cast<const char*>(glGetString(GL_RENDERER));
   qDebug() << "[GLPOWER] GL_VERSION:"  << reinterpret_cast<const char*>(glGetString(GL_VERSION));

   conf->glElecYOffset=-1.7f; conf->glElecZOffset=1.4f; conf->glElecSagittalRotDeg=10.0f;

   //glClearColor(0.07f,0.07f,0.10f,1.0f); // black
   //glClearColor(235.0f/255.0f,240.0f/255.0f,248.0f/255.0f,1.0f); // very light bluish gray
   //glClearColor(248.0f/255.0f,246.0f/255.0f,240.0f/255.0f,1.0f); // warm ivory
   glClearColor(225.0f/255.0f,230.0f/255.0f,235.0f/255.0f,1.0f); // medical
   //glClearColor(250.0f/255.0f,250.0f/255.0f,250.0f/255.0f,1.0f); // almost pure white

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_MULTISAMPLE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
   glShadeModel(GL_SMOOTH);
   glEnable(GL_NORMALIZE);
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glFrontFace(GL_CCW); // GL_CW if head disappears

   static GLfloat ambientLight[4]={0.40f,0.40f,0.40f,1.0f}; static GLfloat diffuseLight[4]={0.75f,0.75f,0.75f,1.0f};
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight); glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);

   if (conf->scalpObj.loaded) scalpList=makeHeadShell(conf->scalpObj,QColor(180,190,205,140));
   if (conf->skullObj.loaded) skullList=makeHeadShell(conf->skullObj,QColor(110,150,110,55));
   if (conf->brainObj.loaded) brainList=makeHeadShell(conf->brainObj,QColor(190,100,100,75));

   conf->glMapGain=0.85f; conf->glMapAlpha=255.0f;

   buildProjectedElectrodes();
   buildScalpInterpolation();
   buildBrainInterpolation();
  }

  void electrodeToXYZ(const PowChnInfo &ch,float &x,float &y,float &z) const {
   const float r=GLPOWER_HEAD_RADIUS + conf->glElecRadiusOffset;
   const float th=ch.topoTheta*float(M_PI)/180.0f; const float ph=ch.topoPhi*float(M_PI)/180.0f;

   // 1. Original spherical coordinates
   x=r*std::cos(ph)*std::sin(th); y=r*std::sin(ph)*std::sin(th); z=r*std::cos(th);

   // 2. Sagittal-plane correction: rotate around X axis, i.e. YZ plane
   const float ax=conf->glElecSagittalRotDeg*float(M_PI)/180.0f;
   if (std::fabs(ax)>1e-6f) {
    const float yy=y*std::cos(ax)-z*std::sin(ax); const float zz=y*std::sin(ax)+z*std::cos(ax);
    y=yy; z=zz;
   }

   // 3. Transverse (azimuth) correction: rotate around Z axis
   const float az=conf->glElecAzimuthRotDeg*float(M_PI)/180.0f;
   if (std::fabs(az)>1e-6f) {
    const float xx=x*std::cos(az)-y*std::sin(az); const float yy=x*std::sin(az)+y*std::cos(az);
    x=xx; y=yy;
   }

   // 4. Translation (XYZ offset)
   x+=conf->glElecXOffset; y+=conf->glElecYOffset; z+=conf->glElecZOffset;
  }
  
  void buildScalpInterpolation() {
   scalpInterp.clear(); scalpInterpReady=false;
   if (!conf || !conf->scalpObj.loaded) return;
   if (conf->refChns.isEmpty()) return;
   QVector<QVector3D> elecPos; elecPos.reserve(conf->refChns.size());
   for (const auto &ch:conf->refChns) { float x,y,z; electrodeToXYZ(ch,x,y,z); elecPos.append(QVector3D(x,y,z)); }
   scalpInterp.resize(conf->scalpObj.v.size());
   for (int vi=0; vi<conf->scalpObj.v.size(); ++vi) {
    const QVector3D v=conf->scalpObj.v[vi];
    QVector<QPair<float,int>> dist; dist.reserve(elecPos.size());
    for (int ei=0;ei<elecPos.size();++ei) { const float d=(v-elecPos[ei]).length(); dist.append(qMakePair(d,ei)); }
    std::sort(dist.begin(),dist.end(),[](const QPair<float,int> &a,const QPair<float,int> &b) { return a.first<b.first; });
    const int n=qMin(conf->glInterpNeighbors,dist.size());
    float wSum=0.0f;
    for (int k=0;k<n;++k) {
     const float d=qMax(dist[k].first,GLPOWER_INTERP_EPS); const float w=1.0f/std::pow(d,conf->glInterpPower);
     scalpInterp[vi].elecIdx.append(dist[k].second); scalpInterp[vi].weight.append(w);
     wSum+=w;
    }
    if (wSum>1e-9f) {
     for (int k=0;k<scalpInterp[vi].weight.size();++k) scalpInterp[vi].weight[k]/=wSum;
    }
   }
   scalpInterpReady=true;
  }

  void buildBrainInterpolation() {
   brainInterp.clear(); brainInterpReady=false;
   if (!conf || !conf->brainObj.loaded) return;
   if (!projectedElecReady || projectedElecPos.isEmpty()) return;
   brainInterp.resize(conf->brainObj.v.size());
   for (int vi=0; vi<conf->brainObj.v.size(); ++vi) {
    const QVector3D v=conf->brainObj.v[vi];
    QVector<QPair<float,int>> dist;
    dist.reserve(projectedElecPos.size());
    for (int ei=0;ei<projectedElecPos.size();++ei) { const float d=(v-projectedElecPos[ei]).length(); dist.append(qMakePair(d,ei)); }
    std::sort(dist.begin(),dist.end(),[](const QPair<float,int> &a,const QPair<float,int> &b) { return a.first<b.first; });
    const int n=qMin(conf->glInterpNeighbors,dist.size());
    float wSum=0.0f;
    for (int k=0;k<n;++k) {
     const float d=qMax(dist[k].first,GLPOWER_INTERP_EPS); const float w=1.0f/std::pow(d,conf->glInterpPower);
     brainInterp[vi].elecIdx.append(dist[k].second); brainInterp[vi].weight.append(w);
     wSum+=w;
    }
    if (wSum>1e-9f) {
     for (int k=0; k<brainInterp[vi].weight.size(); ++k) brainInterp[vi].weight[k]/=wSum;
    }
   }
   brainInterpReady=true;
  }

  void drawColoredScalp() {
   if (!conf || !conf->scalpObj.loaded || !scalpInterpReady) return;
   QVector<float> vals; vals.reserve(conf->refChns.size());
   {
    QMutexLocker lk(&conf->mutex);
    for (int i=0;i<conf->refChns.size();++i) vals.append(channelValue(i));
   }
   if (vals.isEmpty()) return;
   QVector<float> vertexVals; vertexVals.resize(conf->scalpObj.v.size());
   for (int vi=0;vi<conf->scalpObj.v.size();++vi) {
    float interp=0.0f;
    const auto &ip=scalpInterp[vi];
    for (int j=0;j<ip.elecIdx.size();++j) {
     const int ei=ip.elecIdx[j]; if (ei>=0 && ei<vals.size()) interp+=ip.weight[j]*vals[ei];
    }
    vertexVals[vi]=interp;
   }
   float mapMin=1e30f; float mapMax=-1e30f;
   if (mode==ModePower) {
    for (float v:vertexVals) { mapMin=qMin(mapMin,v); mapMax=qMax(mapMax,v); }
   } else {
    mapMin=-1.0f; mapMax=1.0f;
   }
   float span=qMax(mapMax-mapMin,1e-6f);
   if (mode==ModePower) {
    const float a=0.10f; // smaller means smoother range
    if (!smoothMapInit) { smoothMapMin=mapMin; smoothMapMax=mapMax; smoothMapInit=true; }
    else { smoothMapMin=(1.0f-a)*smoothMapMin+a*mapMin; smoothMapMax=(1.0f-a)*smoothMapMax+a*mapMax; }
    mapMin=smoothMapMin; mapMax=smoothMapMax;
   }
   glDisable(GL_LIGHTING);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   glBegin(GL_TRIANGLES);
   for (int fi=0;fi<conf->scalpObj.f.size();++fi) {
    const auto &tri=conf->scalpObj.f[fi];
    if (tri.size()<3) continue;
    for (int k=0;k<3;++k) {
     const int vi=int(tri[k]); if (vi<0 || vi>=conf->scalpObj.v.size()) continue;
     const float interp=vertexVals[vi];
     QColor c;
     if (mode==ModeCorrelation) {
      float r=interp;
      if (!std::isfinite(r)) r=0.0f;
      r=qBound(-1.0f,r,1.0f);
      c=corrColor(r);
     } else {
      float t=(interp-mapMin)/span;
      if (!std::isfinite(t)) t=0.5f;
      t=qBound(0.0f,t,1.0f);
      t=0.5f+conf->glMapGain*(t-0.5f);
      t=qBound(0.0f,t,1.0f);
      c=heatColorBWR(t,int(conf->glMapAlpha));
     }
     qglColor(c);
     if (vi<conf->scalpObj.n.size()) { const QVector3D &nn=conf->scalpObj.n[vi]; glNormal3f(nn.x(),nn.y(),nn.z()); }
     const QVector3D &vv=conf->scalpObj.v[vi]; glVertex3f(vv.x(),vv.y(),vv.z());
    }
   }
   glEnd();

   glDisable(GL_BLEND);
   glEnable(GL_LIGHTING);
  }

  void drawColoredBrain() {
   if (!conf || !conf->brainObj.loaded || !brainInterpReady) return;
   QVector<float> vals; vals.reserve(conf->refChns.size());
   {
    QMutexLocker lk(&conf->mutex);
    for (int i=0;i<conf->refChns.size();++i) vals.append(channelValue(i));
   }
   if (vals.isEmpty()) return;
   QVector<float> vertexVals; vertexVals.resize(conf->brainObj.v.size());
   for (int vi=0;vi<conf->brainObj.v.size();++vi) {
    float interp=0.0f;
    const auto &ip=brainInterp[vi];
    for (int j=0;j<ip.elecIdx.size();++j) {
     const int ei=ip.elecIdx[j]; if (ei>=0 && ei<vals.size()) interp+=ip.weight[j]*vals[ei];
    }
    vertexVals[vi]=interp;
   }
   float mapMin=1e30f; float mapMax=-1e30f;
   if (mode==ModePower) {
    for (float v:vertexVals) { mapMin=qMin(mapMin,v); mapMax=qMax(mapMax,v); }
   } else {
    mapMin=-1.0f; mapMax=1.0f;
   }
   float span=qMax(mapMax-mapMin,1e-6f);
   if (mode==ModePower) {
    const float a=0.10f; // smaller means smoother range
    if (!smoothMapInit) { smoothMapMin=mapMin; smoothMapMax=mapMax; smoothMapInit=true; }
    else { smoothMapMin=(1.0f-a)*smoothMapMin+a*mapMin; smoothMapMax=(1.0f-a)*smoothMapMax+a*mapMax; }
    mapMin=smoothMapMin; mapMax=smoothMapMax;
   }
   glDisable(GL_LIGHTING);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   glBegin(GL_TRIANGLES);
   for (int fi=0;fi<conf->brainObj.f.size();++fi) {
    const auto &tri=conf->brainObj.f[fi];
    if (tri.size()<3) continue;
    for (int k=0;k<3;++k) {
     const int vi=int(tri[k]); if (vi<0 || vi>=conf->brainObj.v.size()) continue;
     const float interp=vertexVals[vi];
     QColor c;
     if (mode==ModeCorrelation) {
      float r=interp;
      if (!std::isfinite(r)) r=0.0f;
      r=qBound(-1.0f,r,1.0f);
      c=corrColor(r);
     } else {
      float t=(interp-mapMin)/span;
      if (!std::isfinite(t)) t=0.5f;
      t=qBound(0.0f,t,1.0f);
      t=0.5f+conf->glMapGain*(t-0.5f);
      t=qBound(0.0f,t,1.0f);
      c=heatColorBWR(t,int(conf->glMapAlpha));
     }
     qglColor(c);
     if (vi<conf->brainObj.n.size()) { const QVector3D &nn=conf->brainObj.n[vi]; glNormal3f(nn.x(),nn.y(),nn.z()); }
     const QVector3D &vv=conf->brainObj.v[vi]; glVertex3f(vv.x(),vv.y(),vv.z());
    }
   }
   glEnd();

   glDisable(GL_BLEND);
   glEnable(GL_LIGHTING);
  }

  void resizeGL(int w,int h) override {
   if (h<=0) h=1;
   glViewport(0,0,w,h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   float fov;
   {
    QMutexLocker lk(&conf->glViewMutex);
    fov=conf->glCameraFov;
   }
   gluPerspective(fov,double(w)/double(h),0.5,300.0);
   glMatrixMode(GL_MODELVIEW);
  }

  void paintGL() override {
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();
   int xRot,yRot,zRot; float zTrans;
   {
    QMutexLocker lk(&conf->glViewMutex);
    xRot=conf->glXRot; yRot=conf->glYRot; zRot=conf->glZRot;
    zTrans=conf->glCameraDistance;
   }
   gluLookAt(zTrans,0.0,0.0,0.0,0.0,0.0,0.0,0.0,1.0);
   glRotated(xRot/16.0,1.0,0.0,0.0); glRotated(yRot/16.0,0.0,1.0,0.0); glRotated(zRot/16.0,0.0,0.0,1.0);
   GLfloat lightPos[4]={zTrans,0.0f,20.0f,1.0f}; glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

   glPushMatrix();
   glScalef(1.0f,1.0f,1.20f);
   // BRAIN
   if (conf->showBrain) {
    if (conf->brainObj.loaded && brainInterpReady) drawColoredBrain();
    else if (brainList) glCallList(brainList);
   }
   // ELECTRODES
   if (conf->showElectrodes) drawElectrodes();
   // SKULL
   if (conf->showSkull) {
    if (skullList) glCallList(skullList);
   }
   // SCALP
   if (conf->showScalp) {
    if (conf->scalpObj.loaded && scalpInterpReady) {
     if (conf->glMapAlpha>=250) {
      glDepthMask(GL_TRUE);
      drawColoredScalp();
     } else {
      glDepthMask(GL_FALSE);
      drawColoredScalp();
      glDepthMask(GL_TRUE);
     }
    } else if (scalpList) glCallList(scalpList);
   }
   glPopMatrix();
  }

  void mousePressEvent(QMouseEvent *event) override { lastPos=event->pos(); }

  void mouseMoveEvent(QMouseEvent *event) override {
   const int dx=event->x()-lastPos.x(); const int dy=event->y()-lastPos.y();
   {
    QMutexLocker lk(&conf->glViewMutex);
    if (event->buttons()&Qt::LeftButton) {
     conf->glYRot+=8*dy; conf->glZRot+=8*dx;
     normalizeAngle(&conf->glYRot); normalizeAngle(&conf->glZRot);
    } else if (event->buttons()&Qt::RightButton) {
     conf->glCameraDistance-=float(dy)/8.0f;
     if (conf->glCameraDistance<8.0f) conf->glCameraDistance=8.0f;
     if (conf->glCameraDistance>120.0f) conf->glCameraDistance=120.0f;
    }
   }
   lastPos=event->pos();
   emit viewChanged();
  }

 private:
  float channelValue(int chIdx) const {
   if (!conf) return 0.0f;

   if (mode==ModeCorrelation) {
    if (chIdx<0 || chIdx>=conf->curChCorr.size()) return 0.0f;
    if (selectedBand>=conf->curChCorr[chIdx].size()) return 0.0f;
    return conf->curChCorr[chIdx][selectedBand];
   }

   if (ampNo>=unsigned(conf->curPowData.size())) return 0.0f;
   if (chIdx<0 || chIdx>=conf->curPowData[int(ampNo)].size()) return 0.0f;
   if (selectedBand>=conf->curPowData[int(ampNo)][chIdx].size()) return 0.0f;

   return conf->curPowData[int(ampNo)][chIdx][selectedBand];
  }

  QColor corrColor(float r) const {
   r=qBound(-1.0f,r,1.0f);
   const float t=0.5f*(r+1.0f);
   return heatColorBWR(t,int(conf->glMapAlpha));
  }

  GLuint makeHeadShell(const PowObj3D &obj,const QColor &col) {
   if (!obj.loaded) return 0;
   GLuint list=glGenLists(1);
   glNewList(list,GL_COMPILE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   qglColor(col);
   glBegin(GL_TRIANGLES);
   for (int i=0;i<obj.f.size();++i) {
    const auto &tri=obj.f[i];
    if (tri.size()<3) continue;
    if (tri[0]>=unsigned(obj.v.size())) continue;
    if (tri[1]>=unsigned(obj.v.size())) continue;
    if (tri[2]>=unsigned(obj.v.size())) continue;
    const QVector3D &a=obj.v[int(tri[0])]; const QVector3D &b=obj.v[int(tri[1])]; const QVector3D &c=obj.v[int(tri[2])];
    QVector3D n=QVector3D::normal(a,b,c);
    glNormal3f(n.x(),n.y(),n.z()); glVertex3f(a.x(),a.y(),a.z()); glVertex3f(b.x(),b.y(),b.z()); glVertex3f(c.x(),c.y(),c.z());
   }
   glEnd();
   glDisable(GL_BLEND);
   glEndList();
   return list;
  }

  void drawHeadShell() {
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   qglColor(QColor(190,190,205,80));

   GLUquadricObj *q=gluNewQuadric();
   gluQuadricNormals(q,GLU_SMOOTH);

   glPushMatrix();
   glScalef(0.86f,1.0f,1.12f);
   gluSphere(q,GLPOWER_HEAD_RADIUS,64,64);
   glPopMatrix();

   gluDeleteQuadric(q);
   glDisable(GL_BLEND);
  }

  void drawElectrodes() {
   if (!conf) return;
   if (mode==ModePower) {
    if (ampNo>=unsigned(conf->curPowData.size())) return;
   }
   QVector<float> vals; vals.reserve(conf->refChns.size());
   {
    QMutexLocker lk(&conf->mutex);
    for (int i=0;i<conf->refChns.size();++i) {
     if (i>=conf->curPowData[int(ampNo)].size()) continue;
     if (selectedBand>=conf->curPowData[int(ampNo)][i].size()) continue;
     vals.append(conf->curPowData[int(ampNo)][i][selectedBand]);
    }
   }
   if (vals.isEmpty()) return;

   float vMin=vals[0],vMax=vals[0];
   for (float v:vals) {
    if (v<vMin) vMin=v;
    if (v>vMax) vMax=v;
   }
   const float span=std::max(vMax-vMin,1e-6f);

   QMutexLocker lk(&conf->mutex);
   for (int i=0;i<conf->refChns.size();++i) {
    if (i>=conf->curPowData[int(ampNo)].size()) continue;
    if (selectedBand>=conf->curPowData[int(ampNo)][i].size()) continue;

    QColor color;
    if (mode==ModeCorrelation) {
     float r=channelValue(i);
     if (!std::isfinite(r)) r=0.0f;
     r=qBound(-1.0f,r,1.0f);
     color=corrColor(r);
    } else {
     const float v=channelValue(i);
     float t=(v-vMin)/span;
     if (!std::isfinite(t)) t=0.5f;
     t=qBound(0.0f,t,1.0f);
     color=heatColorBWR(t,255);
    }

    const auto &ch=conf->refChns[i];
    float x,y,z;
    if (projectedElecReady && i<projectedElecPos.size()) { const QVector3D p=projectedElecPos[i]; x=p.x(); y=p.y(); z=p.z(); }
    else { electrodeToXYZ(ch,x,y,z); }

    qglColor(color);

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    GLUquadricObj *q=gluNewQuadric();
    glPushMatrix();
    glTranslatef(x,y,z);
    gluSphere(q,GLPOWER_ELECTRODE_RADIUS,16,16);
    glPopMatrix();
    gluDeleteQuadric(q);

    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
   }
  }

  static void sphericalToXYZ(float thetaDeg,float phiDeg,float r,float &x,float &y,float &z) {
   const float th=thetaDeg*float(M_PI)/180.0f; const float ph=phiDeg*float(M_PI)/180.0f;
   x=r*std::cos(ph)*std::sin(th); y=r*std::sin(ph)*std::sin(th); z=r*std::cos(th);
  }

  static QColor heatColor(float t) {
   if (t<0.0f) t=0.0f;
   if (t>1.0f) t=1.0f;
   const int r=int(255.0f*t); const int g=int(80.0f*(1.0f-std::abs(t-0.5f)*2.0f)); const int b=int(255.0f*(1.0f-t));
   return QColor(r,g,b,255);
  }

  void normalizeAngle(int *angle) {
   while (*angle<0) *angle+=360*16;
   while (*angle>360*16) *angle-=360*16;
  }

  bool rayTriangleIntersect(const QVector3D &orig,const QVector3D &dir,
                            const QVector3D &v0,const QVector3D &v1,const QVector3D &v2,float &tOut) const {
   const float EPS=1e-6f; QVector3D e1=v1-v0; QVector3D e2=v2-v0;

   QVector3D p=QVector3D::crossProduct(dir,e2); float det=QVector3D::dotProduct(e1,p);
   if (std::fabs(det)<EPS) return false;

   float invDet=1.0f/det;
   QVector3D s=orig-v0;

   float u=invDet*QVector3D::dotProduct(s,p); if (u<0.0f || u>1.0f) return false;

   QVector3D q=QVector3D::crossProduct(s,e1); float v=invDet*QVector3D::dotProduct(dir,q); if (v<0.0f || u+v>1.0f) return false;

   float t=invDet*QVector3D::dotProduct(e2,q); if (t<=EPS) return false;

   tOut=t;
   return true;
  }

  QVector3D electrodeDirection(const PowChnInfo &ch) const {
   const float th=ch.topoTheta*float(M_PI)/180.0f; const float ph=ch.topoPhi*float(M_PI)/180.0f;
   QVector3D d(std::cos(ph)*std::sin(th),std::sin(ph)*std::sin(th),std::cos(th));

   // Same manual correction as before, but without radius.
   const float ax=conf->glElecSagittalRotDeg*float(M_PI)/180.0f;
   if (std::fabs(ax)>1e-6f) {
    const float y=d.y()*std::cos(ax)-d.z()*std::sin(ax); const float z=d.y()*std::sin(ax)+d.z()*std::cos(ax);
    d.setY(y); d.setZ(z);
   }

   const float az=conf->glElecAzimuthRotDeg*float(M_PI)/180.0f;
   if (std::fabs(az)>1e-6f) {
    const float x=d.x()*std::cos(az)-d.y()*std::sin(az); const float y=d.x()*std::sin(az)+d.y()*std::cos(az);
    d.setX(x); d.setY(y);
   }

   d.normalize();
   return d;
  }

  void buildProjectedElectrodes() {
   projectedElecPos.clear(); projectedElecReady=false;
   if (!conf || !conf->scalpObj.loaded) return;
   projectedElecPos.resize(conf->refChns.size());
   const QVector3D origin(conf->glElecXOffset,conf->glElecYOffset,conf->glElecZOffset);

   for (int ei=0;ei<conf->refChns.size();++ei) {
    QVector3D dir=electrodeDirection(conf->refChns[ei]);
    bool found=false; float bestT=1e30f;
    for (const auto &tri:conf->scalpObj.f) {
     if (tri.size()<3) continue;
     if (tri[0]>=unsigned(conf->scalpObj.v.size())) continue;
     if (tri[1]>=unsigned(conf->scalpObj.v.size())) continue;
     if (tri[2]>=unsigned(conf->scalpObj.v.size())) continue;
     float t=0.0f;
     if (rayTriangleIntersect(origin,dir,conf->scalpObj.v[int(tri[0])],conf->scalpObj.v[int(tri[1])],conf->scalpObj.v[int(tri[2])],t)) {
      if (t<bestT) { bestT=t; found=true; }
     }
    }

    if (found) {
     QVector3D p=origin+bestT*dir;
     QVector3D n=p-origin; if (n.lengthSquared()>1e-9f) n.normalize(); // Push slightly outside scalp along radial direction
     p+=n*conf->glElecRadiusOffset; projectedElecPos[ei]=p;
    } else { // fallback to old spherical placement
     float x,y,z; electrodeToXYZ(conf->refChns[ei],x,y,z);
     projectedElecPos[ei]=QVector3D(x,y,z);
    }
   }
   projectedElecReady=true;
  }

  static QColor heatColorBWR(float t,int alpha=180) {
   t=qBound(0.0f,t,1.0f);
   int r,g,b;
   if (t<0.5f) { float u=t*2.0f; r=int(255*u); g=int(255*u); b=255; }
   else { float u=(t-0.5f)*2.0f; r=255; g=int(255*(1.0f-u)); b=int(255*(1.0f-u)); }
   return QColor(r,g,b,alpha);
  }

  static float computeMean(const QVector<float> &vals) {
   if (vals.isEmpty()) return 0.0f;
   double s=0.0; for (float v:vals) s+=v;
   return float(s/double(vals.size()));
  }

  static float computeStd(const QVector<float> &vals,float mean) {
   if (vals.isEmpty()) return 1.0f;
   double s=0.0; for (float v:vals) { const double d=double(v)-double(mean); s+=d*d; }
   float stdv=float(std::sqrt(s/double(vals.size())));
   if (stdv<1e-6f) stdv=1e-6f;
   return stdv;
  }

  ConfParam *conf=nullptr; unsigned int ampNo=0; int selectedBand=ConfParam::POWER_ALPHA;

  QPoint lastPos; GLuint scalpList=0,skullList=0,brainList=0;

  static constexpr float GLPOWER_INTERP_EPS=0.001f;

  struct VertexInterp { QVector<int> elecIdx; QVector<float> weight; };

  GLuint scalpColorList=0;
  QVector<VertexInterp> scalpInterp; bool scalpInterpReady=false;
  QVector<VertexInterp> brainInterp; bool brainInterpReady=false;

  QVector<QVector3D> projectedElecPos; bool projectedElecReady=false;

  DisplayMode mode=ModePower;

  float smoothMapMin=0.0f,smoothMapMax=1.0f; bool smoothMapInit=false;
};
