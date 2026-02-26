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
#include <QString>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <QDateTime>

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#else
#include "eesynth.h"
#endif

#include <iostream>
#include <vector>
#include <algorithm> // nth_element
#include <cstdint>
#include <cmath>
#include <climits>
#include <atomic>

#include "confparam.h"
#include "../common/globals.h"
#include "../common/sample.h"
#include "../common/tcpsample.h"
#include "serialdevice.h"
#include "eeamp.h"
#include "audioamp.h"
#include "../common/logring.h"
#ifdef EEMAGINE
#include "../common/rt_bootstrap.h"
#endif
#include "unitrig.h"

const int CBUF_SIZE_IN_SECS=10;

class AcqThread:public QThread {
 Q_OBJECT
 public:
  AcqThread(ConfParam *c,QObject *parent=nullptr,
            std::vector<amplifier*> *ee=nullptr,AudioAmp *aud=nullptr,SerialDevice *ser=nullptr) : QThread(parent) {
   conf=c; eeAmpsOrig=ee; audioAmp=aud; serDev=ser;

   uniTrig.init(conf->ampCount);

   smp=Sample(conf->physChnCount); tcpEEG=TcpSample(conf->ampCount,conf->physChnCount);
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&(conf->tcpBuffer);

   cBufPivotPrev=cBufPivot=counter0=counter1=0;
   conf->tcpBufHead=0;

   audioBlock.reserve(size_t(conf->eegRate)*AUDIO_N); // 1sec is worst case
   //outBlock.reserve(size_t(conf->eegRate));
   for (unsigned i=0;i<tcpBufSize;++i) (*tcpBuffer)[i].initSizeOnly(conf->ampCount,conf->physChnCount);
  }

  // --------

  void fetchEegData() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif

   std::size_t ampIdx=0;
   for (auto& ee:eeAmps) {
    try {
     ee.buf=ee.str->getData();
     chnCount=ee.buf.getChannelCount();
     ee.smpCount=ee.buf.getSampleCount();
     if (chnCount!=conf->totalChnCount)
      qWarning() << "<fetchEegData> Warning!!! Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    // -----
    const int eegCh=int(chnCount-2); const unsigned baseIdx=ee.cBufIdx;

    // Get Raw EEG Samples
//    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      dst.data[chnIdx]=ee.buf.getSample(chnIdx,smpIdx); // Raw data
     }
    }

#ifdef EEMAGINE
    const bool syncOn=uniTrig.syncOngoing.load(std::memory_order_acquire);
#endif
    for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) { // set trigger/offset serially
     auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
     dst.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     dst.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx;
#ifdef EEMAGINE
     if (syncOn && dst.trigger==(unsigned)TRIG_AMPSYNC) {
      if (uniTrig.noteSyncSeen(ampIdx, uint64_t(baseIdx)+smpIdx)) {
       qInfo("<AmpSync> SYNC received by AMP#%u @bufidx=%llu",unsigned(ampIdx+1),(unsigned long long)uniTrig.syncSeenAtAmpLocalIdx[ampIdx]);
      }
     }
#endif
    }
    // -----
    ee.cBufIdx+=ee.smpCount; // advance head once per block
    ampIdx++;
   }
   cBufPivotPrev=cBufPivot;
   cBufPivot=*std::min_element(cBufPivotList.begin(),cBufPivotList.end());
  }

  void run() override {
   lastSyncSendMs=0;

#ifdef __linux__
#ifdef EEMAGINE
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),1); // Pin acquisition to core 1
   set_thread_rt(pthread_self(),SCHED_FIFO,80); // Give acquisition RT priority
#endif
#endif

#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif

   // --- Initial setup ---

   audioAmp->start();

   // Construct main EE structure
   quint64 rMask,bMask; // Create channel selection masks -- subsequent channels assumed..
   rMask=0; for (unsigned int i=0;i<conf->refChnCount;i++) { rMask<<=1; rMask|=1; }
   bMask=0; for (unsigned int i=0;i<conf->bipChnCount;i++) { bMask<<=1; bMask|=1; }
   //qInfo("<AcqThread_AmpSetup> RBMask: %llx %llx",rMask,bMask);
   std::size_t ampIdx=0;
   for (auto& ee:*eeAmpsOrig) {
    cBufSz=conf->eegRate*CBUF_SIZE_IN_SECS;
    EEAmp e(ampIdx,ee,rMask,bMask,cBufSz,cBufSz/2,conf->physChnCount);
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    eeAmps.push_back(e);
    ampIdx++;
   }

   cBufPivotList.resize(conf->ampCount);

   // Initialize EEG streams of amps.
#ifndef EEMAGINE
   eesynth::set_block_msec(unsigned(conf->eegProbeMsecs));
#endif
   for (auto& e:eeAmps) e.str=e.OpenEegStream(conf->eegRate,conf->refGain,conf->bipGain,e.chnList);
   qInfo() << "<AcqThread_Switch2EEG> EEG upstream started..";

   // EEG stream
   // -----------------------------------------------------------------------------------------------------------------
   fetchEegData(); // The first round of acquisition - to preadjust certain things

   // Cheap sanity check
   const quint64 tcpDataSize=cBufPivot-cBufPivotPrev;
   if (tcpDataSize>quint64(conf->eegRate)*2) {
    qWarning() << "[ACQ] suspicious tcpDataSize=" << tcpDataSize
               << " pivPrev=" << cBufPivotPrev << " piv=" << cBufPivot;
   }

   // Send SYNC
   const uint64_t syncTimeoutSamples=uint64_t(conf->eegRate)/2; // Timeout=0.5s at 1000 sps
   // localIdx "now" must be in same timebase as noteSyncSeen(baseIdx+smpIdx).
   // cBufPivot is min head among amps and is in that baseIdx space.
   uniTrig.beginSync(uint64_t(cBufPivot),syncTimeoutSamples);
   sendAmpSyncByte();
   qInfo("<AmpSync> SYNC sent.. timeout=%llusamp", (unsigned long long)syncTimeoutSamples);

   while (!isInterruptionRequested()) {
    fetchEegData();

    // 1) If all seen -> finalize
    if (uniTrig.syncOngoing.load(std::memory_order_acquire) && uniTrig.syncSeenAtAmpCount==conf->ampCount) {
     if (uniTrig.tryFinalizeIfReady()) {
      qInfo("<AmpSync> aligment offsets (samples):");
      for (size_t a=0;a<uniTrig.ampAlignOffset.size();++a) {
       qInfo(" AMP#%u -> +%u",unsigned(a+1),unsigned(uniTrig.ampAlignOffset[a]));
      }
      qInfo("<AmpSync> SUCCESS in %llusamp",(unsigned long long)(uint64_t(cBufPivot)-uniTrig.syncBeginLocalIdx));
      conf->triggerPending=false;
     }
    }
    // 2) Else if deadline passed -> timeout
    else if (uniTrig.syncOngoing.load(std::memory_order_acquire)) {
     // we can use current pivot as "now"
     const uint64_t nowLocal=uint64_t(cBufPivot);
     if (nowLocal>=uniTrig.syncDeadlineLocalIdx) {
      // timeout: report which amps didn't see it
      qWarning("<AmpSync> TIMEOUT after %llusamp. Seen=%u/%u",
               (unsigned long long)(nowLocal-uniTrig.syncBeginLocalIdx),
               unsigned(uniTrig.syncSeenAtAmpCount),unsigned(uniTrig.syncSeenAtAmp.size()));
      for (size_t a=0;a<uniTrig.syncSeenAtAmp.size();++a) {
       if (!uniTrig.syncSeenAtAmp[a]) qWarning("<AmpSync> missing AMP#%u",unsigned(a+1));
       else qInfo("<AmpSync> seen AMP#%u @%llu",unsigned(a+1),(unsigned long long)uniTrig.syncSeenAtAmpLocalIdx[a]);
      }
      // abort the window (so re-requests can happen)
      uniTrig.syncOngoing.store(false,std::memory_order_release);
      uniTrig.syncSeenAtAmpCount=0;
      conf->triggerPending=false;
     }
    }

    const quint64 tcpDataSize=cBufPivot-cBufPivotPrev;
    if (tcpDataSize==0) { cBufPivotPrev=cBufPivot; continue; }

    // ensure capacity once; resize can still happen if tcpDataSize grows,
    // but it wont do nested heap churn per sample.
    audioBlock.resize(size_t(tcpDataSize)*AUDIO_N);
    std::vector<unsigned> trigOffs; // local, per block
    trigOffs.resize(size_t(tcpDataSize),UINT_MAX);
    for (quint64 i=0;i<tcpDataSize;++i) {
     float* dst=audioBlock.data()+size_t(i)*AUDIO_N;
     // audioBlock holds AUDIO_N samples per EEG sample (aligned by acquisition cadence)
     // fills 48 floats, or zeros on miss + capture trigger
     trigOffs[size_t(i)]=audioAmp->fetchN(dst,2); // Should not be more than 1-2 ms
     if (conf->triggerPending) {
      if (trigOffs[i]!=UINT_MAX) {
       qInfo() << "<AcqThread>[AUDIO:TRIG] Audio trigger ->" << conf->trigVal_r << "/" << conf->audTrigThr;
      } else {
       qInfo() << "<AcqThread>[AUDIO:TRIG] (NO) Audio trigger ->" << conf->trigVal_r << "/" << conf->audTrigThr;
      }
     }
    }

    quint64 start=0,newHead=0;
    {
      QMutexLocker locker(&conf->mutex);
      const quint64 head=conf->tcpBufHead; const quint64 tail=conf->tcpBufTail;
      const quint64 used=head-tail; const quint64 free=quint64(tcpBufSize)-used;
      if (tcpDataSize>free) {
       // POLICY CHOICE:
       // A) block until free (true no-overwrite, but risks upstream loss if hardware buffers overflow)
       // B) -> drop oldest (keeps newest real-time, but causes losing historical samples)
       // C) drop newest (keeps continuity, but causes losing current samples)
       const quint64 need=tcpDataSize-free;
       conf->tcpBufTail+=need;
       droppedSamples+=need; // add this counter in ConfParam
       qWarning() << "[RING] overflow: dropped oldest" << need
                  << " used=" << used << " size=" << tcpBufSize;
      }
      start=head; newHead=head+tcpDataSize;
    }

    // no mutex: fill slots
#ifdef EEMAGINE
    const bool syncOn=uniTrig.syncOngoing.load(std::memory_order_acquire);
#endif
    for (quint64 i=0;i<tcpDataSize;++i) {
     TcpSample &s=(*tcpBuffer)[(start+i)%tcpBufSize];

     const unsigned trigOff=trigOffs[size_t(i)];
     s.audioTrigEvent=(trigOff==UINT_MAX) ? 0u:(uint32_t((trigOff&0xFFu)+1u));

     unsigned tbuf[EE_MAX_AMPCOUNT]; // stack

     for (unsigned ampIdx=0;ampIdx<eeAmps.size();++ampIdx) {
      const Sample &src=eeAmps[ampIdx].cBuf[(cBufPivotPrev+i+quint64(uniTrig.ampAlignOffset[ampIdx]))%cBufSz];
      s.amp[ampIdx].copyFrom(src);

      unsigned t=src.trigger;
#ifdef EEMAGINE
      if (syncOn && t==0) {
       const quint64 rawIdx=(cBufPivotPrev+i+uniTrig.ampAlignOffset[int(ampIdx)]);
       if (pickTrigWithLookaroundExpected(eeAmps[ampIdx].cBuf.data(),rawIdx,cBufSz,unsigned(TRIG_AMPSYNC))) t=unsigned(TRIG_AMPSYNC);
      }
#endif
      tbuf[ampIdx]=t;
     }
     const unsigned hwTrig=uniTrig.mergeTriggers(tbuf,unsigned(eeAmps.size()));
     s.trigger=hwTrig;
     s.userEvent=0;

#ifdef AUDIO_VERBOSE
     static std::vector<int> deltas; static int lastEegTrig=INT_MIN; static int lastAudTrig=INT_MIN;
     const int eegIndex=int(counter0+i); // or (start+i), but be consistent
     if (s.trigger==TRIG_AMPSYNC) lastEegTrig=eegIndex;
     if (s.audioTrigEvent!=0u) lastAudTrig=eegIndex;
     if (lastEegTrig!=INT_MIN && lastAudTrig!=INT_MIN) {
      const int d=lastAudTrig-lastEegTrig;
      deltas.push_back(d);
      if (deltas.size()>31) deltas.erase(deltas.begin());
      auto tmp=deltas; std::nth_element(tmp.begin(),tmp.begin()+tmp.size()/2,tmp.end());
      const int med=tmp[tmp.size()/2];
      const unsigned trigOff=unsigned(s.audioTrigEvent-1u); // because we stored +1
      qInfo("<AcqThread>[AUDALIGN] dSamples=%d median=%d trigOff=%u",d,med,trigOff);
      lastEegTrig=INT_MIN; lastAudTrig=INT_MIN;
     }
#endif

     // Audio copy
     const float* aud=audioBlock.data()+size_t(i)*AUDIO_N;
     for (int k=0;k<AUDIO_N;++k) s.audioN[k]=aud[k];
     s.offset=counter0++; s.timestampMs=0;
    }

    {
      QMutexLocker locker(&conf->mutex);
      conf->tcpBufHead=newHead;
#if defined(EEG_VERBOSE) || defined(HSYNC_VERBOSE)
      static qint64 lastMsRing=0; const qint64 nowMs=QDateTime::currentMSecsSinceEpoch();
      if (nowMs-lastMsRing >= 1000) {
       //bool didLog=false;
#ifdef EEG_VERBOSE
       static quint64 lastH=0,lastT=0;
       const quint64 H=conf->tcpBufHead; const quint64 T=conf->tcpBufTail;
       const quint64 used=H-T;
       const quint64 free=quint64(tcpBufSize)-used;
       const quint64 dH=H-lastH; const quint64 dT=T-lastT;
       qInfo("<AcqThread>[RING] head=%llu tail=%llu used=%llu free=%llu dH/s=%llu dT/s=%llu drop=%llu",
        (unsigned long long)H,(unsigned long long)T,(unsigned long long)used,(unsigned long long)free,
        (unsigned long long)dH,(unsigned long long)dT,(unsigned long long)droppedSamples
       );
       lastH=H; lastT=T;
       //didLog=true;
#endif

#ifdef HSYNC_VERBOSE2
       static uint64_t lastMis=0,lastMiss=0,lastProd=0,lastNZ=0;
       const uint64_t dMis =uniTrig.trigMismatchCounter-lastMis;
       const uint64_t dMiss=uniTrig.trigMissingCounter -lastMiss;
       const uint64_t dProd=uniTrig.trigProduced       -lastProd;
       const uint64_t dNZ  =uniTrig.trigNonZeroProduced-lastNZ;
       qInfo("<AmpSync>[TRIG] prod/s=%llu nz/s=%llu mismatch/s=%llu missing/s=%llu",
        (unsigned long long)dProd,(unsigned long long)dNZ,(unsigned long long)dMis,(unsigned long long)dMiss
       );
       lastMis=uniTrig.trigMismatchCounter; lastMiss=uniTrig.trigMissingCounter;
       lastProd=uniTrig.trigProduced; lastNZ=uniTrig.trigNonZeroProduced;
       if (dMis||dMiss) {
        qWarning("<AmpSync>[TRIG] problems: mismatch/s=%llu missing/s=%llu",(unsigned long long)dMis,(unsigned long long)dMiss);
       }
       //didLog=true;
#endif
       //if (didLog) lastMsRing=nowMs;
       lastMsRing=nowMs;
      }
#endif
    }

    cBufPivotPrev=cBufPivot;

#ifndef EEMAGINE
    msleep(conf->eegProbeMsecs);
#endif

    counter1++;
   } // EEG stream

   audioAmp->stop();
   for (auto& e:eeAmps) { delete e.str; delete e.amp; } // EEG stream stops..
   qInfo("<AcqThread> Exiting thread..");
  }

 public slots:
  void requestAmpSync() {
   // If already in a sync window, dont allow re-triggering.
   if (uniTrig.syncOngoing.load(std::memory_order_acquire)) {
    qInfo() << "<AmpSync> request ignored: sync already ongoing";
    return;
   }
   const qint64 now=QDateTime::currentMSecsSinceEpoch();
   if (lastSyncSendMs!=0 && (now-lastSyncSendMs)<syncCooldownMs) {
    qInfo() << "<AmpSync> request ignored: cooldown"
            << (syncCooldownMs-(now-lastSyncSendMs)) << "ms remaining";
    return;
   }
   lastSyncSendMs=now;

   // Send SYNC
   const uint64_t syncTimeoutSamples=uint64_t(conf->eegRate)/2; // Timeout=0.5s at 1000 sps
   // localIdx "now" must be in same timebase as noteSyncSeen(baseIdx+smpIdx).
   // cBufPivot is min head among amps and is in that baseIdx space.
   //uint64_t nowLocal=uint64_t(cBufPivot); // default
   //if (!cBufPivotList.empty()) { nowLocal=*std::min_element(cBufPivotList.begin(),cBufPivotList.end()); }
   uniTrig.beginSync(uint64_t(cBufPivot),syncTimeoutSamples); // sets syncOngoing=true, resets arrays
   sendAmpSyncByte(); // actually write to SparkFun/amps
   conf->triggerPending=true;
   qInfo("<AmpSync> SYNC sent.. timeout=%llusamp", (unsigned long long)syncTimeoutSamples);
  }

  void requestSynthTrig(uint32_t t) {
   Q_UNUSED(t);
#ifndef OCTO_DIAG_TRIG
   qWarning() << "[TRIG] synthetic trigger disabled (compile without OCTO_DIAG_TRIG)";
   return;
#else
   if (t>=256) {
    qWarning() << "[TRIG] invalid synthetic trigger (must be <256):" << t;
    return;
   }
   if (t==TRIG_AMPSYNC) {
    qWarning() << "[TRIG] use requestAmpSync() for AMPSYNC";
    return;
   }
   sendTriggerByte(uint8_t(t));
   qInfo() << "<AmpSync> Synthetic trigger sent:" << t;
#endif
  }

 private:
  static inline unsigned pickTrigWithLookaroundExpected(const Sample* buf,quint64 idx,unsigned cBufSz,unsigned expected) {
   const unsigned t0=buf[idx%cBufSz].trigger; if (t0==expected) return expected;
   // ±1 sample grace (cheap and deterministic)
   const unsigned tPrev=buf[(idx+cBufSz-1)%cBufSz].trigger; if (tPrev==expected) return expected;
   const unsigned tNext=buf[(idx+1)%cBufSz].trigger; if (tNext==expected) return expected;
   return 0;
  }

  void sendAmpSyncByte() {
#ifdef EEMAGINE
   const unsigned char trig=(unsigned char)TRIG_AMPSYNC;
   serDev->write(trig);
#else
   for (EEAmp& e:eeAmps) (e.amp)->setTrigger(TRIG_AMPSYNC);
#endif
  }

  void sendTriggerByte(uint8_t t) {
#ifdef EEMAGINE
   serDev->write(t);
#else
   for (EEAmp& e: eeAmps) (e.amp)->setTrigger(t);
#endif
  }

  ConfParam *conf;

  TcpSample tcpEEG; Sample smp;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize;

  std::vector<EEAmp> eeAmps; unsigned int cBufSz,smpCount,chnCount;
  quint64 cBufPivotPrev,cBufPivot; std::vector<unsigned int> cBufPivotList;

  quint64 counter0,counter1;

  // --- Hardware pointers -- coming from the sky
  std::vector<amplifier*> *eeAmpsOrig;
  AudioAmp *audioAmp; std::vector<float> audioBlock;

  SerialDevice *serDev;

  // Inter-Amps&Audio Synchronizations and Triggering
  UniTrig uniTrig;

  qint64 lastSyncSendMs=0;

  static constexpr int syncCooldownMs=1000; // 1s of SYNC debouncing/cooling
  unsigned int droppedSamples=0;
};
