/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

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

#include "octopus_loc_master.h"
#include "coord3d.h"
#include "../../common/vec3.h"

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
  HeadGLWidget(QWidget *pnt,LocMaster *lm) :
                                 QGLWidget(QGLFormat(QGL::SampleBuffers),pnt) {
   parent=pnt; p=lm; w=h=parent->height()-30; setGeometry(2,2,w,h);
   xRot=yRot=zRot=0; zTrans=CAMERA_DISTANCE;
   setMouseTracking(true);
   setAutoFillBackground(false);
   p->regRepaintGL(this);
  }

  ~HeadGLWidget() {
   makeCurrent();
   glDeleteLists(avgs,7);
   if (p->gizmoExists) glDeleteLists(gizmo,6);
   glDeleteLists(realistic,5);
   glDeleteLists(parametric,4);
   if (p->scalpExists) glDeleteLists(scalp,3);
   glDeleteLists(grid,2);
   glDeleteLists(frame,1);
  }

  void setView(float theta,float phi) {
   setXRotation(0.); setYRotation(-16.*(-90.+theta)); setZRotation(-16*phi);
   setCameraLocation(ERP_ZOOM_Z); updateGL();
  }
 
 public slots:
  void slotRepaintGL(int code) {
   if (code &  1) { glDeleteLists(scalp,3); scalp=makeScalp(); }
   if (code &  2) { glDeleteLists(parametric,3); parametric=makeParametric(); }
   if (code &  4) { glDeleteLists(realistic,3); realistic=makeRealistic(); }
   if (code &  8) { glDeleteLists(gizmo,3); gizmo=makeGizmo(); }
   if (code & 16) { glDeleteLists(avgs,3); avgs=makeAverages(); }
   updateGL();
  }

 protected:
  void initializeGL() {
   frame=makeFrame();
   grid=makeGrid();
   if (p->scalpExists) scalp=makeScalp();
   parametric=makeParametric();
   realistic=makeRealistic();
   if (p->gizmoExists) gizmo=makeGizmo();
   avgs=makeAverages();

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

   qglClearColor(QColor(120,120,150,255)); // Bluish White

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
     glMaterialf(GL_FRONT,GL_SHININESS,5.);

     static GLfloat lightPos[4]; Vec3 lp(zTrans,0.,0.);
     lp.rotX(2.*M_PI*xRot/360./16.);
     lp.rotY(2.*M_PI*yRot/360./16.);
     lp.rotZ(2.*M_PI*zRot/360./16.);
     lightPos[0]=lp[0]; lightPos[1]=lp[1]; lightPos[2]=lp[2];
      glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

     static GLfloat spotDir[3]; Vec3 sd=lp; sd.normalize();
     spotDir[0]=-sd[0]; spotDir[1]=-sd[1]; spotDir[2]=-sd[2];
     glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,spotDir);

     if (p->hwFrameV) glCallList(frame);
     if (p->hwGridV)  glCallList(grid);
     if (p->scalpExists && p->hwScalpV) glCallList(scalp);
     if (p->hwParamV) glCallList(parametric);
//     if (p->hwRealV) glCallList(realistic);
     glDisable(GL_CULL_FACE);
      if (p->gizmoExists && p->hwGizmoV) glCallList(gizmo);
     glEnable(GL_CULL_FACE);

//     glDisable(GL_LIGHTING);
//     if (p->hwAvgsV) glCallList(avgs);
//     glEnable(GL_LIGHTING);

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

  void makeSphereXYZ(float x,float y,float z,float elecR) {
   GLUquadricObj* q=gluNewQuadric(); glPushMatrix();
    glTranslatef(x,y,z);
    glPushMatrix(); gluSphere(q,elecR,10,10); glPopMatrix();
   glPopMatrix(); gluDeleteQuadric(q);
  }


  // *** SOPHISTICATED ONES ***

  GLuint makeFrame() {// Right-hand Orthogonal Axis
   GLuint list=glGenLists(1); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   qglColor(QColor(0,0,255,128)); // Z-axis -- blended blue
   glPushMatrix(); arrow(20.,1.5,0.1,0.3); glPopMatrix();
   qglColor(QColor(255,0,0,128)); // X-axis -- blended red
   glPushMatrix();
    glRotatef(90.,0,1,0); arrow(20.,1.5,0.1,0.3);
   glPopMatrix();
   qglColor(QColor(0,255,0,128)); // Y-axis -- blended green
   glPushMatrix();
    glRotatef(-90.,1,0,0); arrow(20.,1.5,0.1,0.3);
   glPopMatrix();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeGrid() { // XY-plane Grid
   GLuint list=glGenLists(2); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   glBegin(GL_LINES);
    qglColor(QColor(96,96,96,72)); // Blended gray
    for (int x=-15;x<16;x++) {
     glVertex3f((float)x,-15.,0.); glVertex3f((float)x, 15.,0.); // X-grid
     glVertex3f(-15.,(float)x,0.); glVertex3f( 15.,(float)x,0.); // Y-grid
    }
   glEnd();
   glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeScalp() { // Scalp derived from MR volume..
   QVector<Coord3D> *c=&(p->scalpCoord);
   QVector<QVector<int> > *idx=&(p->scalpIndex);
   GLuint list=glGenLists(3); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   glPushMatrix();
    qglColor(QColor(255,255,192,128)); // Yellow
//    glTranslatef(0.,p->scalpNasion,0.);
//    glRotatef(-p->scalpCzAngle,1,0,0);
//    glTranslatef(0.,-p->scalpNasion,0.);
    glBegin(GL_TRIANGLES);
     for (int i=0;i<idx->size();i++) {
      glVertex3f((*c)[(*idx)[i][0]].x,
                 (*c)[(*idx)[i][0]].y,(*c)[(*idx)[i][0]].z);
      glVertex3f((*c)[(*idx)[i][1]].x,
                 (*c)[(*idx)[i][1]].y,(*c)[(*idx)[i][1]].z);
      glVertex3f((*c)[(*idx)[i][2]].x,
                 (*c)[(*idx)[i][2]].y,(*c)[(*idx)[i][2]].z);
     }
    glEnd();
   glPopMatrix(); glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeParametric() { float r,h; // Make parametric electrode/cap set..
   GLuint list=glGenLists(4); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   for (int i=0;i<p->acqChannels.size();i++) {
    r=ELECTRODE_RADIUS; h=ELECTRODE_HEIGHT;
    if (i==p->currentElectrode) { r=r*3;
     qglColor(QColor(255,255,255,144)); // Hilite
    } else qglColor(QColor(0,255,0,144)); // Greenish
    electrode(p->scalpParamR,
              p->acqChannels[i]->param.y,      // theta
              p->acqChannels[i]->param.z,r,h); // phi
   } glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeRealistic() { float sdx,sdy,sdz,error; // Make realistic set..
   GLuint list=glGenLists(5); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   for (int i=0;i<p->acqChannels.size();i++) {
    if (i==p->currentElectrode) qglColor(QColor(255,255,255,128)); // Hilite
    else qglColor(QColor(0,255,0,128)); // Green
    sdx=p->acqChannels[i]->realS[0];
    sdy=p->acqChannels[i]->realS[1];
    sdz=p->acqChannels[i]->realS[2];
    error=sqrt(sdx*sdx+sdy*sdy+sdz*sdz);
    makeSphereXYZ(p->acqChannels[i]->real[0],
                  p->acqChannels[i]->real[1],
                  p->acqChannels[i]->real[2],0.1+error);
   } glDisable(GL_BLEND); glEndList(); return list;
  }

  GLuint makeGizmo() { int v0,v1,v2;
   float v0c0S,v0c1S,v0c2S,v1c0S,v1c1S,v1c2S,v2c0S,v2c1S,v2c2S;
   float v0c0C,v0c1C,v0c2C,v1c0C,v1c1C,v1c2C,v2c0C,v2c1C,v2c2C;
   GLuint list=glGenLists(6); glNewList(list,GL_COMPILE);

   glEnable(GL_BLEND);

   glBegin(GL_TRIANGLES);
    for (int k=0;k<p->gizmo.size();k++) {
     if (k==p->currentGizmo) qglColor(QColor(255,0,0,96)); // Hilite (red)
     else qglColor(QColor(0,0,255,96)); // Red or blue - default cap color
     if (p->gizmoOnReal) {
      for (int i=0;i<p->gizmo[k]->tri.size();i++) { v0=v1=v2=0;
       for (int j=0;j<p->acqChannels.size();j++) {
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][0]-1) v0=j;
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][1]-1) v1=j;
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][2]-1) v2=j;
       }
       v0c0S=p->acqChannels[v0]->real[0];
       v0c1S=p->acqChannels[v0]->real[1];
       v0c2S=p->acqChannels[v0]->real[2];
       v1c0S=p->acqChannels[v1]->real[0];
       v1c1S=p->acqChannels[v1]->real[1];
       v1c2S=p->acqChannels[v1]->real[2];
       v2c0S=p->acqChannels[v2]->real[0];
       v2c1S=p->acqChannels[v2]->real[1];
       v2c2S=p->acqChannels[v2]->real[2];
       glVertex3f(v0c0S,v0c1S,v0c2S);
       glVertex3f(v1c0S,v1c1S,v1c2S);
       glVertex3f(v2c0S,v2c1S,v2c2S);
      }
     } else {
      for (int i=0;i<p->gizmo[k]->tri.size();i++) { v0=v1=v2=0;
       for (int j=0;j<p->acqChannels.size();j++) {
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][0]-1) v0=j;
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][1]-1) v1=j;
        if (p->acqChannels[j]->physChn==p->gizmo[k]->tri[i][2]-1) v2=j;
       }
       v0c0S=v1c0S=v2c0S=p->scalpParamR;
       v0c1S=p->acqChannels[v0]->param.y; // theta
       v0c2S=p->acqChannels[v0]->param.z; // phi
       v1c1S=p->acqChannels[v1]->param.y;
       v1c2S=p->acqChannels[v1]->param.z;
       v2c1S=p->acqChannels[v2]->param.y;
       v2c2S=p->acqChannels[v2]->param.z;
       v0c0C=v0c0S*cos(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.);
       v0c1C=v0c0S*sin(2.*M_PI*v0c2S/360.)*sin(2.*M_PI*v0c1S/360.);
       v0c2C=v0c0S*cos(2.*M_PI*v0c1S/360.);
       v1c0C=v1c0S*cos(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.);
       v1c1C=v1c0S*sin(2.*M_PI*v1c2S/360.)*sin(2.*M_PI*v1c1S/360.);
       v1c2C=v1c0S*cos(2.*M_PI*v1c1S/360.);
       v2c0C=v2c0S*cos(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.);
       v2c1C=v2c0S*sin(2.*M_PI*v2c2S/360.)*sin(2.*M_PI*v2c1S/360.);
       v2c2C=v2c0S*cos(2.*M_PI*v2c1S/360.);
       glVertex3f(v0c0C,v0c1C,v0c2C);
       glVertex3f(v1c0C,v1c1C,v1c2C);
       glVertex3f(v2c0C,v2c1C,v2c2C);
      }
     } 
    }
   glEnd();

   glDisable(GL_BLEND);

   glEndList();
   return list;
  }

  void electrodeBorder(bool mode,int chn,float r,float th,float phi,float eR) {
   float elecR; if (chn==p->currentElectrode) elecR=eR*3; else elecR=eR;
   if (mode) qglColor(QColor(255,255,255,224));
   else qglColor(QColor(0,0,0,224));
   glPushMatrix(); glRotatef(phi,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix(); glTranslatef(0.,0.,r+ELECTRODE_HEIGHT*3./5.+0.0015);
     glPushMatrix(); glBegin(GL_POLYGON);
      for (float i=0.;i<2.*M_PI;i+=M_PI/40.)
       glVertex3f(elecR*cos(i),elecR*sin(i),0.);
     glEnd(); glPopMatrix();
    glPopMatrix();
   glPopMatrix();
  }

  void electrodeAvg(int chn,int idx,float r,float th,float ph,float eR) {
   QVector<float> *data=p->acqChannels[chn]->avgData[idx]; int sz=data->size();
   QColor evtColor=p->acqEvents[idx]->color;
   float elecR,zPt,c,m; if (chn==p->currentElectrode) elecR=eR*3; else elecR=eR;
   glPushMatrix(); glRotatef(ph,0,0,1); glRotatef(th,0,1,0);
    glPushMatrix(); glTranslatef(0.0,0.0,r+ELECTRODE_HEIGHT*3./5.+0.002);
     glPushMatrix();
      glBegin(GL_LINES);
       qglColor(QColor(96,96,96,224));
       glVertex3f(0.,-elecR,0.002); // Amp Zeroline
       glVertex3f(0., elecR,0.002); // Amp Zeroline
       zPt=-elecR-2.*elecR*p->cp.avgBwd/(p->cp.avgFwd-p->cp.avgBwd);
       m=2.*elecR*100./(p->cp.avgFwd-p->cp.avgBwd);
       glVertex3f(-elecR/2.,zPt,0.002); // Time Zeroline & Amp marker
       glVertex3f( elecR/2.,zPt,0.002);
       c=p->cp.avgFwd/100.;
       for (int i=1;i<c;i++) { // Timeline
        glVertex3f(-elecR/4.,zPt+m*(float)(i),0.002);
        glVertex3f( elecR/4.,zPt+m*(float)(i),0.002);
       }
       c=-p->cp.avgBwd/100.;
       for (int i=1;i<c;i++) { // Timeline
        glVertex3f(-elecR/4.,zPt-m*(float)(i),0.002);
        glVertex3f( elecR/4.,zPt-m*(float)(i),0.002);
       }
       qglColor(evtColor);
       for (int t=0;t<sz-1;t++) {
        glVertex3f(-(*data)[t]*2.*elecR/(p->avgAmpX*8.),
                   -elecR+2.*elecR*(float)(t)/(float)(sz),0.0025);
        glVertex3f(-(*data)[t+1]*2.*elecR/(p->avgAmpX*8.),
                   -elecR+2.*elecR*(float)(t+1)/(float)(sz),0.0025);
       }
      glEnd();
     glPopMatrix();
    glPopMatrix();
   glPopMatrix();
  }

  GLuint makeAverages() { int evtIndex,chn;
   GLuint list=glGenLists(7); glNewList(list,GL_COMPILE); glEnable(GL_BLEND);
   for (int i=0;i<p->acqChannels.size();i++) { chn=-1;
    for (int j=0;j<p->avgVisChns.size();j++)
     if (p->avgVisChns[j]==i) { chn=i; break; }
    if (chn>=0) {
     electrodeBorder(true,chn,p->scalpParamR,
                     p->acqChannels[chn]->param.y, // theta
                     p->acqChannels[chn]->param.z, // phi
                     ELECTRODE_RADIUS*5./6.);
     for (int j=0;j<p->acqEvents.size();j++) {
      if (p->acqEvents[j]->rejected || p->acqEvents[j]->accepted) {
       evtIndex=p->eventIndex(p->acqEvents[j]->no,p->acqEvents[j]->type),
       electrodeAvg(chn,evtIndex,p->scalpParamR,
                    p->acqChannels[chn]->param.y, // theta
                    p->acqChannels[chn]->param.z, // phi
                    ELECTRODE_RADIUS*5./6.);
      }
     }
    } else electrodeBorder(false,i,p->scalpParamR,
                           p->acqChannels[i]->param.y, // theta
                           p->acqChannels[i]->param.z, // phi
                           ELECTRODE_RADIUS*5./6.);
   } glDisable(GL_BLEND); glEndList(); return list;
  }

  void normalizeAngle(int *angle) {
   while (*angle < 0) *angle += 360*16;
   while (*angle > 360*16) *angle -= 360*16;
  }

  int xRot,yRot,zRot; float zTrans; QPoint eventPos;
  float frameAlpha,frameBeta,frameGamma;

  QWidget *parent; LocMaster *p; int w,h; QPainter painter;

  GLuint frame,grid,scalp,parametric,realistic,gizmo,avgs;
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
