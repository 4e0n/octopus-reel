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

#include "octopus_bem_master.h"
#include "../../../common/mesh.h"
//#include "../../../common/vec3.h"

const float LINEWIDTH=0.0;
const float POINTSIZE=0.0;

const int DSIZE=1000;

const float CAMERA_DISTANCE=40.0f;
const float CAMERA_FOV=70.0f;
const float MIN_Z=4.0f;
const float MAX_Z=100.0f;
const float ZOOM_Z=26.0f;
//const float ELECTRODE_WIDTH=0.4f;
//const float ELECTRODE_HEIGHT=0.3f;

class HeadGLWidget : public QGLWidget {
 Q_OBJECT
 public:
  HeadGLWidget(QWidget *pnt,BEMMaster *rm) :
                                 QGLWidget(QGLFormat(QGL::SampleBuffers),pnt) {
   parent=pnt; p=rm; w=h=parent->height()-30; setGeometry(2,2,w,h);
   xRot=yRot=zRot=0; zTrans=CAMERA_DISTANCE;
   setMouseTracking(true); setAutoFillBackground(false); p->regRepaintGL(this);
  }

  ~HeadGLWidget() { makeCurrent();
   glDeleteLists(brain,7); glDeleteLists(skull,6); glDeleteLists(scalp,5);
   glDeleteLists(model,4); glDeleteLists(field,4);
   glDeleteLists(grid,2); glDeleteLists(frame,1);
  }

  void setView(float theta,float phi) {
   setXRotation(0.); setYRotation(-16.*(-90.+theta)); setZRotation(-16*phi);
   setCameraLocation(ZOOM_Z); updateGL();
  }
 
 public slots:
  void slotRepaintGL(int code) {
    if (code &  1) { glDeleteLists(field,3); field=makeField(); }
    if (code &  2) { glDeleteLists(model,4); model=makeModel(); }
    if (code &  4) { glDeleteLists(scalp,5); scalp=makeScalp(); }
    if (code &  8) { glDeleteLists(skull,6); skull=makeSkull(); }
    if (code & 16) { glDeleteLists(brain,7); brain=makeBrain(); }
    updateGL();
  }

 protected:
  void initializeGL() {
   frame=makeFrame(); grid=makeGrid();
   field=makeField(); model=makeModel();
   scalp=makeScalp(); skull=makeSkull(); brain=makeBrain();

   static GLfloat ambientLight[4]={0.45,0.45,0.45,1.};
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);

   static GLfloat diffuseLight[4]={0.45,0.45,0.45,1.};
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);

   glLightf(GL_LIGHT0,GL_SPOT_CUTOFF,50.); // 80-degree cone
   glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,40.);

   static GLfloat matAmbient[4]={1.,1.,1.,1.};
   glMaterialfv(GL_FRONT,GL_AMBIENT,matAmbient);

   static GLfloat matDiffuse[4]={1.,1.,1.,1.};
   glMaterialfv(GL_FRONT,GL_DIFFUSE,matDiffuse);

   static GLfloat matSpecular[4]={1.,1.,1.,1.};
   glMaterialfv(GL_FRONT,GL_SPECULAR,matSpecular);

   qglClearColor(QColor(192,192,255,255)); // Bluish white

   glEnable(GL_POLYGON_SMOOTH);
   glShadeModel(GL_SMOOTH);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_MULTISAMPLE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); // GL_POINT may be cool!
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); gluPerspective(CAMERA_FOV,(float)w/(float)h,1.,300.);
   glMatrixMode(GL_MODELVIEW);
  }

//  void paintEvent(QPaintEvent*) {
  void paintGL() {
   makeCurrent(); // glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity(); gluLookAt(zTrans,0.,0.,0.,0.,0.,0.,0.,1.);
    glRotated(xRot/16.,1.,0.,0.);
    glRotated(yRot/16.,0.,1.,0.);
    glRotated(zRot/16.,0.,0.,1.);

    glPushMatrix();
     glEnable(GL_COLOR_MATERIAL);
     glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
     glMaterialf(GL_FRONT,GL_SHININESS,4.);

     static GLfloat lightPos[4]; Vec3 lp(zTrans*20.,0.,0.);
     lp.rotX(2.*M_PI*xRot/360./16.);
     lp.rotY(2.*M_PI*yRot/360./16.);
     lp.rotZ(2.*M_PI*zRot/360./16.);
     lightPos[0]=lp[0]; lightPos[1]=lp[1]; lightPos[2]=lp[2];
     glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

     static GLfloat spotDir[3]; Vec3 sd=lp; sd.normalize();
     spotDir[0]=-sd[0]; spotDir[1]=-sd[1]; spotDir[2]=-sd[2];
     glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,spotDir);
     glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

     glShadeModel(GL_SMOOTH);
     glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

     glEnable(GL_LIGHTING);

     if (p->glFrameV) glCallList(frame);
     if (p->glGridV) glCallList(grid);
     if (p->glFieldV) glCallList(field);

     glShadeModel(GL_LINES);
     glPolygonMode(GL_FRONT,GL_LINE);

     glDisable(GL_LIGHTING);

     if (p->glModelV) glCallList(model);
     if (p->glScalpV)
     glCallList(scalp);
     if (p->glSkullV) glCallList(skull);
     if (p->glBrainV) glCallList(brain);

    glPopMatrix();
//    glMatrixMode(GL_MODELVIEW);

//   QPainter painter(this);
//   painter.setRenderHint(QPainter::Antialiasing);
//   painter.setPen(Qt::red);
//   painter.drawText(40,40,"Deneme");
//   painter.end();
  }

  void resizeGL(int width,int height) { int side=qMin(width,height);
   glViewport((width-side)/2, (height-side)/2,side,side);
   glMatrixMode(GL_PROJECTION); glLoadIdentity();

//   glOrtho(-0.5,+0.5,+0.5,-0.5,4.0,15.0);
   gluPerspective(CAMERA_FOV,(float)w/(float)h,2.,120.);

   glMatrixMode(GL_MODELVIEW);
  }

  void mousePressEvent(QMouseEvent *event) { eventPos=event->pos(); }

  void mouseMoveEvent(QMouseEvent *event) { int dx,dy;
   dx=event->x()-eventPos.x(); dy=event->y()-eventPos.y();
   if (event->buttons() & Qt::LeftButton) {
    setYRotation(yRot+8*dy); setZRotation(zRot+8*dx);
   } else if (event->buttons() & Qt::RightButton) {
    setCameraLocation(zTrans-(float)(dy)/8.0);
   } else if (event->buttons() & Qt::MidButton) { ;
//    setYRotation(xRot+8*dx);
   } eventPos=event->pos();
  }


 private:
  void setXRotation(int angle) { normalizeAngle(&angle);
   if (angle!=xRot) { xRot=angle; updateGL(); }
  }
  void setYRotation(int angle) { normalizeAngle(&angle);
   if (angle!=yRot) { yRot=angle; updateGL(); }
  }
  void setZRotation(int angle) { normalizeAngle(&angle);
   if (angle!=zRot) { zRot=angle; updateGL(); }
  }
  void setCameraLocation(float z) {
   if (z<MIN_Z) zTrans=MIN_Z; else if (z>=MAX_Z) zTrans=MAX_Z; else zTrans=z;
   updateGL();
  }


  // *** BASIC SHAPE LIBRARY ***

  void arrow(float totalL,float headL,float baseW,float headW) {
   GLUquadricObj* q=gluNewQuadric(); glPushMatrix();
    gluCylinder(q,baseW,baseW,totalL-headL,16,1);
    glTranslatef(0,0,totalL-headL); // Original is in +Z direction
    gluCylinder(q,headW,0,headL,16,1);
   glPopMatrix(); gluDeleteQuadric(q);
  }

  // *** LIST OBJECTS ***

  GLuint makeFrame() {// Right-hand Orthogonal Axis
   GLuint list=glGenLists(1); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   qglColor(QColor(0,0,255,192)); // Z-axis -- blended blue
   glPushMatrix(); arrow(20.,1.5,0.1,0.3); glPopMatrix();
   qglColor(QColor(255,0,0,192)); // X-axis -- blended red
   glPushMatrix();
    glRotatef(90.,0,1,0); arrow(20.,1.5,0.1,0.3);
   glPopMatrix();
   qglColor(QColor(0,255,0,192)); // Y-axis -- blended green
   glPushMatrix();
    glRotatef(-90.,1,0,0); arrow(20.,1.5,0.1,0.3);
   glPopMatrix();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeGrid() { // XY-plane Grid
   GLuint list=glGenLists(2); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   qglColor(QColor(128,128,128,128)); // Blended gray
   glBegin(GL_LINES);
    for (int x=-15;x<16;x++) {
     glVertex3f((float)x,-15.,0.); glVertex3f((float)x, 15.,0.); // X-grid
     glVertex3f(-15.,(float)x,0.); glVertex3f( 15.,(float)x,0.); // Y-grid
    }
   glEnd(); glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeField() { // Gradient Vector Flow Field Sample
   int i,j,k,xs,ys,zs,xs2,ys2,zs2; float tx,ty,tz;
   Vol3<Vec3> *fld=p->vecField; Vec3 vv; float theta,phi;
   GLuint list=glGenLists(3); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   qglColor(QColor(96,96,16,192)); // dark gray

   p->mutex.lock();
   if (p->fldGran<=20 && fld->xSize>=p->fldGran*2 &&
       fld->ySize>=p->fldGran*2 && fld->zSize>=p->fldGran*2) {
    xs=(int)((float)fld->xSize/(float)p->fldGran); xs2=xs/2;
    ys=(int)((float)fld->ySize/(float)p->fldGran); ys2=ys/2;
    zs=(int)((float)fld->zSize/(float)p->fldGran); zs2=zs/2;

    for (k=zs2;k<fld->zSize;k+=zs) for (j=ys2;j<fld->ySize;j+=ys)
    for (i=xs2;i<fld->xSize;i+=xs) {
     tx=(float)(i-fld->xSize/2)*0.125;
     ty=(float)(j-fld->ySize/2)*0.125;
     tz=(float)(k-fld->zSize/2)*0.125;
     vv=fld->data[k][j][i];
     theta=vv.sphTheta()*180./M_PI;
     phi=vv.sphPhi()*180./M_PI;
    
     glPushMatrix(); glTranslatef(tx,ty,tz);
      glPushMatrix(); glRotatef(phi,0,0,1); glRotatef(theta,0,1,0);
       glPushMatrix(); glTranslatef(0,0,-0.5);
        arrow(1.0,0.3,0.06,0.18);
       glPopMatrix();
      glPopMatrix();
     glPopMatrix();
    }
   }
   p->mutex.unlock();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeModel() { GLfloat a[3],b[3],c[3]; GLuint list=glGenLists(4);
   glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   qglColor(QColor(255,255,255,128));
   glBegin(GL_TRIANGLES); // Triangled Mesh
    for (int i=0;i<p->model->f.size();i++) {
     int x0=p->model->f[i].v[0];
     int x1=p->model->f[i].v[1];
     int x2=p->model->f[i].v[2];
     a[0]=(GLfloat)(p->model->v[x0].r[0]);
     a[1]=(GLfloat)(p->model->v[x0].r[1]);
     a[2]=(GLfloat)(p->model->v[x0].r[2]);
     b[0]=(GLfloat)(p->model->v[x1].r[0]);
     b[1]=(GLfloat)(p->model->v[x1].r[1]);
     b[2]=(GLfloat)(p->model->v[x1].r[2]);
     c[0]=(GLfloat)(p->model->v[x2].r[0]);
     c[1]=(GLfloat)(p->model->v[x2].r[1]);
     c[2]=(GLfloat)(p->model->v[x2].r[2]);
     glNormal3fv(c); glVertex3fv(c);
     glNormal3fv(b); glVertex3fv(b);
     glNormal3fv(a); glVertex3fv(a);
    }
   glEnd();
   glDisable(GL_BLEND); glEndList(); return list;
  }




  GLuint makeScalp() { GLfloat a[3],b[3],c[3];
   GLuint list=glGenLists(5); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   p->mutex.lock();
   qglColor(QColor(128,128,0,192));
   glBegin(GL_TRIANGLES); // Triangled Mesh
    for (int i=0;i<p->defScalp->f.size();i++) {
     int x0=p->defScalp->f[i].v[0];
     int x1=p->defScalp->f[i].v[1];
     int x2=p->defScalp->f[i].v[2];
     a[0]=(GLfloat)(p->defScalp->v[x0].r[0]);
     a[1]=(GLfloat)(p->defScalp->v[x0].r[1]);
     a[2]=(GLfloat)(p->defScalp->v[x0].r[2]);
     b[0]=(GLfloat)(p->defScalp->v[x1].r[0]);
     b[1]=(GLfloat)(p->defScalp->v[x1].r[1]);
     b[2]=(GLfloat)(p->defScalp->v[x1].r[2]);
     c[0]=(GLfloat)(p->defScalp->v[x2].r[0]);
     c[1]=(GLfloat)(p->defScalp->v[x2].r[1]);
     c[2]=(GLfloat)(p->defScalp->v[x2].r[2]);
     glNormal3fv(c); glVertex3fv(c);
     glNormal3fv(b); glVertex3fv(b);
     glNormal3fv(a); glVertex3fv(a);
    }
   glEnd();
   p->mutex.unlock();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeSkull() { GLfloat a[3],b[3],c[3];
   GLuint list=glGenLists(6); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   p->mutex.lock();
   qglColor(QColor(128,128,0,192));
   glBegin(GL_TRIANGLES); // Triangled Mesh
    for (int i=0;i<p->defSkull->f.size();i++) {
     int x0=p->defSkull->f[i].v[0];
     int x1=p->defSkull->f[i].v[1];
     int x2=p->defSkull->f[i].v[2];
     a[0]=(GLfloat)(p->defSkull->v[x0].r[0]);
     a[1]=(GLfloat)(p->defSkull->v[x0].r[1]);
     a[2]=(GLfloat)(p->defSkull->v[x0].r[2]);
     b[0]=(GLfloat)(p->defSkull->v[x1].r[0]);
     b[1]=(GLfloat)(p->defSkull->v[x1].r[1]);
     b[2]=(GLfloat)(p->defSkull->v[x1].r[2]);
     c[0]=(GLfloat)(p->defSkull->v[x2].r[0]);
     c[1]=(GLfloat)(p->defSkull->v[x2].r[1]);
     c[2]=(GLfloat)(p->defSkull->v[x2].r[2]);
     glNormal3fv(c); glVertex3fv(c);
     glNormal3fv(b); glVertex3fv(b);
     glNormal3fv(a); glVertex3fv(a);
    }
   glEnd();
   p->mutex.unlock();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeBrain() { GLfloat a[3],b[3],c[3];
   GLuint list=glGenLists(7); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   p->mutex.lock();
   qglColor(QColor(128,128,0,192));
   glBegin(GL_TRIANGLES); // Triangled Mesh
    for (int i=0;i<p->defBrain->f.size();i++) {
     int x0=p->defBrain->f[i].v[0];
     int x1=p->defBrain->f[i].v[1];
     int x2=p->defBrain->f[i].v[2];
     a[0]=(GLfloat)(p->defBrain->v[x0].r[0]);
     a[1]=(GLfloat)(p->defBrain->v[x0].r[1]);
     a[2]=(GLfloat)(p->defBrain->v[x0].r[2]);
     b[0]=(GLfloat)(p->defBrain->v[x1].r[0]);
     b[1]=(GLfloat)(p->defBrain->v[x1].r[1]);
     b[2]=(GLfloat)(p->defBrain->v[x1].r[2]);
     c[0]=(GLfloat)(p->defBrain->v[x2].r[0]);
     c[1]=(GLfloat)(p->defBrain->v[x2].r[1]);
     c[2]=(GLfloat)(p->defBrain->v[x2].r[2]);
     glNormal3fv(c); glVertex3fv(c);
     glNormal3fv(b); glVertex3fv(b);
     glNormal3fv(a); glVertex3fv(a);
    }
   glEnd();
   p->mutex.unlock();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  void normalizeAngle(int *angle) {
   while (*angle < 0) *angle += 360*16;
   while (*angle > 360*16) *angle -= 360*16;
  }

  int xRot,yRot,zRot; float zTrans; QPoint eventPos;
  float frameAlpha,frameBeta,frameGamma;
  QWidget *parent; BEMMaster *p; int w,h; QPainter painter;
  GLuint frame,grid,field,model,scalp,skull,brain;
};

#endif
/*
   for (i=0;i<p->scalp->sf.size();i++) {
    glBegin(GL_TRIANGLE_FAN);
//    glBegin(GL_POINTS);
     // Find center
     c[0]=c[1]=c[2]=0.;
     for (j=0;j<p->scalp->sf[i]->size();j++) {
      c[0]+=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[j]].r[0]);
      c[1]+=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[j]].r[1]);
      c[2]+=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[j]].r[2]);
     }
     c[0]/=(GLfloat)(p->scalp->sf[i]->size());
     c[1]/=(GLfloat)(p->scalp->sf[i]->size());
     c[2]/=(GLfloat)(p->scalp->sf[i]->size());
     glNormal3fv(c);
     glVertex3fv(c);
     for (j=0;j<=p->scalp->sf[i]->size();j++) {
      k=p->scalp->sf[i]->size()-(j%p->scalp->sf[i]->size())-1;
      a[0]=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[k]].r[0]);
      a[1]=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[k]].r[1]);
      a[2]=(GLfloat)(p->scalp->sv[(*(p->scalp->sf[i]))[k]].r[2]);
      glNormal3fv(a);
      glVertex3fv(a);
     }
    glEnd();
   }
*/
