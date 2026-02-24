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

#include <QColor>
#include <QTcpSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <atomic>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QLabel>
#include <QDateTime>

#include "guichninfo.h"
#include "headmodel.h"
#include "../common/tcpsample_pp.h"
#include "../common/framedstreamreader.h"

#include "../../../common/event.h"
#include "../../../common/coord3d.h"
#include "../../../common/gizmo.h"

const int EEG_SCROLL_REFRESH_RATE=50; // 100Hz max.

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {
   eegSweepRefreshRate=EEG_SCROLL_REFRESH_RATE; eegSweepUpdating=0;
   quitPending=ctrlRecordingActive=false; tcpBufHead=tcpBufTail=0; audWaveH=100;
  }

  void initMultiAmp(int ampC=0) {
   ampCount=ampC; eegAmpX.resize(ampCount); erpAmpX.resize(ampCount); threads.resize(ampCount);
  }

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) { // Upstream command
   if (!socket || socket->state()!=QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  void loadGizmo(QString fn) {
   QFile file; QTextStream stream; QString line; QStringList lines,validLines,opts,opts2;
   bool gError=false; QVector<unsigned int> t(3); QVector<unsigned int> ll(2);

   // TODO: here a dry-run would fit.
 
   // Delete previous if any
   if (glGizmoLoaded) glGizmo.resize(0);
   glGizmoLoaded=false;

   file.setFileName(fn); file.open(QIODevice::ReadOnly|QIODevice::Text); stream.setDevice(&file);
   while (!stream.atEnd()) { line=stream.readLine(250); lines.append(line); }
   file.close();

   // Get valid lines
   for (int i=0;i<lines.size();i++)
    if (!(lines[i].at(0)=='#')&&lines[i].contains('|')) validLines.append(lines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<validLines.size();i++) {
    opts2=validLines[i].split(" "); opts=opts2[0].split("|"); opts2.removeFirst(); opts2=opts2[0].split(",");
    if (opts.size()==2 && opts2.size()>0) {
     // GIZMO|LIST must be prior to others or segfault will occur..
     if (opts[0].trimmed()=="GIZMO") {
      if (opts[1].trimmed()=="NAME") {
       for (int g=0;g<opts2.size();g++) { Gizmo dummyGizmo(opts2[g].trimmed()); glGizmo.append(dummyGizmo); }
      }
     } else if (opts[0].trimmed()=="SHAPE") {
      ;
     } else if (opts[1].trimmed()=="SEQ") {
      int k=gizFindIndex(opts[0].trimmed()); for (int j=0;j<opts2.size();j++) glGizmo[k].seq.append(opts2[j].toInt()-1);
     } else if (opts[1].trimmed()=="TRI") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%3==0) {
       for (int j=0;j<opts2.size()/3;j++) {
        t[0]=opts2[j*3+0].toInt()-1; t[1]=opts2[j*3+1].toInt()-1; t[2]=opts2[j*3+2].toInt()-1; glGizmo[k].tri.append(t);
       }
      } else { gError=true;
       qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: Triangles not multiple of 3 vertices..";
      }
     } else if (opts[1].trimmed()=="LIN") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%2==0) {
       for (int j=0;j<opts2.size()/2;j++) {
        ll[0]=opts2[j*2+0].toInt()-1; ll[1]=opts2[j*2+1].toInt()-1;
	glGizmo[k].lin.append(ll);
       }
      } else { gError=true;
       qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: Lines not multiple of 2 vertices..";
      }
     }
    } else { gError=true; qDebug() << "node-time: <ConfParam> <LoadGizmo> <OGZ> ERROR: while parsing!"; }
   } if (!gError) glGizmoLoaded=true;
  }

  int gizFindIndex(QString s) { int idx=-1;
   for (int i=0;i<glGizmo.size();i++) if (glGizmo[i].name==s) { idx=i; break; }
   return idx;
  }

  QString acqIpAddr; quint32 acqCommPort,acqStrmPort; QTcpSocket *acqCommSocket,*acqStrmSocket;
  QString storIpAddr; quint32 storCommPort; QTcpSocket *storCommSocket;

  QVector<TcpSamplePP> tcpBuffer; quint32 tcpBufSize; quint64 tcpBufHead,tcpBufTail; int frameBytes;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick;
  unsigned int physChnCount,totalChnCount,totalCount; float refGain,bipGain;

  QVector<QThread*> threads; QMutex mutex; QVector<GUIChnInfo> chnInfo;
  QVector<bool> eegSweepPending; bool quitPending; unsigned int eegSweepUpdating;

  unsigned int scrAvailableSamples,scrUpdateSamples; int eegSweepSpeedIdx=0;
  unsigned int eegSweepRefreshRate,eegSweepFrameTimeMs,eegBand; QWaitCondition eegSweepWait;

  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH, guiAmpX,guiAmpY,guiAmpW,guiAmpH;
  int sweepFrameW,sweepFrameH, audWaveH, gl3DFrameW,gl3DFrameH;

  bool ctrlRecordingActive;

  QVector<float> eegAmpX,erpAmpX;
  const float eegAmpRange[6]={(1e6/1000.0),
                              (1e6/ 500.0),
                              (1e6/ 200.0),
                              (1e6/ 100.0),
                              (1e6/  50.0),
                              (1e6/  20.0)};
  const float erpAmpRange[6]={(1e6/1000.0), // 2,5,10,20,50,100
                              (1e6/ 500.0),
                              (1e6/ 200.0),
                              (1e6/ 100.0),
                              (1e6/  50.0),
                              (1e6/  20.0)};

  QVector<HeadModel> headModel; bool glGizmoLoaded; QVector<Gizmo> glGizmo;
  QVector<bool> glFrameOn,glGridOn,glScalpOn,glSkullOn,glBrainOn,glGizmoOn,glElectrodesOn;

  QVector<Event> events; bool event; QString curEventName; quint32 curEventType;
  float erpRejBwd,erpAvgBwd,erpAvgFwd,erpRejFwd; // In terms of milliseconds
  QVector<QColor> rgbPalette;

  std::atomic<quint64> rxFrames{0};
  std::atomic<quint64> rxBytes{0};
  std::atomic<quint64> rbDrops{0};   // if you drop-oldest or overwrite
  std::atomic<quint64> drawFrames{0}; // how many GUI sweeps happened
  std::atomic<quint64> consFrames{0}; // how many samples consumers pulled

  // --------------------------------------------------------------------------------------

 signals:
  void glUpdate();
  void glUpdateParam(int);

 public slots:
  void onStrmDataReady() {
   static quint64 outerOk=0;
   static quint64 timeOk=0;
   static quint64 timeBad=0;
   static qint64 timeLastMsRx=0;
   static QByteArray inbuf;
   inbuf.append(acqStrmSocket->readAll());
   const qint64 now=QDateTime::currentMSecsSinceEpoch();

   // RX backlog log 1 Hz
   if (timeLastMsRx==0) timeLastMsRx=now;
   if (now-timeLastMsRx>=1000) {
    timeLastMsRx=now;
    qInfo().noquote() << QString("[TIME:RX] inbuf=%1 bytes").arg(inbuf.size());
   }

   while (inbuf.size()>=4) {
    const uchar *p0=reinterpret_cast<const uchar*>(inbuf.constData());
    const quint32 Lo=(quint32)p0[0]
                     |((quint32)p0[1]<<8)
                     |((quint32)p0[2]<<16)
                     |((quint32)p0[3]<<24);
    if (inbuf.size() < 4+(int)Lo) break;

    // payload pointer (NO COPY)
    const char *pay=inbuf.constData()+4;
    const int payLen=(int)Lo;

    // fixed-size frames: payLen must be multiple of frameBytes
    if (payLen%frameBytes!=0) {
     timeBad++;
     inbuf.remove(0,4+payLen);
     continue;
    }
    const int nFrames=payLen/frameBytes;

    for (int i=0;i<nFrames;++i) {
     const char *framePtr=pay+i*frameBytes;
     // Wrap without copying
     const QByteArray one=QByteArray::fromRawData(framePtr,frameBytes);

     TcpSamplePP s;
     if (!s.deserialize(one,chnCount)) { timeBad++; continue; }
     //if (!s.deserializeRaw(framePtr,frameBytes)) { timeBad++; continue; }
     timeOk++;

     bool doWake=false;
     {
       QMutexLocker locker(&mutex);
       tcpBuffer[tcpBufHead%tcpBufSize]=s;
       tcpBufHead++;

       const quint64 avail=tcpBufHead-tcpBufTail;
       doWake=(avail>=scrAvailableSamples && eegSweepUpdating==0);
       if (doWake) {
	//scrUpdateSamples=qMax(1u,eegRate/eegSweepRefreshRate); // 20 at 1kHz/50Hz
        //eegSweepUpdating=ampCount;
        for (auto &v:eegSweepPending) v=true;
       }
     }
     if (doWake) eegSweepWait.wakeAll();
    }

    // now discard the outer packet
    inbuf.remove(0,4+payLen);
    outerOk++;
   }
  }

 private:
};
