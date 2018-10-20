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

#ifndef OCTOPUS_HEAD_GLWIDGET_H
#define OCTOPUS_HEAD_GLWIDGET_H

#include <QtGui>
#include <QtOpenGL>
#include <QGLWidget>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QMutex>
#include <cmath>
#include <GL/glu.h>

#include "octopus_seg_master.h"
#include "coord3d.h"
#include "../../common/vec3.h"

const int DSIZE=1000;

const float CAMERA_DISTANCE=40.0f;
const float CAMERA_FOV=70.0f;
const float MIN_Z=-200.0f;
const float MAX_Z=200.0f;
//const float ELECTRODE_WIDTH=0.4f;
//const float ELECTRODE_HEIGHT=0.3f;

class HeadGLWidget : public QGLWidget {
 Q_OBJECT
 public:
  HeadGLWidget(QWidget *pnt,SegMaster *sm) : QGLWidget(pnt) {
   parent=pnt; p=sm; w=parent->width()-5; h=parent->height()-60;
   setGeometry(2,2,w,h);

   xRot=yRot=zRot=0; zTrans=CAMERA_DISTANCE;
   setMouseTracking(true);	// Receive events also when mouse
  }

  ~HeadGLWidget() {
   if (p->scalpCoord.size()) glDeleteLists(scalp,3);
   glDeleteLists(grid,2);
   glDeleteLists(frame,1);
  }

  void repaintGL() {
   makeCurrent();
   if (p->brainCoord.size()) glDeleteLists(brain,5);
   if (p->skullCoord.size()) glDeleteLists(skull,4);
   if (p->scalpCoord.size()) glDeleteLists(scalp,3);
   glDeleteLists(grid,2);
   glDeleteLists(frame,1);
   frame=makeFrame();
   grid=makeGrid();
   if (p->scalpCoord.size()) scalp=makeScalp();
   if (p->skullCoord.size()) skull=makeSkull();
   if (p->brainCoord.size()) brain=makeBrain();
   updateGL();
  }

 public slots:
  void slotUpdate3D() { repaintGL(); }

 protected:
  void initializeGL() {
   qglClearColor(QColor(0,0,0,255)); // Black
   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glFrontFace(GL_CCW);
   glCullFace(GL_BACK);

   ambientLight[0]=ambientLight[1]=ambientLight[2]=0.45f;
   ambientLight[3]=1.0f;
   diffuseLight[0]=diffuseLight[1]=diffuseLight[2]=0.45f;
   diffuseLight[3]=1.0f;
   lightPosition[0]=0.0f; lightPosition[1]=180.0f; lightPosition[2]=0.0f;
   lightPosition[3]=1.0f;
   spotDirection[0]=0.0f; spotDirection[1]=-1.0f; spotDirection[2]=0.0f;

   matAmbient[0]=matAmbient[1]=matAmbient[2]=1.0f;
   matAmbient[3]=1.0f;
   matDiffuse[0]=matDiffuse[1]=matDiffuse[2]=1.0f;
   matDiffuse[3]=1.0f;
   matSpecular[0]=matSpecular[1]=matSpecular[2]=1.0f;
   matSpecular[3]=1.0f;

   glEnable(GL_LIGHTING);
   glMaterialfv(GL_FRONT,GL_AMBIENT,matAmbient);
   glMaterialfv(GL_FRONT,GL_DIFFUSE,matDiffuse);
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_POSITION,lightPosition);

   glLightf(GL_LIGHT0,GL_SPOT_CUTOFF,50.0f); // 80-degree cone
   glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,40.0f);
   glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,spotDirection);

//   glEdgeFlag(GL_FALSE);
//   glEnable(GL_POLYGON_SMOOTH);
   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
//   glPolygonMode(GL_BACK,GL_LINES);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluLookAt(zTrans,0.0f,0.0f,
             0.0f,0.0f,0.0f,
             0.0f,0.0f,1.0f);
   gluPerspective(CAMERA_FOV,(float)w/(float)h,1.0f,300.0f);

   glMatrixMode(GL_MODELVIEW);
   frame=makeFrame();
   grid=makeGrid();
   if (p->scalpCoord.size()) scalp=makeScalp();
   if (p->skullCoord.size()) skull=makeSkull();
   if (p->brainCoord.size()) brain=makeBrain();
  }

  void paintGL() {
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

   glLoadIdentity();
   gluLookAt(zTrans,0.0f,0.0f,
             0.0f,0.0f,0.0f,
             0.0f,0.0f,1.0f);

   glRotated(xRot/16.0,1.0,0.0,0.0);
   glRotated(yRot/16.0,0.0,1.0,0.0);
   glRotated(zRot/16.0,0.0,0.0,1.0);

   glPushMatrix();

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
    glDisable(GL_LIGHTING);
    glCallList(frame);
    glCallList(grid);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

    glMaterialfv(GL_FRONT,GL_SPECULAR,matSpecular);
    glMaterialf(GL_FRONT,GL_SHININESS,4.0f);

//    glPushMatrix();
//    glTranslatef(headOrigin.x,headOrigin.y,headOrigin.z); // Translate along r
//    glTranslatef(0.0,0.0,0.5);
//     glRotated(frameGamma,0.0,0.0,1.0);
//     glRotated(frameBeta,0.0,1.0,0.0);
//     glRotated(frameAlpha,1.0,0.0,0.0);
//     glCallList(head);
//     glCallList(electrodes);
//     glCallList(erpData);
    if (p->scalpCoord.size()) glCallList(scalp);
    if (p->skullCoord.size()) glCallList(skull);
    if (p->brainCoord.size()) glCallList(brain);
//    glPopMatrix();

   glPopMatrix();
  }

  void resizeGL(int ww,int hh) {
   w=ww; h=hh;

   int side=qMin(w,h);
   glViewport((w-side)/2,(h-side)/2,side,side);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(CAMERA_FOV,(float)w/(float)h,2.0f,120.0f);

   glMatrixMode(GL_MODELVIEW);
  }

  void mousePressEvent(QMouseEvent *event) {
   eventPos=event->pos();
  }

  void mouseMoveEvent(QMouseEvent *event) {
   int dx=event->x()-eventPos.x();
   int dy=event->y()-eventPos.y();
   if (event->buttons() & Qt::LeftButton) {
    setYRotation(yRot+8*dy);
    setZRotation(zRot+8*dx);
   } else if (event->buttons() & Qt::RightButton) {
    setCameraLocation(zTrans-(float)(dy)/8.0);
   } else if (event->buttons() & Qt::MidButton) {
; //   setYRotation(xRot+8*dx);
   }
   eventPos=event->pos();
  }

 private:
  void setXRotation(int angle) {
   normalizeAngle(&angle);
   if (angle!=xRot) {
    xRot=angle; updateGL();
   }
  }
  void setYRotation(int angle) {
   normalizeAngle(&angle);
   if (angle!=yRot) {
    yRot=angle; updateGL();
   }
  }
  void setZRotation(int angle) {
   normalizeAngle(&angle);
   if (angle!=zRot) {
    zRot=angle; updateGL();
   }
  }
  void setCameraLocation(float z) {
   if (z<MIN_Z) zTrans=MIN_Z;
   else if (z>=MAX_Z) zTrans=MAX_Z;
   else zTrans=z;
   updateGL();
  }

  void makeArrow(GLfloat arrowLength,
                 GLfloat headLength,
                 GLfloat baseWidth,
                 GLfloat headWidth) {
   GLUquadricObj* quadAx=gluNewQuadric();
   glPushMatrix();
    gluCylinder(quadAx,baseWidth,baseWidth,arrowLength-headLength,10,1);
    glTranslatef(0,0,arrowLength-headLength); // Original is in +Z direction
    gluCylinder(quadAx,headWidth,0,headLength,10,1);
   glPopMatrix();
   gluDeleteQuadric(quadAx);
  }

  GLuint makeFrame() {		// Right-hand Orthogonal Axis
   GLuint list=glGenLists(1);
   glNewList(list,GL_COMPILE);

   glEnable(GL_BLEND);
//   glEnable(GL_NORMALIZE);
   qglColor(QColor(0,0,255,144));	// Z-axis -- blended blue
   glPushMatrix();
    makeArrow(20.0f,1.5f,0.1f,0.3f);
   glPopMatrix();

   qglColor(QColor(255,0,0,144));	// X-axis -- blended red
   glPushMatrix();
    glRotatef(90,0,1,0);
    makeArrow(20.0f,1.5f,0.1f,0.3f);
   glPopMatrix();

   qglColor(QColor(0,255,0,144));	// Y-axis -- blended green
   glPushMatrix();
    glRotatef(-90,1,0,0);
    makeArrow(20.0f,1.5f,0.1f,0.3f);
   glPopMatrix();

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  GLuint makeGrid() {		// XY-plane Grid
   GLuint list=glGenLists(2);
   glNewList(list,GL_COMPILE);
  
   qglColor(QColor(128,128,128,144)); // Blended gray

   glEnable(GL_BLEND);
   glBegin(GL_LINES);
   for (int x=-15;x<16;x++) {
    glVertex3f((float)x,-15.0f,0.0f);	// X-grid
    glVertex3f((float)x,+15.0f,0.0f);
    glVertex3f(-15.0f,(float)x,0.0f);	// Y-grid
    glVertex3f(+15.0f,(float)x,0.0f);
   }
   glEnd();

   qglColor(QColor(64,64,64,144)); // Blended gray

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  GLuint makeScalp() {		// Scalp derived from MR image set..
   QVector<Coord3D> *c=&(p->scalpCoord);
   QVector<QVector<int> > *idx=&(p->scalpIndex);
   GLuint list=glGenLists(3);
   glNewList(list,GL_COMPILE);
  
   qglColor(QColor(255,255,128,128)); // Yellow

   glEnable(GL_BLEND);

   glBegin(GL_TRIANGLES);
    for (int i=0;i<idx->size();i++) {
     glVertex3f((*c)[(*idx)[i][0]].x,(*c)[(*idx)[i][0]].y,(*c)[(*idx)[i][0]].z);
     glVertex3f((*c)[(*idx)[i][1]].x,(*c)[(*idx)[i][1]].y,(*c)[(*idx)[i][1]].z);
     glVertex3f((*c)[(*idx)[i][2]].x,(*c)[(*idx)[i][2]].y,(*c)[(*idx)[i][2]].z);
    }
   glEnd();

//   glBegin(GL_POINTS);
//   for (int n=0;n<p->scalpCoord.size();n++)
//    glVertex3f(p->scalpCoord[n].x,p->scalpCoord[n].y,p->scalpCoord[n].z);
//   glEnd();

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  GLuint makeSkull() {		// Skull derived from MR image set..
   QVector<Coord3D> *c=&(p->skullCoord);
   QVector<QVector<int> > *idx=&(p->skullIndex);
   GLuint list=glGenLists(4);
   glNewList(list,GL_COMPILE);
  
   qglColor(QColor(255,128,255,128));

   glEnable(GL_BLEND);

   glBegin(GL_TRIANGLES);
    for (int i=0;i<idx->size();i++) {
     glVertex3f((*c)[(*idx)[i][0]].x,(*c)[(*idx)[i][0]].y,(*c)[(*idx)[i][0]].z);
     glVertex3f((*c)[(*idx)[i][1]].x,(*c)[(*idx)[i][1]].y,(*c)[(*idx)[i][1]].z);
     glVertex3f((*c)[(*idx)[i][2]].x,(*c)[(*idx)[i][2]].y,(*c)[(*idx)[i][2]].z);
    }
   glEnd();

//   glBegin(GL_POINTS);
//   for (int n=0;n<p->skullCoord.size();n++)
//    glVertex3f(p->skullCoord[n].x,p->skullCoord[n].y,p->skullCoord[n].z);
//   glEnd();

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  GLuint makeBrain() {		// Brain derived from MR image set..
   QVector<Coord3D> *c=&(p->brainCoord);
   QVector<QVector<int> > *idx=&(p->brainIndex);
   GLuint list=glGenLists(5);
   glNewList(list,GL_COMPILE);
  
   qglColor(QColor(255,0,0,128)); // Red

   glEnable(GL_BLEND);

   glBegin(GL_TRIANGLES);
    for (int i=0;i<idx->size();i++) {
     glVertex3f((*c)[(*idx)[i][0]].x,(*c)[(*idx)[i][0]].y,(*c)[(*idx)[i][0]].z);
     glVertex3f((*c)[(*idx)[i][1]].x,(*c)[(*idx)[i][1]].y,(*c)[(*idx)[i][1]].z);
     glVertex3f((*c)[(*idx)[i][2]].x,(*c)[(*idx)[i][2]].y,(*c)[(*idx)[i][2]].z);
    }
   glEnd();

//   glBegin(GL_POINTS);
//   for (int n=0;n<p->brainCoord.size();n++)
//    glVertex3f(p->brainCoord[n].x,p->brainCoord[n].y,p->brainCoord[n].z);
//   glEnd();

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  void normalizeAngle(int *angle) {
   while (*angle < 0) *angle += 360*16;
   while (*angle > 360*16) *angle -= 360*16;
  }

  // Lighting and Materials
  float ambientLight[4],diffuseLight[4],lightPosition[4],spotDirection[3];
  float matAmbient[4],matDiffuse[4],matSpecular[4];
  int xRot,yRot,zRot; float zTrans; QPoint eventPos;
  float frameAlpha,frameBeta,frameGamma;

  QWidget *parent; SegMaster *p; int w,h;

  GLuint frame,grid,scalp,skull,brain;
};

#endif
