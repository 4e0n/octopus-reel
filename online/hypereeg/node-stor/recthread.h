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

#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QDateTime>
#include <cmath>
#include <atomic>
#include <deque>
#include "confparam.h"
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "bvwav.h"

//#pragma pack(push,1)
//struct ProofRecChunk {
// quint64 unixMs;
// qint64 off0;
// qint64 offN;
// quint32 n;
// quint32 magic; // 'PRF1'
// quint32 crc;
//};
//#pragma pack(pop)
//static_assert(sizeof(ProofRecChunk)==36,"ProofRecChunk size unexpected");

class RecThread : public QThread {
  Q_OBJECT
 public:
//  explicit RecThread(ConfParam* c,QObject* parent=nullptr) : QThread(parent),conf(c) {
  RecThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c;
   tcpEEG=TcpSample(conf->ampCount,conf->chnCount);
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&conf->tcpBuffer;
   eegChunk.resize(conf->eegSamplesInTick);
   conf->tcpBufTail=0;
  }

 public slots:
  void requestStartRecording(QString basePathNoExt) { // e.g. "/opt/octopus/pool/20260111-183012-123"
   qDebug() << "Recording START requested.";
   QMutexLocker lk(&cmdMutex);
   pendingBase=basePathNoExt;
   startReq.store(true,std::memory_order_release);
   // also wake consumer so it processes quickly
   conf->dataReady.wakeAll();
  }

  void requestStopRecording() {
   qDebug() << "Recording STOP requested.";
   stopReq.store(true,std::memory_order_release);
   conf->dataReady.wakeAll();
  }

  void requestMarker(QString type,QString desc) { // optional for later
   {
    QMutexLocker lk(&markerMutex);
    markerQueue.push_back({std::move(type),std::move(desc)});
   }
   markerReq.store(true,std::memory_order_release);
   // Wake the thread so the marker is flushed quickly.
   conf->dataReady.wakeAll();
  }

 protected:
  void run() override {
   N=conf->eegSamplesInTick; eegChunk.resize(N);

   while (!isInterruptionRequested()) {

    // Phase 0 (STOP)
    if (stopReq.exchange(false, std::memory_order_acq_rel)) {
     if (recOn) {
      QString err;
      if (!recorder.stop(&err))
       qCritical() << "STOR[REC] stop() failed:" << err;
      recOn=false;
      qInfo()<<"STOR[REC] stopped.";
     }
     lastOffset=-1;
    }

    // ---- Phase 0: START ----
    if (startReq.exchange(false, std::memory_order_acq_rel)) {
     QString base;
     { QMutexLocker lk(&cmdMutex); base=pendingBase; }

     // stop any previous run
     if (recOn) {
      QString err;
      if (!recorder.stop(&err))
       qCritical() << "STOR[REC] stop before restart failed:" << err;
       recOn = false;
     }

     BrainVisionMeta bv;
     bv.sampleRateHz=conf->eegRate; bv.unit="uV"; bv.channelResolution=1.0;

     bv.channelNames.clear();

//     bv.channelNames.reserve(int(conf->ampCount*conf->chnCount));
//     for (uint32_t a=0;a<conf->ampCount;++a) for (uint32_t c=0;c<conf->chnCount;++c) {
//      //bv.channelNames << QString("A%1_Ch%2").arg(a+1).arg(c+1);
//      // EEG
//      bv.channelNames << "A"+QString::number(a+1)+"_"+conf->chns[c].chnName;
//      qDebug() << "A"+QString::number(a+1)+"_"+conf->chns[c].chnName;
//     }

     bv.channelNames.reserve(int(conf->ampCount*conf->chnCount+48));
     // EEG: amp-major, channel-minor (as you already do)
     for (uint32_t a=0;a<conf->ampCount;++a) for (uint32_t c=0;c<conf->chnCount;++c) {
      bv.channelNames << ("A"+QString::number(a+1)+"_"+conf->chns[c].chnName);
     }
     // AUD: appended after all EEG channels
     for (int i=0;i<48;++i) {
      bv.channelNames << QString("AUD%1").arg(i);
     }

     WavMeta wav; wav.sampleRate=48000; wav.numChannels=1;

     QString err;
     if (!recorder.setup(base,bv,wav,&err)) {
      qCritical() << "STOR[REC] setup failed:" << err;
      recOn=false;
     } else {
      recOn=true; recBase=base; bvSampleIndex=1; lastOffset=-1;
      qInfo() << "STOR[REC] started base=" << base;
     }
    }

    // Phase 0b (markers)
    if (markerReq.exchange(false,std::memory_order_acq_rel)) {
     std::deque<PendingMarker> local;
     { QMutexLocker lk(&markerMutex); local.swap(markerQueue); }

     if (recOn) {
      QString err;
      for (const auto& m:local) {
       if (!recorder.addMarker(m.type,m.desc,bvSampleIndex,&err)) qCritical() << "STOR[REC] addMarker failed:" << err;
      }
     }
    }

    // Phase A (wait for N samples, copy-out under mutex)
    quint64 tail0=0;
    {
     QMutexLocker locker(&conf->mutex);
     while (!isInterruptionRequested()) {
      const quint64 avail=conf->tcpBufHead-conf->tcpBufTail;
      if (avail>=(quint64)N) break;
      conf->dataReady.wait(&conf->mutex,50);
     }
     if (isInterruptionRequested()) break;

     tail0=conf->tcpBufTail;
     for (int i=0; i<N;++i)
      eegChunk[i]=conf->tcpBuffer[(tail0+i)%conf->tcpBufSize];
    }

    // ***ALARM*** (sequence integrity check)
    bool okSeq=true;
    for (int i=1;i<N;++i) {
     if (eegChunk[i].offset!=eegChunk[i-1].offset+1) { okSeq=false; break; }
    }
    if (lastOffset>=0 && eegChunk[0].offset!=lastOffset+1) okSeq=false;

    if (!okSeq) {
     const qint64 expected=lastOffset+1;
     const qint64 got=eegChunk[0].offset;
     qCritical() << "STOR[ALARM] DATA LOSS DETECTED!"
                 << "expectedOff=" << expected
                 << "gotOff=" << got
                 << "gap=" << (got-expected);
     // optional: auto-stop
     // if (recOn) { QString e; recorder.stop(&e); recOn=false; }
    }
    lastOffset=eegChunk[N-1].offset;

    // Phase B (write outside mutex)
    if (recOn) {
     QString err;
     if (!recorder.writeChunk_FromTcpSamples(eegChunk,conf->ampCount,conf->chnCount,&err)) {
      qCritical() << "STOR[REC] writeChunk failed:" << err;

      QString err2;
      if (!recorder.stop(&err2))
       qCritical() << "STOR[REC] stop after write failure failed:" << err2;

      recOn=false;
     } else {
      bvSampleIndex+=uint64_t(N);
     }
    }

    // Phase C (commit tail under mutex, wake producer)
    bool freed=false;
    {
     QMutexLocker locker(&conf->mutex);
     const quint64 oldTail = conf->tcpBufTail;
     const quint64 wantTail = tail0 + (quint64)N;
     if (conf->tcpBufTail < wantTail) conf->tcpBufTail = wantTail;
     freed = (conf->tcpBufTail != oldTail);
    }
    if (freed) conf->spaceReady.wakeOne();
   }

   // ---- Shutdown cleanup ----
   if (recOn) {
    QString err;
    if (!recorder.stop(&err))
     qCritical() << "STOR[REC] stop (shutdown) failed:" << err;
    recOn=false;
   }
  }

 private:
  ConfParam* conf=nullptr;
  std::atomic<bool> startReq{false};
  std::atomic<bool> stopReq{false};
  QString pendingBase; // guarded by cmdMutex
  QMutex cmdMutex;

  // Recording state owned by this thread
  bool recOn=false;
  QString recBase;
  uint64_t bvSampleIndex=1; // 1-based for vmrk
  BVWav recorder;

  // chunk buffers
  TcpSample tcpEEG;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize;
  int N=0;
  std::vector<TcpSample> eegChunk;

  // for alarms
  qint64 lastOffset=-1;

  struct PendingMarker {
   QString type;
   QString desc;
  };

  QMutex markerMutex;
  std::deque<PendingMarker> markerQueue;
  std::atomic<bool> markerReq{false};

// QFile proofFile;
// bool proofOn=false;

  static quint32 cheapCrc(qint64 off0,qint64 offN,quint32 n) {
   quint64 x=(quint64)off0^((quint64)offN<<1)^((quint64)n<<32)^0x9e3779b97f4a7c15ULL;
   x^=(x>>33); x*=0xff51afd7ed558ccdULL;
   x^=(x>>33); x*=0xc4ceb9fe1a85ec53ULL;
   x^=(x>>33);
   return (quint32)(x & 0xffffffffu);
  }
};
