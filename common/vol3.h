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

#ifndef OCTOPUS_VOL3_H
#define OCTOPUS_VOL3_H

#include <QVector>
#include "mrivolume.h"
#include "mesh.h"

template<class T> class Vol3 : QObject {
 public:
  Vol3() { xSize=ySize=zSize=0; }
  Vol3(int x,int y,int z) { resize(x,y,z); }

  void resize(int x,int y,int z) { int i,j,k; xSize=x; ySize=y; zSize=z;
   data.resize(z); for (k=0;k<zSize;k++) { data[k].resize(y);
                    for (j=0;j<ySize;j++) data[k][j].resize(x); }
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
                                                             data[k][j][i]=T();
  }

  void convolve(Vol3<float> *vol) { int x,y,z,i,j,k,X,Y,Z,n; float sum;
   if (vol->xSize==vol->ySize && vol->ySize==vol->zSize && vol->xSize%2) {
    n=vol->xSize/2;
    for (z=0;z<zSize;z++) {
     for (y=0;y<ySize;y++) for (x=0;x<xSize;x++) {
      for (sum=0.,k=-n;k<=n;k++) for (j=-n;j<=n;j++) for (i=-n;i<=n;i++) {
       Z=z+k; Y=y+j; X=x+i;
       if (X>=0 && Y>=0 && Z>=0 && X<xSize && Y<ySize && Z<zSize) {
        sum += data[Z][Y][X] * vol->data[k+n][j+n][i+n];
       } data[z][y][x]=sum;
      }
     } qDebug("CONVz: %d",z);
    }
   } else qDebug("ERROR: Destination not suitable for convolution!");
  }

  void setGaussian(float s) { int i,j,k,size,xs,ys,zs; float r2,k1,k2,sum;
   size=(int)((2.*3.*s)); if (!(size%2)) size++; resize(size,size,size);
   k1=1./(sqrt(8.*M_PI*M_PI*M_PI*s*s*s)); k2=-1./(s*s); xs=ys=zs=size/2;
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++) {
    r2=(xs-i)*(xs-i)+(ys-j)*(ys-j)+(zs-k)*(zs-k); data[k][j][i]=k1*exp(k2*r2); }

//   for (sum=0.,k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
//    sum+=data[k][j][i];
//   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
//    data[k][j][i]/=sum;

//   for (k=0;k<zSize;k++) { for (j=0;j<ySize;j++) {
//    for (i=0;i<xSize;i++) { printf("%2.4f ",data[k][j][i]);
//     sum+=data[k][j][i]; }
//     printf("\n"); } printf("---------\n"); }
//   printf("================\n");
//   printf("%2.5f\n",sum);
  }

  void normalize() { int i,j,k; float min=1e10,max=-1e10;
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++) {
    if (data[k][j][i]<min) min=data[k][j][i];
    if (data[k][j][i]>max) max=data[k][j][i]; }
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
    data[k][j][i]=(data[k][j][i]-min)/(max-min);
  }

  void initNeumann(Vol3<float> *vol) { int i,j,k;
   // Zero edges
   for (i=0;i<xSize;i++) { data[0][0][i]=data[0][ySize-1][i]=
    data[zSize-1][0][i]=data[zSize-1][ySize-1][i]=Vec3(0.,0.,0.); }
   for (j=0;j<ySize;j++) { data[0][j][0]=data[0][j][xSize-1]=
    data[zSize-1][j][0]=data[zSize-1][j][xSize-1]=Vec3(0.,0.,0.); }
   for (k=0;k<zSize;k++) { data[k][0][0]=data[k][0][xSize-1]=
    data[k][ySize-1][0]=data[k][ySize-1][xSize-1]=Vec3(0.,0.,0.); }

   // Compute initial condition of left and right boundary..
   for (k=1;k<zSize-1;k++) for (j=1;j<ySize-1;j++) {
    data[k][j][0]=Vec3(0.,
             0.5*(vol->data[0][j+1][0]-vol->data[0][j-1][0]),
             0.5*(vol->data[k+1][0][0]-vol->data[k-1][0][0]));
    data[k][j][xSize-1]=Vec3(0.,
             0.5*(vol->data[0][j+1][xSize-1]-vol->data[0][j-1][xSize-1]),
             0.5*(vol->data[k+1][0][xSize-1]-vol->data[k-1][0][xSize-1])); }

   // Compute initial condition of front and rear boundary..
   for (k=1;k<zSize-1;k++) for (i=1;i<xSize-1;i++) {
    data[k][0][i]=Vec3(
             0.5*(vol->data[0][0][i+1]-vol->data[0][0][i-1]),0.,
             0.5*(vol->data[k+1][0][0]-vol->data[k-1][0][0]));
    data[k][ySize-1][i]=Vec3(
             0.5*(vol->data[0][ySize-1][i+1]-vol->data[0][ySize-1][i-1]),0.,
             0.5*(vol->data[k+1][ySize-1][0]-vol->data[k-1][ySize-1][0])); }

   // Compute initial condition of up and down boundary..
   for (j=1;j<ySize-1;j++) for (i=1;i<xSize-1;i++) {
    data[0][j][i]=Vec3(
             0.5*(vol->data[0][0][i+1]-vol->data[0][0][i-1]),
             0.5*(vol->data[0][j+1][0]-vol->data[0][j-1][0]),0.);
    data[zSize-1][j][i]=Vec3(
             0.5*(vol->data[zSize-1][0][i+1]-vol->data[zSize-1][0][i-1]),
             0.5*(vol->data[zSize-1][j+1][0]-vol->data[zSize-1][j-1][0]),0.); }

   // Compute interior derivative..
   for (k=1;k<zSize-1;k++) for (j=1;j<ySize-1;j++) for (i=1;i<xSize-1;i++)
    data[k][j][i]=Vec3(0.5*(vol->data[k][j][i+1]-vol->data[k][j][i-1]),
                       0.5*(vol->data[k][j+1][i]-vol->data[k][j-1][i]),
                       0.5*(vol->data[k+1][j][i]-vol->data[k-1][j][i]));
  }

  void updateNeumann(Vol3<Vec3> *fld) { int i,j,k;
   for (i=0;i<xSize;i++) { // Update edges along x..
    data[0][0][i]=0.5*(fld->data[0][1][i]+fld->data[1][0][i])-
                                                            fld->data[0][0][i];
    data[0][ySize-1][i]=0.5*(fld->data[1][ySize-1][i]+fld->data[0][ySize-2][i])-
                                                      fld->data[0][ySize-1][i];
    data[zSize-1][0][i]=0.5*(fld->data[zSize-2][0][i]+fld->data[zSize-1][1][i])-
                                                      fld->data[zSize-1][0][i];
    data[zSize-1][ySize-1][i]=0.5*(fld->data[zSize-2][ySize-1][i]+
                                   fld->data[zSize-1][ySize-2][i])-
                                              fld->data[zSize-1][ySize-1][i]; }
   for (j=0;j<ySize;j++) { // Update edges along y..
    data[0][j][0]=0.5*(fld->data[0][j][1]+fld->data[1][j][0])-
                                                            fld->data[0][j][0];
    data[0][j][xSize-1]=0.5*(fld->data[1][j][xSize-1]+fld->data[0][j][xSize-2])-
                                                      fld->data[0][j][xSize-1];
    data[zSize-1][j][0]=0.5*(fld->data[zSize-2][j][0]+fld->data[xSize-1][j][1])-
                                                      fld->data[zSize-1][j][0];
    data[zSize-1][j][xSize-1]=0.5*(fld->data[zSize-2][j][xSize-1]+
                                   fld->data[zSize-1][j][xSize-2])-
                                              fld->data[zSize-1][j][xSize-1]; }
   for (k=0;k<zSize;k++) { // Update edges along z..
    data[k][0][0]=0.5*(fld->data[k][0][1]+fld->data[k][1][0])-
                                                            fld->data[k][0][0];
    data[k][0][xSize-1]=0.5*(fld->data[k][1][xSize-1]+fld->data[k][0][xSize-2])-
                                                      fld->data[k][0][xSize-1];
    data[k][ySize-1][0]=0.5*(fld->data[k][ySize-2][0]+fld->data[k][ySize-1][1])-
                                                      fld->data[k][ySize-1][0];
    data[k][ySize-1][xSize-1]=0.5*(fld->data[k][ySize-2][xSize-1]+
                                   fld->data[k][ySize-1][xSize-2])-
                                              fld->data[k][ySize-1][xSize-1]; }

   for (k=1;k<zSize-1;k++) for (j=1;j<ySize-1;j++) { // Left & Right boundaries
    data[k][j][0]=
     (2.*fld->data[k][j][1]+
      fld->data[k][j-1][0]+fld->data[k][j+1][0]+
      fld->data[k-1][j][0]+fld->data[k+1][j][0])*(1./6.)-fld->data[k][j][0];
    data[k][j][xSize-1]=
     (2.*fld->data[k][j][xSize-2]+
      fld->data[k][j-1][xSize-1]+fld->data[k][j+1][xSize-1]+
      fld->data[k-1][j][xSize-1]+fld->data[k+1][j][xSize-1])*(1./6.)-
                                                    fld->data[k][j][xSize-1]; }
   for (k=1;k<zSize-1;k++) for (i=1;i<xSize-1;i++) { // Front & Rear boundaries
    data[k][0][i]=
     (2.*fld->data[k][1][i]+
      fld->data[k][0][i-1]+fld->data[k][0][i+1]+
      fld->data[k-1][0][i]+fld->data[k+1][0][i])*(1./6.)-fld->data[k][0][i];
    data[k][ySize-1][i]=
     (2.*fld->data[k][ySize-2][i]+
      fld->data[k][ySize-1][i-1]+fld->data[k][ySize-1][i+1]+
      fld->data[k-1][ySize-1][i]+fld->data[k+1][ySize-1][i])*(1./6.)-
                                                    fld->data[k][ySize-1][i]; }
   for (j=1;j<ySize-1;j++) for (i=1;i<xSize-1;i++) { // Up & Down boundaries
    data[0][j][i]=
     (2.*fld->data[1][j][i]+
      fld->data[0][j][i-1]+fld->data[0][j][i+1]+
      fld->data[0][j-1][i]+fld->data[0][j+1][i])*(1./6.)-fld->data[0][j][i];
    data[zSize-1][j][i]=
     (2.*fld->data[zSize-2][j][i]+
      fld->data[zSize-1][j][i-1]+fld->data[zSize-1][j][i+1]+
      fld->data[zSize-1][j-1][i]+fld->data[zSize-1][j+1][i])*(1./6.)-
                                                    fld->data[zSize-1][j][i]; }
   // Update interior volume..
   for (k=1;k<zSize-1;k++) for (j=1;j<ySize-1;j++) for (i=1;i<xSize-1;i++)
    data[k][j][i]=(fld->data[k][j][i-1]+fld->data[k][j][i+1]+
                   fld->data[k][j-1][i]+fld->data[k][j+1][i]+
                   fld->data[k-1][j][i]+fld->data[k+1][j][i])*(1./6.)-
                                                            fld->data[k][j][i];
  }

  void updateGVF(Vol3<float> *b,Vol3<float> *g,Vol3<Vec3> *c,
                 Vol3<Vec3> *u,Vol3<Vec3> *lu) { int i,j,k;
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
    u->data[k][j][i] = (1.-b->data[k][j][i])*u->data[k][j][i]+
                        4.*g->data[k][j][i]*lu->data[k][j][i]+c->data[k][j][i];
  }

  void getX(Vol3<Vec3> *fld) { int i,j,k;
   resize(fld->xSize,fld->ySize,fld->zSize);
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
    data[k][j][i]=fld->data[k][j][i][2];
  }

  void computeGVF(Vol3<float> *vol,float mu,int iter) {
   int i,j,k,count; float dX,dY,dZ;
   resize(vol->xSize-1,vol->ySize-1,vol->zSize-1);

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
    lu.updateNeumann(&u);
    u.updateGVF(&b,&g,&c,&u,&lu); qDebug("GVF Iteration #%d",count);
   }

   // Copy the result over our own..
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++)
    data[k][j][i]=u.data[k][j][i];
  }

  void setFromMRI(MRIVolume *vol) { int i,j,k; float vox;
   resize(vol->xSize,vol->ySize,vol->zSize);
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++) {
    vox=(float)(qGray(vol->slice[k]->data.pixel(i,j)))/255.; data[k][j][i]=vox;}
  }

  void setToMRI(MRIVolume *vol) { int i,j,k,vox;
   for (k=0;k<zSize;k++) for (j=0;j<ySize;j++) for (i=0;i<xSize;i++) {
    vox=(quint8)(data[k][j][i]*255.); vol->slice[k]->data.setPixel(i,j,vox); }
  }

  void deform(Mesh *src,Mesh *dst,
              float alpha,float beta,float tau,int iter) {
   int count,i; Vec3 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,del,del2; SNeigh *ss;

   for (count=0;count<iter;count++) {
    for (i=0;i<src->sv.count();i++) { ss=&(src->sn[i]); v0=src->sv[i].r;
//     qDebug("%d %d %d",ss->v[0],ss->v[1],ss->v[2]);
     v1=src->sv[ss->v[0]].r;
     v2=src->sv[ss->v[1]].r;
     v3=src->sv[ss->v[2]].r;
     v4=src->sv[src->sn[ss->v[0]].v[0]].r;
     v5=src->sv[src->sn[ss->v[0]].v[1]].r;
     v6=src->sv[src->sn[ss->v[1]].v[0]].r;
     v7=src->sv[src->sn[ss->v[1]].v[1]].r;
     v8=src->sv[src->sn[ss->v[2]].v[0]].r;
     v9=src->sv[src->sn[ss->v[2]].v[1]].r;
     del=v0.del(v1,v2,v3); del2=v0.del2(v1,v2,v3,v4,v5,v6,v7,v8,v9);
     dst->sv[i].r=v0+tau*(alpha*del-beta*del2+data[v0[2]][v0[1]][v0[0]]);
    }
    for (int i=0;i<src->sv.size();i++) src->sv[i]=dst->sv[i];
    qDebug("Deformable Model Iteration #%d",count);
   }
  }

  int xSize,ySize,zSize; QVector<QVector<QVector< T > > > data;
};

#endif
