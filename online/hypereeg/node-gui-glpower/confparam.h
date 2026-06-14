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

#include <QTcpSocket>
#include <QTcpServer>
#include <QMutex>
#include <atomic>
#include <QVector>
#include <QLabel>
#include <QVector3D>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include "powchninfo.h"

const int GLPOWER_REFRESH_MS=500; // Not to be smaller than 500ms

struct PowObj3D {
 QVector<QVector3D> v;
 QVector<QVector<unsigned int>> f;
 QVector<QVector3D> n;   // per-vertex normals
 bool loaded=false;
};

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {
   powRefreshMs=GLPOWER_REFRESH_MS; quitPending=false;
   glXRot=0; glYRot=0; glZRot=0; // adjust if frontal is 90/-90 in your coordinate convention
   glCameraDistance=60.0f; glCameraFov=35.0f; 

   curSpatialCorr.resize(ConfParam::POWER_BAND_COUNT);
   spatialCorrInit.resize(ConfParam::POWER_BAND_COUNT);
   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
    curSpatialCorr[b]=0.0f;
    spatialCorrInit[b]=false;
   }
  }

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) { // Upstream command
   if (!socket || socket->state()!=QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  void computeNormals(PowObj3D &obj) {
   obj.n.clear(); obj.n.resize(obj.v.size());
   for (int i=0;i<obj.n.size();++i) obj.n[i]=QVector3D(0,0,0);
   for (const auto &tri:obj.f) {
    if (tri.size()<3) continue;
    if (tri[0]>=unsigned(obj.v.size())) continue;
    if (tri[1]>=unsigned(obj.v.size())) continue;
    if (tri[2]>=unsigned(obj.v.size())) continue;
    const QVector3D &a=obj.v[int(tri[0])];
    const QVector3D &b=obj.v[int(tri[1])];
    const QVector3D &c=obj.v[int(tri[2])];
    QVector3D nn=QVector3D::normal(a,b,c);
    obj.n[int(tri[0])]+=nn; obj.n[int(tri[1])]+=nn; obj.n[int(tri[2])]+=nn;
   }
   for (int i=0;i<obj.n.size();++i) {
    if (obj.n[i].lengthSquared()>1e-12f)
     obj.n[i].normalize();
    else
     obj.n[i]=QVector3D(0,0,1);
   }
  }

  bool loadObj(const QString &fn,PowObj3D &obj) {
   QFile file(fn);
   if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
    qWarning() << "node-gui-glpower: cannot open OBJ" << fn;
    return false;
   }
   obj.v.clear(); obj.f.clear(); obj.loaded=false;

   QTextStream st(&file);
   while (!st.atEnd()) {
    QString line=st.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#")) continue;

    QStringList s=line.split(QRegExp("\\s+"),Qt::SkipEmptyParts);
    if (s.size()<1) continue;

    if (s[0]=="v" && s.size()>=4) {
     const float x=s[1].toFloat(); const float y=s[2].toFloat(); const float z=s[3].toFloat();
     // skip extreme invalid vertices
     if (std::fabs(x)>1e6f || std::fabs(y)>1e6f || std::fabs(z)>1e6f) obj.v.append(QVector3D(0,0,0));
     else obj.v.append(QVector3D(x,y,z));
    } else if (s[0]=="f" && s.size()>=4) {
     QVector<unsigned int> tri; tri.resize(3);
     for (int i=0;i<3;++i) {
      QString t=s[i+1].split("/")[0]; tri[i]=t.toUInt();   // zero-indexed in your format
     }
     obj.f.append(tri);
    }
   }
   file.close();
   obj.loaded=(!obj.v.isEmpty() && !obj.f.isEmpty());

   qInfo() << "node-gui-glpower: loaded OBJ" << fn << "verts=" << obj.v.size() << "faces=" << obj.f.size() << "loaded=" << obj.loaded;

   computeNormals(obj); obj.loaded=(!obj.v.isEmpty() && !obj.f.isEmpty());

   return obj.loaded;
  }

  void requestQuit() { 
   {
     QMutexLocker locker(&mutex);
     quitPending=true;
   }
  }

  unsigned int guiMaxAmpPerLine=0;
  QString compPPIpAddr; quint32 compPPCommPort; QTcpSocket *compPPCommSocket;
  unsigned int ampCount,refChnCount,bipChnCount,metaChnCount,physChnCount,totalChnCount,powerChnCount=0;
  unsigned int gfpAllIdx=0,gfpLeftIdx=0,gfpRightIdx=0;
  QVector<PowChnInfo> refChns,bipChns,metaChns; int powRefreshMs;
  std::atomic<bool> quitPending{false};

#ifdef EEGBANDSCOMP
  unsigned int eegBand;
#endif

  int guiX,guiY,guiW,guiH,frameW,frameH,cellSize;

  QVector<QVector<QVector<float>>> curPowData; // [amp][powerChn][band]

  QMutex mutex; bool pollingActive=true;

  quint32 powCommPort=0; QTcpServer powCommServer; QVector<QTcpSocket*> powClients;

  PowObj3D scalpObj,skullObj,brainObj;
  QString scalpObjPath="/opt/octopus/data/headmodel/obj/scalp.obj";
  QString skullObjPath ="/opt/octopus/data/headmodel/obj/skull.obj";
  QString brainObjPath ="/opt/octopus/data/headmodel/obj/brain.obj";

  bool showScalp=true,showSkull=false,showBrain=true,showElectrodes=true;

  // Shared GL camera/view state
  QMutex glViewMutex;

  float glCameraDistance=60.0f,glCameraFov=35.0f;

  int glXRot=0,glYRot=0,glZRot=-90*16;

  bool glAutoRotate=true;
  int glRotateStep=8,glRotateMs=50; // angle units are /16 deg, i.e. 8=0.5 deg/tick -- 20 fps

  static constexpr int POWER_BAND_COUNT=6;
  static constexpr int POWER_OVERALL=0;
  static constexpr int POWER_DELTA=1;
  static constexpr int POWER_THETA=2;
  static constexpr int POWER_ALPHA=3;
  static constexpr int POWER_BETA=4;
  static constexpr int POWER_GAMMA=5;

  int glSelectedBand=POWER_OVERALL;

  // GL electrode placement adjustment
  float glElecRadiusOffset=0.0f;   // radial offset from scalp sphere
  float glElecSagittalRotDeg=0.0f; // rotation in YZ plane around X axis
  float glElecAzimuthRotDeg=0.0f;  // optional left/right correction around Z

  float glElecXOffset=0.0f,glElecYOffset=0.0f,glElecZOffset=0.0f; // Y: anterior direction (along -nose)

  float glMapGain=0.85f;    // lower-> milder contrast
  float glMapMid=0.5f;      // fixed midpoint
  float glMapAlpha=255.0f;

  float glSharedMapMin=0.0f,glSharedMapMax=1.0f; bool glSharedMapValid=false;

  QVector<QVector<float>> corrHistA,corrHistB; // [band][history]
  QVector<float> curCorr; // [band]
			  //
  QVector<QVector<QVector<float>>> corrChHistA,corrChHistB; // [chn][band][history]
  QVector<QVector<float>> curChCorr; // [chn][band]

  int corrWindowUpdates=120; // 60 s if powRefreshMs=500 ms
  bool corrValuesValid=false;

  float corrDeadZone=0.10f;

  int glInterpNeighbors=16;
  float glInterpPower=1.50f;

  QVector<float> curSpatialCorr; // [band]
  bool spatialCorrValid=false;

  int spatialWindowUpdates=12;   // 6 sec at 500 ms
  float spatialCorrSmooth=0.10f;
  QVector<bool> spatialCorrInit; // [band]

 public slots:
 private slots:
 private:
};
