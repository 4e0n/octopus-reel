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

#include "../../../common/event.h"
#include "../../../common/coord3d.h"
#include "../../../common/gizmo.h"

const int EEG_SCROLL_REFRESH_RATE=50; // 100Hz max.


class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {
   eegSweepRefreshRate=EEG_SCROLL_REFRESH_RATE; eegSweepUpdating=0;
   eegSweepMode=quitPending=ctrlRecordingActive=false; ctrlNotchActive=false;
   tcpBufHead=tcpBufTail=0; recCounter=0; audWaveH=100; //cumIdx=0;
  };

  void initMultiAmp(int ampC=0) {
   ampCount=ampC; eegAmpX.resize(ampCount); erpAmpX.resize(ampCount); threads.resize(ampCount);
  };

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) { // Upstream command
   if (!socket || socket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
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
    if (!(lines[i].at(0)=='#') && lines[i].contains('|')) validLines.append(lines[i]);

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

  QVector<QColor> rgbPalette;

  QString acqIpAddr; quint32 acqCommPort,acqStrmPort; QTcpSocket *acqCommSocket,*acqStrmSocket;
  QString storIpAddr; quint32 storCommPort; QTcpSocket *storCommSocket;

  //QString timeIpAddr; quint32 timeCommPort,timeStrmPort; // We're server

  QMutex mutex; QVector<GUIChnInfo> chnInfo; QVector<QThread*> threads;

  bool eegSweepMode; unsigned int eegSweepRefreshRate,eegSweepFrameTimeMs,eegSweepDivider,eegBand; QWaitCondition eegSweepWait;
  //std::atomic<unsigned int> eegSweepUpdating;
  unsigned int eegSweepUpdating; QVector<bool> eegSweepPending;

  std::vector<std::vector<float>> s0,s0s,s1,s1s; // Sweep vals

  const int eegSweepCoeff[5]={10,5,4,2,1};

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

  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  quint64 tcpBufHead,tcpBufTail; QVector<TcpSamplePP> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;
  int cntPastSize,cntPastIndex;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick; float refGain,bipGain;

  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH, guiAmpX,guiAmpY,guiAmpW,guiAmpH;
  int sweepFrameW,sweepFrameH, audWaveH, gl3DFrameW,gl3DFrameH;

  bool ctrlRecordingActive,ctrlNotchActive, tick,event,quitPending;

  QVector<Event> events; QString curEventName; quint32 curEventType;
  float erpRejBwd,erpAvgBwd,erpAvgFwd,erpRejFwd; // In terms of milliseconds

  unsigned int scrAvailableSamples,scrUpdateSamples;

  quint64 recCounter; QString rHour,rMin,rSec; QLabel *timeLabel;

  QVector<HeadModel> headModel; bool glGizmoLoaded; QVector<Gizmo> glGizmo;
  QVector<bool> glFrameOn,glGridOn,glScalpOn,glSkullOn,glBrainOn,glGizmoOn,glElectrodesOn;

  bool usePixmap=false;

  int serSize;

 signals:
  void glUpdate();
  void glUpdateParam(int);

 public slots:

void onStrmDataReady() {

static quint64 g_outer_ok = 0;
static quint64 g_time_ok = 0;
static quint64 g_time_bad = 0;
//static qint64  g_time_last_ms = 0;
static qint64  g_time_last_ms_rx = 0;
  static QByteArray buffer;
  buffer.append(acqStrmSocket->readAll());

  const qint64 now = QDateTime::currentMSecsSinceEpoch();

  // RX backlog log 1 Hz
  if (g_time_last_ms_rx==0) g_time_last_ms_rx=now;
  if (now - g_time_last_ms_rx >= 1000) {
    g_time_last_ms_rx = now;
    qInfo().noquote()
      << QString("[TIME:RX] buffer=%1 bytes")
           .arg(buffer.size());
  }

  while (buffer.size() >= 4) {
   const uchar *p0 = reinterpret_cast<const uchar*>(buffer.constData());
   const quint32 Lo = (quint32)p0[0]
                   | ((quint32)p0[1] << 8)
                   | ((quint32)p0[2] << 16)
                   | ((quint32)p0[3] << 24);

   if (buffer.size() < 4 + (int)Lo) break;

   // payload pointer (NO COPY)
   const char *pay = buffer.constData() + 4;
   const int payLen = (int)Lo;

   // fixed-size frames: payLen must be multiple of serSize
   if (payLen % serSize != 0) {
    g_time_bad++;
    buffer.remove(0, 4 + payLen);
    continue;
   }
   const int nFrames = payLen / serSize;

   for (int i = 0; i < nFrames; ++i) {
    const char *framePtr = pay + i*serSize;

    // Wrap without copying
    const QByteArray one = QByteArray::fromRawData(framePtr, serSize);

    TcpSamplePP s;
    if (!s.deserialize(one, chnCount)) { g_time_bad++; continue; }
    g_time_ok++;

    {
      QMutexLocker locker(&mutex);
      tcpBuffer[tcpBufHead % tcpBufSize] = s;
      tcpBufHead++;

      const quint64 availableSamples = tcpBufHead - tcpBufTail;
      if (availableSamples > scrAvailableSamples && eegSweepUpdating==0) {
        scrUpdateSamples = scrAvailableSamples / (eegSweepFrameTimeMs / eegSweepDivider);
        if (quitPending) {
          if (acqStrmSocket && acqStrmSocket->isOpen())
            acqStrmSocket->disconnectFromHost();
        }
        eegSweepUpdating = ampCount;
        for (auto &v : eegSweepPending) v = true;
        eegSweepWait.wakeAll();
      }
     }
    }

    // now discard the outer packet
    buffer.remove(0, 4 + payLen);
    g_outer_ok++;
   }


/*

  while (buffer.size() >= 4) {
    const uchar *p = reinterpret_cast<const uchar*>(buffer.constData());
    const quint32 L = (quint32)p[0]
                    | ((quint32)p[1] << 8)
                    | ((quint32)p[2] << 16)
                    | ((quint32)p[3] << 24);

    if (buffer.size() < 4 + (int)L) break;

    const QByteArray payload = buffer.mid(4, L);
    buffer.remove(0, 4 + (int)L);
    g_outer_ok++;

    int pos = 0;
    while (pos + 4 <= payload.size()) {
     const uchar *p = reinterpret_cast<const uchar*>(payload.constData() + pos);
     const quint32 L = (quint32)p[0]
                       | ((quint32)p[1] << 8)
                       | ((quint32)p[2] << 16)
                       | ((quint32)p[3] << 24);
     pos += 4;
     if (pos + (int)L > payload.size()) break; // corrupted / partial (shouldn't happen)

     const QByteArray one = payload.mid(pos, L);
     pos += (int)L;

     TcpSamplePP s;
     if (!s.deserialize(one,chnCount)) { g_time_bad++; continue; }
     g_time_ok++;

     // push into your ring under mutex (as you already do)
     {
       QMutexLocker locker(&mutex);
       tcpBuffer[tcpBufHead % tcpBufSize] = s; // or std::move(s)
       tcpBufHead++;
       // your wake logic...
       const quint64 availableSamples = tcpBufHead - tcpBufTail;

       if (availableSamples > scrAvailableSamples && eegSweepUpdating==0) {
        if (availableSamples >= (quint64)(tcpBufSize * 3 / 4))
          qWarning() << "[TIME] Buffer high-water:" << availableSamples;

        scrUpdateSamples = scrAvailableSamples / (eegSweepFrameTimeMs / eegSweepDivider);

        if (quitPending) {
          if (acqStrmSocket && acqStrmSocket->isOpen())
            acqStrmSocket->disconnectFromHost();
        }

        eegSweepUpdating = ampCount;
        for (auto &v : eegSweepPending) v = true;
        eegSweepWait.wakeAll();
       }
     }
    }
   }

   // packets/sec log 1 Hz
   if (g_time_last_ms==0) g_time_last_ms=now;
   if (now - g_time_last_ms >= 1000) {
    qInfo().noquote()
      << QString("[TIME:PKT] outer=%1/s samples=%2/s bad=%3/s")
          .arg((qulonglong)g_outer_ok)
          .arg((qulonglong)g_time_ok)
          .arg((qulonglong)g_time_bad);
    g_outer_ok = 0;
    g_time_ok = g_time_bad = 0;
    g_time_last_ms = now;
   }
*/
  }

 private:
  void updateRecTime() { int s,m,h; QString dummyString; s=recCounter/eegRate; //m=s/60; h=m/60;
   h=s/3600; s%=3600; m=s/60; s%=60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }
};
