#ifndef OCTOPUS_MESH_H
#define OCTOPUS_MESH_H

#include <QVector>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <iostream>

#include "octopus_wing.h"

class Mesh {
 public:
  Mesh() {}

  Mesh(int rank) { setIcosa();
   qDebug("Creating deformable mesh prototype.. please wait..");
   for (int i=0;i<rank;i++) subdivide(); computeSimplex();
   qDebug("Icosa(%d)+Simplex generated.. V:%d E:%d F:%d W:%d N:%d",rank,
                           v.size(),e.size(),f.size(),w.size(),n.size());
  }

  Mesh(Mesh *m) { QVector<int> *d; int i,j; clear();
   for (i=0;i<m->v.size();i++) v.append(m->v[i]);
   for (i=0;i<m->e.size();i++) e.append(m->e[i]);
   for (i=0;i<m->f.size();i++) f.append(m->f[i]);
   for (i=0;i<m->w.size();i++) w.append(m->w[i]);
   for (i=0;i<m->sv.size();i++) sv.append(m->sv[i]);
   for (i=0;i<m->sn.size();i++) sn.append(m->sn[i]);
   for (i=0;i<m->n.size();i++) { d=new QVector<int>();
    for (j=0;j<m->n[i]->size();j++) d->append((*(m->n[i]))[j]); n.append(d);
   }
   for (i=0;i<m->sf.size();i++) { d=new QVector<int>();
    for (j=0;j<m->sf[i]->size();j++) d->append((*(m->sf[i]))[j]); sf.append(d);
   }
  }

  ~Mesh() { clear(); }

  void setRadius(float x,float y,float z) { int i; normalizeV();
   for (i=0;i<v.size();i++) { v[i].r[0]*=x; v[i].r[1]*=y; v[i].r[2]*=z; }
   computeSimplex();
   for (i=0;i<sv.size();i++) { sv[i].r[0]*=x; sv[i].r[1]*=y; sv[i].r[2]*=z; }
  }

  void takeOver(Mesh *m) { int i,j; QVector<int> *d; clear();
   for (i=0;i<m->v.size();i++) v.append(m->v[i]);
   for (i=0;i<m->e.size();i++) e.append(m->e[i]);
   for (i=0;i<m->f.size();i++) f.append(m->f[i]);
   for (i=0;i<m->w.size();i++) w.append(m->w[i]);
   for (i=0;i<m->sv.size();i++) sv.append(m->sv[i]);
   for (i=0;i<m->sn.size();i++) sn.append(m->sn[i]);
   for (i=0;i<m->n.size();i++) { d=new QVector<int>();
    for (j=0;j<m->n[i]->size();j++) d->append((*(m->n[i]))[j]); n.append(d);
   }
   for (i=0;i<m->sf.size();i++) { d=new QVector<int>();
    for (j=0;j<m->sf[i]->size();j++) d->append((*(m->sf[i]))[j]); sf.append(d);
   }
  }

  void updateFromSimplex() { int i,j; Vec3 center;
   for (i=0;i<sf.size();i++) { center[0]=center[1]=center[2]=0.;
    for (j=0;j<sf[i]->size();j++) {
     center[0]+=sv[(*(sf[i]))[j]].r[0];
     center[1]+=sv[(*(sf[i]))[j]].r[1];
     center[2]+=sv[(*(sf[i]))[j]].r[2];
    } center*=(1./(float)(sf[i]->size())); v[i].r=center;
   }
  }

  // Winged-edge triangle structure
  QVector<Vertex> v;		// Vertices
  QVector<Edge> e;		// Edges
  QVector<Face> f;		// Faces
  QVector<Wing> w;		// Wing Duplets
  QVector<QVector<int>* > n;	// Vertex Neighbors

  // Computed simplex
  QVector<Vertex> sv;		// Simplex vertices
  QVector<SNeigh> sn;		// Simplex vertex neighbours
  QVector<QVector<int>* > sf;	// Simplex faces

 private:
  void setIcosa() { clear();
   Vertex vv; Face ff; Edge ee; Wing ww; QVector<int> *d;
   vv.r=Vec3( 0.850650808352039932, 0., 0.525731112119133606); v.append(vv);
   vv.r=Vec3(-0.850650808352039932, 0.,-0.525731112119133606); v.append(vv);
   vv.r=Vec3( 0.850650808352039932, 0.,-0.525731112119133606); v.append(vv);
   vv.r=Vec3( 0., 0.525731112119133606,-0.850650808352039932); v.append(vv);
   vv.r=Vec3( 0.525731112119133606, 0.850650808352039932, 0.); v.append(vv);
   vv.r=Vec3(-0.525731112119133606, 0.850650808352039932, 0.); v.append(vv);
   vv.r=Vec3( 0., 0.525731112119133606, 0.850650808352039932); v.append(vv);
   vv.r=Vec3(-0.850650808352039932, 0., 0.525731112119133606); v.append(vv);
   vv.r=Vec3( 0.,-0.525731112119133606, 0.850650808352039932); v.append(vv);
   vv.r=Vec3(-0.525731112119133606,-0.850650808352039932, 0.); v.append(vv);
   vv.r=Vec3( 0.525731112119133606,-0.850650808352039932, 0.); v.append(vv);
   vv.r=Vec3( 0.,-0.525731112119133606,-0.850650808352039932); v.append(vv);
   d=new QVector<int>();
   d->append(2); d->append(10); d->append( 8); d->append( 6); d->append( 4);
   n.append(d);
   d=new QVector<int>();
   d->append(3); d->append( 5); d->append( 7); d->append( 9); d->append(11);
   n.append(d);
   d=new QVector<int>();
   d->append(0); d->append( 4); d->append( 3); d->append(11); d->append(10);
   n.append(d);
   d=new QVector<int>();
   d->append(2); d->append( 4); d->append( 5); d->append( 1); d->append(11);
   n.append(d);
   d=new QVector<int>();
   d->append(0); d->append( 6); d->append( 5); d->append( 3); d->append( 2);
   n.append(d);
   d=new QVector<int>();
   d->append(1); d->append( 3); d->append( 4); d->append( 6); d->append( 7);
   n.append(d);
   d=new QVector<int>();
   d->append(0); d->append( 8); d->append( 7); d->append( 5); d->append( 4);
   n.append(d);
   d=new QVector<int>();
   d->append(1); d->append( 5); d->append( 6); d->append( 8); d->append( 9);
   n.append(d);
   d=new QVector<int>();
   d->append(0); d->append(10); d->append( 9); d->append( 7); d->append( 6);
   n.append(d);
   d=new QVector<int>();
   d->append(1); d->append( 7); d->append( 8); d->append(10); d->append(11);
   n.append(d);
   d=new QVector<int>();
   d->append(0); d->append( 2); d->append(11); d->append( 9); d->append( 8);
   n.append(d);
   d=new QVector<int>();
   d->append(1); d->append( 9); d->append(10); d->append( 2); d->append( 3);
   n.append(d);
   ff.v[0]= 2; ff.v[1]= 4; ff.v[2]= 3; f.append(ff); 
   ff.v[0]= 0; ff.v[1]= 4; ff.v[2]= 2; f.append(ff); 
   ff.v[0]= 0; ff.v[1]= 6; ff.v[2]= 4; f.append(ff); 
   ff.v[0]= 0; ff.v[1]= 8; ff.v[2]= 6; f.append(ff); 
   ff.v[0]= 0; ff.v[1]=10; ff.v[2]= 8; f.append(ff); 
   ff.v[0]= 0; ff.v[1]= 2; ff.v[2]=10; f.append(ff); 
   ff.v[0]= 2; ff.v[1]=11; ff.v[2]=10; f.append(ff); 
   ff.v[0]= 9; ff.v[1]=10; ff.v[2]=11; f.append(ff); 
   ff.v[0]= 8; ff.v[1]=10; ff.v[2]= 9; f.append(ff); 
   ff.v[0]= 7; ff.v[1]= 8; ff.v[2]= 9; f.append(ff); 
   ff.v[0]= 6; ff.v[1]= 8; ff.v[2]= 7; f.append(ff); 
   ff.v[0]= 5; ff.v[1]= 6; ff.v[2]= 7; f.append(ff); 
   ff.v[0]= 4; ff.v[1]= 6; ff.v[2]= 5; f.append(ff); 
   ff.v[0]= 3; ff.v[1]= 4; ff.v[2]= 5; f.append(ff); 
   ff.v[0]= 1; ff.v[1]= 3; ff.v[2]= 5; f.append(ff); 
   ff.v[0]= 1; ff.v[1]= 5; ff.v[2]= 7; f.append(ff); 
   ff.v[0]= 1; ff.v[1]= 7; ff.v[2]= 9; f.append(ff); 
   ff.v[0]= 1; ff.v[1]= 9; ff.v[2]=11; f.append(ff); 
   ff.v[0]= 1; ff.v[1]=11; ff.v[2]= 3; f.append(ff); 
   ff.v[0]= 2; ff.v[1]= 3; ff.v[2]=11; f.append(ff); 
   ee.v[0]= 0; ee.v[1]= 2; ww.f[0]= 1; ww.f[1]= 5; e.append(ee); w.append(ww);
   ee.v[0]= 0; ee.v[1]= 4; ww.f[0]= 1; ww.f[1]= 2; e.append(ee); w.append(ww);
   ee.v[0]= 0; ee.v[1]= 6; ww.f[0]= 2; ww.f[1]= 3; e.append(ee); w.append(ww);
   ee.v[0]= 0; ee.v[1]= 8; ww.f[0]= 3; ww.f[1]= 4; e.append(ee); w.append(ww);
   ee.v[0]= 0; ee.v[1]=10; ww.f[0]= 4; ww.f[1]= 5; e.append(ee); w.append(ww);
   ee.v[0]= 2; ee.v[1]= 4; ww.f[0]= 0; ww.f[1]= 1; e.append(ee); w.append(ww);
   ee.v[0]= 4; ee.v[1]= 6; ww.f[0]= 2; ww.f[1]=12; e.append(ee); w.append(ww);
   ee.v[0]= 6; ee.v[1]= 8; ww.f[0]= 3; ww.f[1]=10; e.append(ee); w.append(ww);
   ee.v[0]= 8; ee.v[1]=10; ww.f[0]= 4; ww.f[1]= 8; e.append(ee); w.append(ww);
   ee.v[0]= 2; ee.v[1]=10; ww.f[0]= 5; ww.f[1]= 6; e.append(ee); w.append(ww);
   ee.v[0]= 2; ee.v[1]= 3; ww.f[0]= 0; ww.f[1]=19; e.append(ee); w.append(ww);
   ee.v[0]= 3; ee.v[1]= 4; ww.f[0]= 0; ww.f[1]=13; e.append(ee); w.append(ww);
   ee.v[0]= 4; ee.v[1]= 5; ww.f[0]=12; ww.f[1]=13; e.append(ee); w.append(ww);
   ee.v[0]= 5; ee.v[1]= 6; ww.f[0]=11; ww.f[1]=12; e.append(ee); w.append(ww);
   ee.v[0]= 6; ee.v[1]= 7; ww.f[0]=10; ww.f[1]=11; e.append(ee); w.append(ww);
   ee.v[0]= 7; ee.v[1]= 8; ww.f[0]= 9; ww.f[1]=10; e.append(ee); w.append(ww);
   ee.v[0]= 8; ee.v[1]= 9; ww.f[0]= 8; ww.f[1]= 9; e.append(ee); w.append(ww);
   ee.v[0]= 9; ee.v[1]=10; ww.f[0]= 7; ww.f[1]= 8; e.append(ee); w.append(ww);
   ee.v[0]=10; ee.v[1]=11; ww.f[0]= 6; ww.f[1]= 7; e.append(ee); w.append(ww);
   ee.v[0]= 2; ee.v[1]=11; ww.f[0]= 6; ww.f[1]=19; e.append(ee); w.append(ww);
   ee.v[0]= 3; ee.v[1]= 5; ww.f[0]=13; ww.f[1]=14; e.append(ee); w.append(ww);
   ee.v[0]= 5; ee.v[1]= 7; ww.f[0]=11; ww.f[1]=15; e.append(ee); w.append(ww);
   ee.v[0]= 7; ee.v[1]= 9; ww.f[0]= 9; ww.f[1]=16; e.append(ee); w.append(ww);
   ee.v[0]= 9; ee.v[1]=11; ww.f[0]= 7; ww.f[1]=17; e.append(ee); w.append(ww);
   ee.v[0]= 3; ee.v[1]=11; ww.f[0]=18; ww.f[1]=19; e.append(ee); w.append(ww);
   ee.v[0]= 1; ee.v[1]= 3; ww.f[0]=14; ww.f[1]=18; e.append(ee); w.append(ww);
   ee.v[0]= 1; ee.v[1]= 5; ww.f[0]=14; ww.f[1]=15; e.append(ee); w.append(ww);
   ee.v[0]= 1; ee.v[1]= 7; ww.f[0]=15; ww.f[1]=16; e.append(ee); w.append(ww);
   ee.v[0]= 1; ee.v[1]= 9; ww.f[0]=16; ww.f[1]=17; e.append(ee); w.append(ww);
   ee.v[0]= 1; ee.v[1]=11; ww.f[0]=17; ww.f[1]=18; e.append(ee); w.append(ww);
   computeSimplex();
  }

  void subdivide() { int i,j;
   Vertex dV; Edge dE; Face dF,latF; QVector<int> *dN; // Wing dW;
   Edge e1,e2; Face f1,f2; int a,b,ax=0,bx=0; Wing w1,w2,w3;
   Mesh *mesh=new Mesh(); // Empty destination to fill..

   // COPY ORIGINAL VERTICES..

   // Copy original vertex coordinates to mesh.
   // Newly generated coords with implicit offsets will append upon them..
   mesh->v.resize(v.size()); for (int i=0;i<v.size();i++) mesh->v[i]=v[i];

   // APPEND NEWLY CREATED (MID) VERTICES..

   // Create & append new vertices and then recreate the two connected edges.
   // New vertices are one-to-one associated with the previous edges..
   for (i=0;i<e.size();i++) {
    dV.r[0]=(v[e[i].v[0]].r[0]+v[e[i].v[1]].r[0])/2.0;
    dV.r[1]=(v[e[i].v[0]].r[1]+v[e[i].v[1]].r[1])/2.0;
    dV.r[2]=(v[e[i].v[0]].r[2]+v[e[i].v[1]].r[2])/2.0;
    mesh->v.append(dV); // Midpoint computed and appended..
    // Create the two edges linked to this vertex (must be side to center)..
    dE.v[0]=e[i].v[0]; dE.v[1]=v.size()+i; mesh->e.append(dE);
    dE.v[0]=e[i].v[1]; dE.v[1]=v.size()+i; mesh->e.append(dE);
   }

   // CREATE FACES

   // A completely new list (center tri having the same offset with parent)..
   mesh->f.resize(4*f.size()); // 4 times the number of old triangles..
   for (i=0;i<f.size();i++) { // Center triangles with the same offset
    dF.v[0]=v.size()+findEdge(f[i].v[0],f[i].v[1]); // Newly created vertices
    dF.v[1]=v.size()+findEdge(f[i].v[1],f[i].v[2]); // coming right afterwards..
    dF.v[2]=v.size()+findEdge(f[i].v[2],f[i].v[0]); mesh->f[i]=dF; // CWise!

    // Newly introduced center edges
    dE.v[0]=dF.v[0]; dE.v[1]=dF.v[1]; mesh->e.append(dE);
    dE.v[0]=dF.v[1]; dE.v[1]=dF.v[2]; mesh->e.append(dE);
    dE.v[0]=dF.v[2]; dE.v[1]=dF.v[0]; mesh->e.append(dE);

    // Handle the three new lateral faces
    latF.v[0]=f[i].v[0]; latF.v[1]=dF.v[0]; latF.v[2]=dF.v[2]; // #1
    mesh->f[f.size()+i]=latF;
    latF.v[0]=f[i].v[1]; latF.v[1]=dF.v[1]; latF.v[2]=dF.v[0]; // #2
    mesh->f[2*f.size()+i]=latF;
    latF.v[0]=f[i].v[2]; latF.v[1]=dF.v[2]; latF.v[2]=dF.v[1]; // #3
    mesh->f[3*f.size()+i]=latF;
   }

   // CREATE EDGE WINGS

   // New wings are a completely new list..
   for (i=0;i<e.size();i++) {
    e1=mesh->e[2*i]; e2=mesh->e[2*i+1];	// Newly created child edges..
    a=w[i].f[0]; b=w[i].f[1]; f1=f[a]; f2=f[b]; // Faces of the parent wing..

    // Find the distant vertices of the parent wings' faces..
         if (f1.v[0]!=e1.v[0] && f1.v[0]!=e2.v[0]) ax=0; // Which of the three?
    else if (f1.v[1]!=e1.v[0] && f1.v[1]!=e2.v[0]) ax=1;
    else if (f1.v[2]!=e1.v[0] && f1.v[2]!=e2.v[0]) ax=2;
         if (f2.v[0]!=e1.v[0] && f2.v[0]!=e2.v[0]) bx=0; // Same for the other
    else if (f2.v[1]!=e1.v[0] && f2.v[1]!=e2.v[0]) bx=1; // adjacent face..
    else if (f2.v[2]!=e1.v[0] && f2.v[2]!=e2.v[0]) bx=2;

    // Adjust the probability of being upside-down..
    if (e1.v[0]==mesh->f[(((ax+1)%3)+1)*f.size()+a].v[0]) {
     w1.f[0]=(((bx+2)%3)+1)*f.size()+b; w1.f[1]=(((ax+1)%3)+1)*f.size()+a;
     w2.f[0]=(((ax+2)%3)+1)*f.size()+a; w2.f[1]=(((bx+1)%3)+1)*f.size()+b;
    } else {
     w1.f[0]=(((ax+2)%3)+1)*f.size()+a; w1.f[1]=(((bx+1)%3)+1)*f.size()+b;
     w2.f[0]=(((bx+2)%3)+1)*f.size()+b; w2.f[1]=(((ax+1)%3)+1)*f.size()+a;
    } mesh->w.append(w1); mesh->w.append(w2); // Append the wings..
   }

   // Handle the three old edges to form the three new inner edges..
   for (i=0;i<f.size();i++) {
    w1.f[0]=2*f.size()+i; w1.f[1]=i; // Out-to-center..
    w2.f[0]=3*f.size()+i; w2.f[1]=i;
    w3.f[0]=1*f.size()+i; w3.f[1]=i; // Append them afterwards..
    mesh->w.append(w1); mesh->w.append(w2); mesh->w.append(w3);
   }

   // Create vertex neighbours of the updated context..
   for (i=0;i<mesh->v.size();i++) { dN=new QVector<int>(); mesh->n.append(dN); }

   // Parent vertices
   for (i=0;i<v.size();i++) for (j=0;j<n[i]->size();j++)
    mesh->n[i]->append(v.size()+findEdge(i,(*(n[i]))[j]));

   for (i=0;i<e.size();i++) { // Newly generated (midpoint) vertices
    mesh->n[v.size()+i]->append(mesh->e[2*i].v[0]);
    mesh->n[v.size()+i]->append(mesh->f[mesh->w[2*i].f[0]].v[1]);
    mesh->n[v.size()+i]->append(mesh->f[mesh->w[2*i+1].f[1]].v[2]);
    mesh->n[v.size()+i]->append(mesh->e[2*i+1].v[0]);
    mesh->n[v.size()+i]->append(mesh->f[mesh->w[2*i+1].f[0]].v[1]);
    mesh->n[v.size()+i]->append(mesh->f[mesh->w[2*i].f[1]].v[2]);
   }

   // Wipe old data, get generated data, kill the host, normalize vertices..
   takeOver(mesh); delete mesh; normalizeV();
  }

  void computeSimplex() {
   int v0,v1,v2,n0,n1,n2,i,j,k; Wing *ww0,*ww1,*ww2; SNeigh nn; Vertex dV;
   QVector<int> *ff; sv.resize(0); sn.resize(0);
   for (i=0;i<sf.size();i++) delete sf[i]; sf.resize(0);

   // SIMPLEX VERTICES BEING COMPUTED AS CENTERS OF FACES'

   for (i=0;i<f.size();i++) { v0=f[i].v[0]; v1=f[i].v[1]; v2=f[i].v[2];
    dV.r=(v[v0].r+v[v1].r+v[v2].r)*(1./3.); sv.append(dV);
   }

//   for (i=0;i<v.size();i++) { ff=new QVector<int>(); // Simplex faces
//    for (j=0;j<n[i]->size();j++) {
//     k=findEdge(i,(*(n[i]))[j]); ff->append(w[k].f[0]); // ! changed func.
//    } sf.append(ff);
//   }

//   for (i=v.size();i<v.size();i++) { ff=new QVector<int>();
//    for (j=0;j<n[i]->size();j+=2) { k=findEdge(i,(*(n[i]))[j]); // ch func.
//     if (j==4) { ff->append(w[k].f[0]); ff->append(w[k].f[1]); }
//     else { ff->append(w[k].f[1]); ff->append(w[k].f[0]); }
//    } sf.append(ff);
//   }

   for (i=0;i<v.size();i++) { ff=new QVector<int>();
    for (j=0;j<n[i]->size();j++) {
     k=findFace(i,(*(n[i]))[j],(*(n[i]))[(j+1)%n[i]->size()]); ff->append(k);
    } sf.append(ff);
   }

   for (i=0;i<f.size();i++) { // Simplex neighbours
    v0=f[i].v[0]; v1=f[i].v[1]; v2=f[i].v[2];
    n0=findEdge(v0,v1); n1=findEdge(v1,v2); n2=findEdge(v2,v0);
    ww0=&(w[n0]); ww1=&(w[n1]); ww2=&(w[n2]);
    if (ww0->f[0]==i) nn.v[0]=ww0->f[1]; else nn.v[0]=ww0->f[0];
    if (ww1->f[0]==i) nn.v[1]=ww1->f[1]; else nn.v[1]=ww1->f[0];
    if (ww2->f[0]==i) nn.v[2]=ww2->f[1]; else nn.v[2]=ww2->f[0]; sn.append(nn);
   }

   normalizeSV();
  }

  void clear() { int i;
   for (i=0;i<n.size();i++) delete n[i];
   for (i=0;i<sf.size();i++) delete sf[i];
   n.resize(0); sf.resize(0); sv.resize(0); sn.resize(0);
   v.resize(0); e.resize(0); f.resize(0); w.resize(0);
  }
// 7.1,10.1,7.8
  void normalizeV() { int i; float d; // Normalize Vertices
   for (i=0;i<v.size();i++) { d=v[i].r.norm(); if (d) v[i].r*=(1./d); }
//    v[i].r[0]*=8.8; v[i].r[1]*=11.4; v[i].r[2]*=10.4;
//   }
  }

  void normalizeSV() { int i; float d; // Normalize Simplex Vertices
   for (i=0;i<sv.size();i++) { d=sv[i].r.norm(); if (d) sv[i].r*=(1./d); }
//    sv[i].r[0]*=8.8; sv[i].r[1]*=11.4; sv[i].r[2]*=10.4;
//   }
  }

  int findEdge(int v0,int v1) { int i,count;
   for (count=0,i=0;i<e.size();i++) {
    if ( (e[i].v[0]==v0 && e[i].v[1]==v1) ||
         (e[i].v[0]==v1 && e[i].v[1]==v0) ) break; count++;
   } if (count==e.size()) return -1; else return count;
  }

  int findFace(int v0,int v1,int v2) { int i,offset;
   for (offset=-1,i=0;i<f.size();i++) {
    if ((f[i].v[0]==v0 && f[i].v[1]==v1 && f[i].v[2]==v2) ||
        (f[i].v[0]==v0 && f[i].v[1]==v2 && f[i].v[2]==v1) ||
        (f[i].v[0]==v1 && f[i].v[1]==v0 && f[i].v[2]==v2) ||
        (f[i].v[0]==v1 && f[i].v[1]==v2 && f[i].v[2]==v0) ||
        (f[i].v[0]==v2 && f[i].v[1]==v0 && f[i].v[2]==v1) ||
        (f[i].v[0]==v2 && f[i].v[1]==v1 && f[i].v[2]==v0)) { offset=i; break; }
   } return offset;
  }

};

#endif
