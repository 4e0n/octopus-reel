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

#ifndef HEADGLWIDGET_H
#define HEADGLWIDGET_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QMutex>
#include <cmath>
#include <GL/glu.h>

#include "acqmaster.h"
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

class HeadGLWidget : public QGLWidget {
 Q_OBJECT
 public:
  HeadGLWidget(QWidget *p,AcqMaster *acqm,unsigned int a) : QGLWidget(QGLFormat(QGL::SampleBuffers),p) {
   parent=p; acqM=acqm; ampNo=a; xRot=yRot=zRot=0; zTrans=CAMERA_DISTANCE; setMouseTracking(true); setAutoFillBackground(false); acqM->regRepaintGL(this);
  }

  ~HeadGLWidget() {
   makeCurrent(); //glDeleteLists(source,11);
   if (acqM->brainExists[ampNo]) glDeleteLists(brain,10);
   if (acqM->skullExists[ampNo]) glDeleteLists(skull,9);
   if (acqM->scalpExists[ampNo]) glDeleteLists(scalp,8);
   glDeleteLists(avgs,7);
   if (acqM->gizmoExists) glDeleteLists(gizmo,6);
   glDeleteLists(realistic,5);
   glDeleteLists(parametric,4);
   if (acqM->digExists[ampNo]) glDeleteLists(dig,3);
   glDeleteLists(grid,2);
   glDeleteLists(frame,1);
  }

  void setView(float theta,float phi) { setXRotation(0.); setYRotation(-16.*(-90.+theta)); setZRotation(-16*phi); setCameraLocation(ERP_ZOOM_Z); updateGL(); }
 
 public slots:
  void slotRepaintGL(int code) {
   if (code &   1) { glDeleteLists(dig,3); dig=makeDigitizer(); }
   if (code &   2) { glDeleteLists(parametric,4); parametric=makeParametric(); }
   if (code &   4) { glDeleteLists(realistic,5); realistic=makeRealistic(); }
   if (code &   8) { glDeleteLists(gizmo,6); gizmo=makeGizmo(); }
   if (code &  16) { glDeleteLists(avgs,7); avgs=makeAverages(); }
   if (code &  32) { glDeleteLists(scalp,8); scalp=makeScalp(); }
   if (code &  64) { glDeleteLists(skull,9); skull=makeSkull(); }
   if (code & 128) { glDeleteLists(brain,10); brain=makeBrain(); }
   updateGL();
  }

 protected:
  void initializeGL() {
   frame=makeFrame(); grid=makeGrid();
   if (acqM->digExists[ampNo]) dig=makeDigitizer();
   parametric=makeParametric(); realistic=makeRealistic();
   if (acqM->gizmoExists) gizmo=makeGizmo();
   avgs=makeAverages();
   if (acqM->scalpExists[ampNo]) scalp=makeScalp();
   if (acqM->skullExists[ampNo]) skull=makeSkull();
   if (acqM->brainExists[ampNo]) brain=makeBrain();

   static GLfloat ambientLight[4]={0.45,0.45,0.45,1.}; glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   static GLfloat diffuseLight[4]={0.45,0.45,0.45,1.}; glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);

   glLightf(GL_LIGHT0,GL_SPOT_CUTOFF,50.); glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,40.); // 80-degree cone
   static GLfloat matAmbient[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_AMBIENT,matAmbient);
   static GLfloat matDiffuse[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_DIFFUSE,matDiffuse);
   static GLfloat matSpecular[4]={1.,1.,1.,1.}; glMaterialfv(GL_FRONT,GL_SPECULAR,matSpecular);

   qglClearColor(QColor(120,120,150,255)); // Bluish White

   glEnable(GL_POLYGON_SMOOTH);

   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE); glEnable(GL_MULTISAMPLE); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); gluPerspective(CAMERA_FOV,(float)(acqM->gl3DGuiW)/(float)(acqM->gl3DGuiH),1.,300.);
   glMatrixMode(GL_MODELVIEW);
  }

  void paintGL() {
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
    lightPos[0]=lp[0]; lightPos[1]=lp[1]; lightPos[2]=lp[2]; glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

    static GLfloat spotDir[3]; Vec3 sd=lp; sd.normalize();
    spotDir[0]=-sd[0]; spotDir[1]=-sd[1]; spotDir[2]=-sd[2];
    glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,spotDir);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    if (acqM->hwFrameV[ampNo]) glCallList(frame);
    if (acqM->hwGridV[ampNo])  glCallList(grid);
    if (acqM->brainExists[ampNo] && acqM->hwBrainV[ampNo]) glCallList(brain);
    if (acqM->skullExists[ampNo] && acqM->hwSkullV[ampNo]) glCallList(skull);
    if (acqM->scalpExists[ampNo] && acqM->hwScalpV[ampNo]) glCallList(scalp);
    if (acqM->hwRealV[ampNo]) glCallList(realistic);
    if (acqM->hwParamV[ampNo]) glCallList(parametric);

    glDisable(GL_CULL_FACE);
     if (acqM->gizmoExists && acqM->hwGizmoV[ampNo]) glCallList(gizmo);
     if (acqM->hwAvgsV[ampNo]) glCallList(avgs);
    glEnable(GL_CULL_FACE);

    glDisable(GL_LIGHTING);
     if (acqM->digExists[ampNo] && acqM->hwDigV[ampNo]) glCallList(dig);
    glEnable(GL_LIGHTING);
   glPopMatrix();
  }

  void resizeGL(int width,int height) { int side=qMin(width,height);
   glViewport((width-side)/2,(height-side)/2,side,side);
   glMatrixMode(GL_PROJECTION); glLoadIdentity(); // glOrtho(-0.5,+0.5,+0.5,-0.5,4.0,15.0);
   gluPerspective(CAMERA_FOV,(float)(acqM->gl3DGuiW)/(float)(acqM->gl3DGuiH),2.,120.);
   glMatrixMode(GL_MODELVIEW);
  }

  void mousePressEvent(QMouseEvent *event) { eventPos=event->pos(); }

  void mouseMoveEvent(QMouseEvent *event) { int dx=event->x()-eventPos.x(); int dy=event->y()-eventPos.y();
   if (event->buttons() & Qt::LeftButton) { setYRotation(yRot+8*dy); setZRotation(zRot+8*dx); }
   else if (event->buttons() & Qt::RightButton) { setCameraLocation(zTrans-(float)(dy)/8.0); }
   else if (event->buttons() & Qt::MiddleButton) { ; //    setYRotation(xRot+8*dx);
   } eventPos=event->pos();
  }

 private:
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
   float elecR; if (chn==acqM->currentElectrode[ampNo]) elecR=eR*3; else elecR=eR;
   if (mode) qglColor(QColor(255,255,255,224));
   else qglColor(QColor(0,0,0,224));
   glPushMatrix(); glRotatef(phi,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix(); glTranslatef(0.,0.,r+ELECTRODE_HEIGHT*3./5.+0.0015);
     glPushMatrix(); glBegin(GL_POLYGON); for (float i=0.;i<2.*M_PI;i+=M_PI/40.) glVertex3f(elecR*cos(i),elecR*sin(i),0.); glEnd(); glPopMatrix();
    glPopMatrix();
   glPopMatrix();
  }

  void electrodeAvg(int chn,int idx,float r,float th,float ph,float eR) { float elecR,zPt,c,m;
   QVector<float> *data=&acqM->acqChannels[ampNo][chn]->avgData[idx]; int sz=data->size();
   QColor evtColor=acqM->acqEvents[idx]->color;
   //qDebug() << chn << idx << r << th << ph << eR << sz;
   //    for (int t=0;t<sz;t++)
   //     qDebug() << (*data)[t];
   if (chn==acqM->currentElectrode[ampNo]) elecR=eR*3; else elecR=eR;
   glPushMatrix();
    glRotatef(ph,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix();
     glTranslatef(0.0,0.0,r+ELECTRODE_HEIGHT*3./5.+0.002);
     glPushMatrix();
      glBegin(GL_LINES);
       qglColor(QColor(96,96,96,224)); glVertex3f(0.,-elecR,0.002); glVertex3f(0., elecR,0.002); // Amp Zeroline

       zPt=-elecR-2.*elecR*acqM->cp.avgBwd/(acqM->cp.avgFwd-acqM->cp.avgBwd);
       m=2.*elecR*100./(acqM->cp.avgFwd-acqM->cp.avgBwd);
       glVertex3f(-elecR/2.,zPt,0.002); glVertex3f( elecR/2.,zPt,0.002); // Time Zeroline & Amp marker

       c= acqM->cp.avgFwd/100.; for (int i=1;i<c;i++) { glVertex3f(-elecR/4.,zPt+m*(float)(i),0.002); glVertex3f( elecR/4.,zPt+m*(float)(i),0.002); } // Timeline
       c=-acqM->cp.avgBwd/100.; for (int i=1;i<c;i++) { glVertex3f(-elecR/4.,zPt-m*(float)(i),0.002); glVertex3f( elecR/4.,zPt-m*(float)(i),0.002); } // Timeline

       //qglColor(QColor(128,128,0,255)); // SL Cursor
       //zPt=-elecR+2.*elecR*(float)acqM->slTimePt/(float)acqM->cp.avgCount;
       //glVertex3f(-elecR/3,zPt,0.002); glVertex3f( elecR/3,zPt,0.002);

       qglColor(evtColor);
       for (int t=0;t<sz-1;t++) {
        glVertex3f(-(*data)[t  ]*2.*elecR/(acqM->avgAmpX[ampNo]*8.),-elecR+2.*elecR*(float)(t  )/(float)(sz),0.0025);
        glVertex3f(-(*data)[t+1]*2.*elecR/(acqM->avgAmpX[ampNo]*8.),-elecR+2.*elecR*(float)(t+1)/(float)(sz),0.0025);
       }
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
     for (int x=-15;x<16;x++) { glVertex3f((float)x,-15.,0.); glVertex3f((float)x, 15.,0.); glVertex3f(-15.,(float)x,0.); glVertex3f( 15.,(float)x,0.); }
    glEnd();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeDigitizer() { float x0,y0,z0,x1,y1,z1,x2,y2,z2,x3,y3,z3; GLuint list=glGenLists(3); // Digitizer current vectors
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glBegin(GL_LINES);
     qglColor(QColor(96,96,96,255)); // Blended gray
     x0=acqM->sty[0]; y0=acqM->sty[1]; z0=acqM->sty[2]; x1=acqM->xp[0]; y1=acqM->xp[1]; z1=acqM->xp[2];
     x2=acqM->yp[0];  y2=acqM->yp[1];  z2=acqM->yp[2];  x3=acqM->zp[0]; y3=acqM->zp[1]; z3=acqM->zp[2];
     qglColor(QColor(255,255,255,255)); glVertex3f(0.,0.,0.); glVertex3f(x0,y0,z0); // White
     qglColor(QColor(255, 64, 64,255)); glVertex3f(1.,1.,1.); glVertex3f(x1,y1,z1); // Red
     qglColor(QColor( 64,255, 64,255)); glVertex3f(1.,1.,1.); glVertex3f(x2,y2,z2); // Green
     qglColor(QColor( 64, 64,255,255)); glVertex3f(1.,1.,1.); glVertex3f(x3,y3,z3); // Blue
    glEnd();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeParametric() { float r,h; GLuint list=glGenLists(4); // Make parametric electrode/cap set..
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    for (int i=0;i<acqM->acqChannels[ampNo].size();i++) {
     r=ELECTRODE_RADIUS; h=ELECTRODE_HEIGHT;
     if (i==acqM->currentElectrode[ampNo]) { r=r*3; qglColor(QColor(255,255,255,144)); } // Hilite
     else qglColor(acqM->acqChannels[ampNo][i]->notchColor);
     electrode(acqM->scalpParamR[ampNo],acqM->acqChannels[ampNo][i]->param.y,acqM->acqChannels[ampNo][i]->param.z,r,h); // theta,phi
    } glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeRealistic() { float sdx,sdy,sdz,error; GLuint list=glGenLists(5); // Make realistic set..
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    for (int i=0;i<acqM->acqChannels[ampNo].size();i++) {
     if (i==acqM->currentElectrode[ampNo]) qglColor(QColor(255,255,255,128)); // Hilite
     else qglColor(acqM->acqChannels[ampNo][i]->notchColor);
     sdx=acqM->acqChannels[ampNo][i]->realS[0]; sdy=acqM->acqChannels[ampNo][i]->realS[1]; sdz=acqM->acqChannels[ampNo][i]->realS[2];
     error=sqrt(sdx*sdx+sdy*sdy+sdz*sdz);
     sphereXYZ(acqM->acqChannels[ampNo][i]->real[0],acqM->acqChannels[ampNo][i]->real[1],acqM->acqChannels[ampNo][i]->real[2],0.1+error);
    } glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeGizmo() { int v0,v1,v2; float v0c0S,v0c1S,v0c2S,v1c0S,v1c1S,v1c2S,v2c0S,v2c1S,v2c2S, v0c0C,v0c1C,v0c2C,v1c0C,v1c1C,v1c2C,v2c0C,v2c1C,v2c2C;
   GLuint list=glGenLists(6);
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glBegin(GL_TRIANGLES);
     for (int k=0;k<acqM->gizmo.size();k++) {
      if (k==acqM->currentGizmo[ampNo]) qglColor(QColor(255,0,0,96)); else qglColor(QColor(0,0,255,96)); // Hilite (red) vs. Red or blue - default cap color
      if (acqM->gizmoOnReal[ampNo]) {
       for (int i=0;i<acqM->gizmo[k]->tri.size();i++) { v0=v1=v2=0;
        for (int j=0;j<acqM->acqChannels[ampNo].size();j++) {
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][0]-1) v0=j;
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][1]-1) v1=j;
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][2]-1) v2=j;
        }
        v0c0S=acqM->acqChannels[ampNo][v0]->real[0]; v0c1S=acqM->acqChannels[ampNo][v0]->real[1]; v0c2S=acqM->acqChannels[ampNo][v0]->real[2];
        v1c0S=acqM->acqChannels[ampNo][v1]->real[0]; v1c1S=acqM->acqChannels[ampNo][v1]->real[1]; v1c2S=acqM->acqChannels[ampNo][v1]->real[2];
        v2c0S=acqM->acqChannels[ampNo][v2]->real[0]; v2c1S=acqM->acqChannels[ampNo][v2]->real[1]; v2c2S=acqM->acqChannels[ampNo][v2]->real[2];
        glVertex3f(v0c0S,v0c1S,v0c2S); glVertex3f(v1c0S,v1c1S,v1c2S); glVertex3f(v2c0S,v2c1S,v2c2S);
       }
      } else {
       for (int i=0;i<acqM->gizmo[k]->tri.size();i++) { v0=v1=v2=0;
        for (int j=0;j<acqM->acqChannels[ampNo].size();j++) {
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][0]-1) v0=j;
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][1]-1) v1=j;
         if (acqM->acqChannels[ampNo][j]->physChn==acqM->gizmo[k]->tri[i][2]-1) v2=j;
        }
        v0c0S=v1c0S=v2c0S=acqM->scalpParamR[ampNo];
        v0c1S=acqM->acqChannels[ampNo][v0]->param.y; v0c2S=acqM->acqChannels[ampNo][v0]->param.z; // theta,phi
        v1c1S=acqM->acqChannels[ampNo][v1]->param.y; v1c2S=acqM->acqChannels[ampNo][v1]->param.z;
        v2c1S=acqM->acqChannels[ampNo][v2]->param.y; v2c2S=acqM->acqChannels[ampNo][v2]->param.z;
        v0c0C=v0c0S*cos(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.); v0c1C=v0c0S*sin(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.); v0c2C=v0c0S*cos(2.*M_PI*v0c1S/360.);
        v1c0C=v1c0S*cos(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.); v1c1C=v1c0S*sin(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.); v1c2C=v1c0S*cos(2.*M_PI*v1c1S/360.);
        v2c0C=v2c0S*cos(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.); v2c1C=v2c0S*sin(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.); v2c2C=v2c0S*cos(2.*M_PI*v2c1S/360.);
        glVertex3f(v0c0C,v0c1C,v0c2C); glVertex3f(v1c0C,v1c1C,v1c2C); glVertex3f(v2c0C,v2c1C,v2c2C);
       }
      } 
     }
    glEnd();
    glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeAverages() { int evtIndex,chn; GLuint list=glGenLists(7);
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    for (int i=0;i<acqM->acqChannels[ampNo].size();i++) { chn=-1;

     for (int j=0;j<acqM->avgVisChns.size();j++) if (acqM->avgVisChns[ampNo][j]==i) { chn=i; break; }

     if (acqM->elecOnReal[ampNo]) { Vec3 dummyVec,vs;

      if (chn>=0) {
       dummyVec=acqM->acqChannels[ampNo][chn]->real;
       vs[0]=dummyVec.sphR(); vs[1]=dummyVec.sphTheta()*180./M_PI; vs[2]=dummyVec.sphPhi()*180./M_PI;
       electrodeBorder(true,chn,vs[0],vs[1],vs[2],ELECTRODE_RADIUS*5./6.);
       for (int j=0;j<acqM->acqEvents.size();j++) {
        if (acqM->acqEvents[j]->rejected || acqM->acqEvents[j]->accepted) {
         evtIndex=acqM->eventIndex(acqM->acqEvents[j]->no,acqM->acqEvents[j]->type),
         electrodeAvg(chn,evtIndex,vs[0],vs[1],vs[2],ELECTRODE_RADIUS*5./6.);
        }
       }
      } else electrodeBorder(false,i,vs[0],vs[1],vs[2],ELECTRODE_RADIUS*5./6.);

     } else {

      if (chn>=0) {
       electrodeBorder(true,chn,acqM->scalpParamR[ampNo],acqM->acqChannels[ampNo][chn]->param.y,acqM->acqChannels[ampNo][chn]->param.z,ELECTRODE_RADIUS*5./6.); // theta,phi
       for (int j=0;j<acqM->acqEvents.size();j++) {
        if (acqM->acqEvents[j]->rejected || acqM->acqEvents[j]->accepted) {
         evtIndex=acqM->eventIndex(acqM->acqEvents[j]->no,acqM->acqEvents[j]->type),
         electrodeAvg(chn,evtIndex,acqM->scalpParamR[ampNo],acqM->acqChannels[ampNo][chn]->param.y,acqM->acqChannels[ampNo][chn]->param.z,ELECTRODE_RADIUS*5./6.);
        }
       }
      } else electrodeBorder(false,i,acqM->scalpParamR[ampNo],acqM->acqChannels[ampNo][i]->param.y,acqM->acqChannels[ampNo][i]->param.z,ELECTRODE_RADIUS*5./6.);

     }

    } glDisable(GL_BLEND);
   glEndList(); return list;
  }

  GLuint makeScalp() { QVector<Coord3D> *c=&(acqM->scalpCoord); QVector<QVector<int> > *idx=&(acqM->scalpIndex); GLuint list=glGenLists(8); // Scalp derived from MR volume..
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glPushMatrix();
     qglColor(QColor(255,255,64,64)); // Yellow
//    glTranslatef(0.,acqM->scalpNasion,0.); glRotatef(-acqM->scalpCzAngle,1,0,0); glTranslatef(0.,-acqM->scalpNasion,0.);
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

  GLuint makeSkull() { QVector<Coord3D> *c=&(acqM->skullCoord); QVector<QVector<int> > *idx=&(acqM->skullIndex); GLuint list=glGenLists(9); // Skull derived from MR volume..
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glPushMatrix();
     qglColor(QColor(0,255,0,96)); // Cyan
//    glTranslatef(0.,acqM->scalpNasion,0.); glRotatef(-acqM->scalpCzAngle,1,0,0); glTranslatef(0.,-acqM->scalpNasion,0.);
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

  GLuint makeBrain() { QVector<Coord3D> *c=&(acqM->brainCoord); QVector<QVector<int> > *idx=&(acqM->brainIndex); GLuint list=glGenLists(10); // Brain derived from MR volume..
   glNewList(list,GL_COMPILE);
    glEnable(GL_BLEND);
    glPushMatrix();
     qglColor(QColor(255,64,64,128)); // Magenta
//    glTranslatef(0.,acqM->scalpNasion,0.); glRotatef(-acqM->scalpCzAngle,1,0,0); glTranslatef(0.,-acqM->scalpNasion,0.);
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

  void dipole(Vec3 v,float theta,float phi) {
   glPushMatrix();
    glTranslatef(v[0],v[1],v[2]);
    glPushMatrix(); glRotatef(phi,0,0,1); glRotatef(theta,0,1,0); dipoleArrow(0.3,0.1,1.); glPopMatrix();
   glPopMatrix();
  }

  void normalizeAngle(int *angle) {
   while (*angle < 0) *angle += 360*16;
   while (*angle > 360*16) *angle -= 360*16;
  }

  unsigned int ampNo;
  int xRot,yRot,zRot; float zTrans,frameAlpha,frameBeta,frameGamma; QPoint eventPos;
  QWidget *parent; AcqMaster *acqM; QPainter painter;
  GLuint frame,grid,dig,parametric,realistic,gizmo,avgs,scalp,skull,brain;
};

#endif

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
