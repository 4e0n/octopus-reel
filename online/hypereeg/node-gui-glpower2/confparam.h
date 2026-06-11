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
#include <QColor>
#include <QWaitCondition>
#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QQueue>
#include <algorithm>
#include "guichninfo.h"
#include "headmodel.h"
#include "../../../common/event.h"
#include "../../../common/coord3d.h"
#include "../../../common/gizmo.h"

const int GLHEAD_REFRESH_MS=500; // Not to be smaller than 500ms

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() { glRefreshMs=GLHEAD_REFRESH_MS; quitPending=false; }

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) { // Upstream command
   if (!socket || socket->state()!=QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  void requestQuit() { 
   {
     QMutexLocker locker(&mutex);
     quitPending=true;
   }
  }

  unsigned int guiMaxAmpPerLine=0;

  QString compPPIpAddr; quint32 compPPCommPort; QTcpSocket *compPPCommSocket;

  unsigned int ampCount,refChnCount,bipChnCount,metaChnCount;
  unsigned int physChnCount,chnCount,totalChnCount,totalCount;

  QVector<GUIChnInfo> refChns,bipChns,metaChns;
  std::atomic<bool> quitPending{false};

  int glRefreshMs;
#ifdef EEGBANDSCOMP
  unsigned int eegBand;
#endif

  int guiX,guiY,guiW,guiH,frameW,frameH,cellSize;
  QVector<QVector<float>> curGLData;

  QMutex mutex;

  quint32 glCommPort=0;
  QTcpServer glCommServer;
  QVector<QTcpSocket*> glClients;

  bool pollingActive=true;

  void loadGizmo(QString fn) {
    QFile file; QTextStream stream; QString line; QStringList lines,validLines,opts,opts2;
    bool gError=false; QVector<unsigned int> t(3); QVector<unsigned int> ll(2);

    if (glGizmoLoaded) glGizmo.resize(0);
    glGizmoLoaded=false;

    file.setFileName(fn);
    file.open(QIODevice::ReadOnly|QIODevice::Text);
    stream.setDevice(&file);
    while (!stream.atEnd()) { line=stream.readLine(250); lines.append(line); }
    file.close();

    for (int i=0;i<lines.size();i++)
      if (!(lines[i].at(0)=='#') && lines[i].contains('|')) validLines.append(lines[i]);

    for (int i=0;i<validLines.size();i++) {
      opts2=validLines[i].split(" ");
      opts=opts2[0].split("|");
      opts2.removeFirst();
      opts2=opts2[0].split(",");
      if (opts.size()==2 && opts2.size()>0) {
        if (opts[0].trimmed()=="GIZMO") {
          if (opts[1].trimmed()=="NAME") {
            for (int g=0;g<opts2.size();g++) { Gizmo dummyGizmo(opts2[g].trimmed()); glGizmo.append(dummyGizmo); }
          }
        } else if (opts[0].trimmed()=="SHAPE") {
          ;
        } else if (opts[1].trimmed()=="SEQ") {
          int k=gizFindIndex(opts[0].trimmed());
          for (int j=0;j<opts2.size();j++) glGizmo[k].seq.append(opts2[j].toInt()-1);
        } else if (opts[1].trimmed()=="TRI") {
          int k=gizFindIndex(opts[0].trimmed());
          if (opts2.size()%3==0) {
            for (int j=0;j<opts2.size()/3;j++) {
              t[0]=opts2[j*3+0].toInt()-1; t[1]=opts2[j*3+1].toInt()-1; t[2]=opts2[j*3+2].toInt()-1;
              glGizmo[k].tri.append(t);
            }
          } else { gError=true; qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: Triangles not multiple of 3 vertices.."; }
        } else if (opts[1].trimmed()=="LIN") {
          int k=gizFindIndex(opts[0].trimmed());
          if (opts2.size()%2==0) {
            for (int j=0;j<opts2.size()/2;j++) {
              ll[0]=opts2[j*2+0].toInt()-1; ll[1]=opts2[j*2+1].toInt()-1;
              glGizmo[k].lin.append(ll);
            }
          } else { gError=true; qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: Lines not multiple of 2 vertices.."; }
        }
      } else { gError=true; qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: while parsing!"; }
    }
    if (!gError) glGizmoLoaded=true;
  }

  int gizFindIndex(QString s) {
    int idx=-1;
    for (int i=0;i<glGizmo.size();i++) if (glGizmo[i].name==s) { idx=i; break; }
    return idx;
  }

  QVector<QColor> rgbPalette;

  int gl3DFrameW=0,gl3DFrameH=0;

  bool event=false;
  QVector<Event> events;
  QString curEventName;
  quint32 curEventType=0;

  QVector<HeadModel> headModel;
  bool glGizmoLoaded=false;
  QVector<Gizmo> glGizmo;
  QVector<bool> glFrameOn,glGridOn,glScalpOn,glSkullOn,glBrainOn,glGizmoOn,glElectrodesOn;

 signals:
  void glUpdate();
  void glUpdateParam(int);

 public slots:

 private slots:

 private:
};

