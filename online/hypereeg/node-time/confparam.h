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
#include <QElapsedTimer>
#include <QTimer>
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
   ctrlRecordingActive=false; tcpBufHead=tcpBufTail=0; audWaveH=100;
   quitPending=false;
  }

  void initMultiAmp(int ampC=0) {
   ampCount=ampC; eegAmpX.resize(ampCount); erpAmpX.resize(ampCount); threads.resize(ampCount);
   eegSweepPending.resize(ampCount); for (auto &v:eegSweepPending) v=false;
   // tick window (keep your existing logic/values)
   scrUpdateSamples=qMax(1u,eegRate/eegSweepRefreshRate);
   scrAvailableSamples=scrUpdateSamples;
   // PLL init (no gating applied yet; Step 2 will use it)
   pllT.start();
   pllTargetPeriodMs=int(1000/qMax(1u,eegSweepRefreshRate));
   pllNextMs=pllT.elapsed()+qint64(1000/qMax(1u,eegSweepRefreshRate)); // ~20ms
//   pllNextMs=pllT.elapsed()+pllTargetPeriodMs;
   pllErrI=0.0;
   pllInit=true;

   if (!pllTimer) {
    pllTimer=new QTimer(this);
    pllTimer->setTimerType(Qt::PreciseTimer);
    connect(pllTimer,&QTimer::timeout,this,&ConfParam::onPllTick);
   }
   // 1ms polling is fine; PLL gate will only allow ~20ms ticks anyway
   pllTimer->start(1);
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
  
  void requestQuit() { 
   {
     QMutexLocker locker(&mutex);
     quitPending=true;
     // unblock all consumers even if no pending tick
     for (auto &v:eegSweepPending) v=true;
     eegSweepUpdating=ampCount; // doesn't matter much now; just prevents weird last==0 paths
   }
   for (auto *t:threads) if (t) t->requestInterruption();
   if (pllTimer) pllTimer->stop();
   if (acqStrmSocket) {
    acqStrmSocket->blockSignals(true);
    disconnect(acqStrmSocket,nullptr,this,nullptr);
    acqStrmSocket->close();
   }
   eegSweepWait.wakeAll();
  }

  QString acqIpAddr; quint32 acqCommPort,acqStrmPort; QTcpSocket *acqCommSocket,*acqStrmSocket;
  QString storIpAddr; quint32 storCommPort; QTcpSocket *storCommSocket;

  QVector<TcpSamplePP> tcpBuffer; quint32 tcpBufSize; quint64 tcpBufHead,tcpBufTail; int frameBytes;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick;
  unsigned int physChnCount,totalChnCount,totalCount; float refGain,bipGain;

  QVector<QThread*> threads; QMutex mutex; QVector<GUIChnInfo> chnInfo;
  QVector<bool> eegSweepPending; unsigned int eegSweepUpdating; bool quitPending;

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

   // --- PLL/barrier timing (producer-side gate) ---
  QElapsedTimer pllT;
  bool   pllInit=false;
  qint64 pllNextMs=0;          // next allowed wake time in pllT.elapsed() ms domain
  int    pllTargetPeriodMs=20; // derived from eegSweepRefreshRate (e.g. 20ms @ 50Hz)
  double pllErrI=0.0;

  // --- perf / logging (producer + consumer) ---
  std::atomic<quint64> time_okFrames{0};     // successful deserializations
  std::atomic<quint64> time_badFrames{0};    // deserialize fail OR payLen%frameBytes!=0
  std::atomic<quint64> wakeIssued{0};        // producer scheduled a tick
  std::atomic<quint64> wakeGateSkip{0};      // producer wanted to wake but PLL gate blocked
  std::atomic<quint64> ringOverruns{0};      // if you later add "drop" policy; kept for completeness

  // consumer-side counters (all amps combined)
  std::atomic<quint64> consTicks{0};         // how many tick renders happened (all amps)
  std::atomic<quint64> consSamples{0};       // how many samples consumers processed (all amps)
  std::atomic<quint64> consDrawMsAcc{0};     // accumulated draw ms (all amps)

  QTimer *pllTimer=nullptr;

  // --------------------------------------------------------------------------------------

 signals:
  void glUpdate();
  void glUpdateParam(int);

 public slots:
  void onStrmDataReady() {
   if (quitPending) return;
   static quint64 outerOk=0;
   static qint64 timeLastMsRx=0;
   static qint64 perfLastMs=0;
   static quint64 perf_last_ok=0,perf_last_bad=0,perf_last_wake=0,perf_last_skip=0;

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
     time_badFrames.fetch_add(1,std::memory_order_relaxed);
     inbuf.remove(0,4+payLen);
     continue;
    }
    const int nFrames=payLen/frameBytes;

    for (int i=0;i<nFrames;++i) {
     const char *framePtr=pay+i*frameBytes;
     // Wrap without copying
     const QByteArray one=QByteArray::fromRawData(framePtr,frameBytes);
     TcpSamplePP s;
     if (!s.deserialize(one,chnCount)) {
      time_badFrames.fetch_add(1,std::memory_order_relaxed);
      continue;
     }
     // producer counters
     time_okFrames.fetch_add(1,std::memory_order_relaxed);

     {
       QMutexLocker locker(&mutex);
       tcpBuffer[tcpBufHead%tcpBufSize]=s; tcpBufHead++;
     } 

/*
     bool doWake=false; bool gateSkip=false;
     const qint64 nowPll = pllT.elapsed(); // IMPORTANT: use same time domain as pllNextMs
     {
       QMutexLocker locker(&mutex);
       tcpBuffer[tcpBufHead%tcpBufSize]=s; tcpBufHead++;
       const quint64 avail=tcpBufHead-tcpBufTail;
       const bool wantWake=(avail>=scrAvailableSamples && eegSweepUpdating==0);
       if (wantWake) {
        if (!pllInit) {
         pllT.start(); pllNextMs=pllT.elapsed()+pllTargetPeriodMs;
         pllInit=true; gateSkip=true;
        } else if (nowPll<pllNextMs) {
         gateSkip=true;
        } else {
         eegSweepUpdating=ampCount;
         for (auto &v:eegSweepPending) v=true;
         doWake=true;
         wakeIssued.fetch_add(1,std::memory_order_relaxed);
         // advance gate (no catch-up storm)
         pllNextMs=nowPll+pllTargetPeriodMs;
        }
       }
     }
     if (gateSkip) wakeGateSkip.fetch_add(1,std::memory_order_relaxed);
     if (doWake) eegSweepWait.wakeAll();
*/
    }
    // discard the outer packet
    inbuf.remove(0,4+payLen);
    outerOk++;
   }

   // PERF log 1 Hz (producer-side + ring fill)
   if (perfLastMs==0) perfLastMs=now;
   if (now-perfLastMs>=1000) {
    perfLastMs=now;
    const quint64 ok=time_okFrames.load(std::memory_order_relaxed);
    const quint64 bad=time_badFrames.load(std::memory_order_relaxed);
    const quint64 wk=wakeIssued.load(std::memory_order_relaxed);
    const quint64 sk=wakeGateSkip.load(std::memory_order_relaxed);
    quint64 hSnap=0,tSnap=0;
    {
      QMutexLocker locker(&mutex);
      hSnap=tcpBufHead; tSnap=tcpBufTail;
    }
    const quint64 used=hSnap-tSnap;
    const double fillPct=tcpBufSize ? (100.0*double(used)/double(tcpBufSize)):0.0;

    const quint64 d_ok  =ok -perf_last_ok;   perf_last_ok=ok;
    const quint64 d_bad =bad-perf_last_bad;  perf_last_bad=bad;
    const quint64 d_wk  =wk -perf_last_wake; perf_last_wake=wk;
    const quint64 d_sk  =sk -perf_last_skip; perf_last_skip=sk;

    qInfo().noquote()
     << QString("[TIME:PERF] ok=%1/s bad=%2/s wake=%3/s gateSkip=%4/s ring=%5/%6 (%7%)")
         .arg((qulonglong)d_ok)
         .arg((qulonglong)d_bad)
         .arg((qulonglong)d_wk)
         .arg((qulonglong)d_sk)
         .arg((qulonglong)used)
         .arg((qulonglong)tcpBufSize)
         .arg(fillPct, 0, 'f', 1);
   }
  }

 private slots:
  void onPllTick() {
   if (!pllInit) return;
   if (quitPending) return;
   bool doWake=false; bool gateSkip=false;
   const qint64 nowPll=pllT.elapsed();
   {
     QMutexLocker locker(&mutex);
     const quint64 avail=tcpBufHead-tcpBufTail;
     const bool wantWake=(avail>=scrAvailableSamples) && (eegSweepUpdating==0);
     if (!wantWake) return;
     if (nowPll<pllNextMs) {
      gateSkip=true;
     } else {
      eegSweepUpdating=ampCount;
      for (auto &v:eegSweepPending) v=true;
      doWake=true;
      wakeIssued.fetch_add(1,std::memory_order_relaxed);
      // no catch-up storm
      pllNextMs=nowPll+pllTargetPeriodMs;
     }
   }
   if (gateSkip) wakeGateSkip.fetch_add(1,std::memory_order_relaxed);
   if (doWake) eegSweepWait.wakeAll();
  }

 private:
};
