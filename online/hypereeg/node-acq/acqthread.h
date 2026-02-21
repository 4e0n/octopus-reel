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

const int CBUF_SIZE_IN_SECS=10;

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(ConfParam *c,QObject *parent=nullptr,
            std::vector<amplifier*> *ee=nullptr,AudioAmp *aud=nullptr,SerialDevice *ser=nullptr) : QThread(parent) {
   conf=c; eeAmpsOrig=ee; audioAmp=aud; serDev=ser;

   conf->syncOngoing.store(false,std::memory_order_release);
   conf->syncPerformed.store(false,std::memory_order_release);

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

    // First compute for raw and notch filtered version separately
//    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      dst.data[chnIdx]=ee.buf.getSample(chnIdx,smpIdx); // Raw data
     }
    }

    for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) { // set trigger/offset serially
     auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
     dst.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     dst.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx;
#ifdef EEMAGINE
     if (conf->syncOngoing.load(std::memory_order_acquire) && dst.trigger==(unsigned)TRIG_AMPSYNC) {
      if (!syncSeen[ampIdx]) {
       syncSeen[ampIdx]=true;
       syncAtIdx[ampIdx]=quint64(baseIdx)+smpIdx; // <-- our own timebase
       ++syncSeenCount;
       qInfo("<AmpSync> SYNC received by AMP#%u @bufidx=%llu",(unsigned)ampIdx+1,(unsigned long long)syncAtIdx[ampIdx]);
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

  void beginSync() {
   conf->syncOngoing.store(true,std::memory_order_release);
   //conf->syncPerformed stays as "was ever doneâ"
   std::fill(syncSeen.begin(),syncSeen.end(),false);
   std::fill(syncAtOff.begin(),syncAtOff.end(),0u);
   syncSeenCount=0;
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
    ampAlignOffset.push_back(0); // Construct trigger counts with "# of triggers=0" for each amp.
    ampIdx++;
   }
   cBufPivotList.resize(eeAmps.size());
#ifdef EEMAGINE
   syncTrigRecvd=0;
#endif

   syncAtIdx.assign(eeAmps.size(),0);

   syncSeen.assign(eeAmps.size(),false);
   syncAtOff.assign(eeAmps.size(),0u);
   syncSeenCount=0;

   chkTrig.resize(eeAmps.size()); chkTrigOffset=0;

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
   beginSync();
   sendAmpSyncByte();
   qInfo() << "<AmpSync> SYNC sent..";

   while (true) {
    fetchEegData();

    if (conf->syncOngoing.load(std::memory_order_acquire) && syncSeenCount==(unsigned)eeAmps.size()) {
     quint64 minIdx=*std::min_element(syncAtIdx.begin(),syncAtIdx.end());
     for (size_t a=0;a<eeAmps.size();++a)
      ampAlignOffset[a]=unsigned(syncAtIdx[a]-minIdx); // Should be small now (0..few)

     qInfo() << "<AmpSync> alignment offsets (samples):";
     for (int a=0;a<ampAlignOffset.size();++a)
      qInfo(" AMP#%d -> +%u",a+1,ampAlignOffset[a]);

     conf->syncOngoing.store(false,std::memory_order_release);
     conf->syncPerformed.store(true,std::memory_order_release);
     syncSeenCount=0;
    }

    const quint64 tcpDataSize=cBufPivot-cBufPivotPrev;
    if (tcpDataSize==0) { cBufPivotPrev=cBufPivot; continue; }

    // ensure capacity once; resize can still happen if tcpDataSize grows,
    // but it wont do nested heap churn per sample.
    audioBlock.resize(size_t(tcpDataSize)*AUDIO_N);
    for (quint64 i=0;i<tcpDataSize;++i) {
     float* dst=audioBlock.data()+size_t(i)*AUDIO_N;
     // audioBlock holds AUDIO_N samples per EEG sample (aligned by acquisition cadence)
     // TODO soon: add pulse detection on right channel for trigger verification.
     audioAmp->fetchN(dst,0); // fills 48 floats, or zeros on miss
    }

    quint64 start=0,newHead=0;

    {
      QMutexLocker locker(&conf->mutex);
      const quint64 head=conf->tcpBufHead; const quint64 tail=conf->tcpBufTail;
      const quint64 used=head-tail; const quint64 free=quint64(tcpBufSize)-used;
      if (tcpDataSize>free) {
       // POLICY CHOICE:
       // A) block until free (true no-overwrite, but risks upstream loss if hardware buffers overflow)
       // B) -> drop oldest (keeps newest real-time, but you lose historical samples)
       // C) drop newest (keeps continuity, but you lose current samples)
       const quint64 need=tcpDataSize-free;
       conf->tcpBufTail+=need;
       conf->droppedSamples+=need; // add this counter in ConfParam
       qWarning() << "[RING] overflow: dropped oldest" << need
                  << " used=" << used << " size=" << tcpBufSize;
      }
      start=head; newHead=head+tcpDataSize;
    }

    // no mutex: fill slots
    for (quint64 i=0;i<tcpDataSize;++i) {
     TcpSample &s=(*tcpBuffer)[(start+i)%tcpBufSize];
     unsigned hwTrig=0; int trigNonZeroCount=0;
     for (unsigned ampIdx=0;ampIdx<eeAmps.size();++ampIdx) {
      const Sample &src=eeAmps[ampIdx].cBuf[(cBufPivotPrev+i+ampAlignOffset[int(ampIdx)])%cBufSz];
      s.amp[ampIdx].copyFrom(src);

      unsigned t=src.trigger;
      if (conf->syncOngoing.load(std::memory_order_acquire) && t==0) {
       const quint64 rawIdx=(cBufPivotPrev+i+ampAlignOffset[int(ampIdx)]);
       if (pickTrigWithLookaroundExpected(eeAmps[ampIdx].cBuf.data(),rawIdx,cBufSz,unsigned(TRIG_AMPSYNC))) {
        t=unsigned(TRIG_AMPSYNC);
       }
      }

      if (t!=0) {
       ++trigNonZeroCount;
       if (hwTrig==0) hwTrig=t;
       else if (hwTrig!=t) ++trigMismatchCounter;
      }
     }
     // detect missing triggers on some amps
     if (hwTrig!=0 && trigNonZeroCount!=int(eeAmps.size())) ++trigMissingCounter;
     s.trigger=hwTrig; s.userEvent=0; // future
     ++trigProduced; if (hwTrig) ++trigNonZeroProduced;
     // audio copy as before
     const float* aud=audioBlock.data()+size_t(i)*AUDIO_N;
     // TODO soon: verify trigger timing from audio right-channel pulse
     // (disabled now; should not affect realtime path)
     // const bool audioPulse=detectAudioPulse(aud, maybe conf thresholds);
     // if (audioPulse) { optional: log / compare with hwTrig }
     for (int k=0;k<AUDIO_N;++k) s.audioN[k]=aud[k];
     s.offset=counter0++; s.timestampMs=0;
    }

    {
      QMutexLocker locker(&conf->mutex);
      conf->tcpBufHead=newHead;
      static quint64 lastH=0,lastTail=0; static qint64  lastMsRing=0;
      log_ring_1hz("ACQ:PROD", conf->tcpBufHead,conf->tcpBufTail,lastH,lastTail,lastMsRing);
      // --- TRIG stats gated to ~1Hz ---
      static qint64 lastMsTrig=0;
      const qint64 nowMs=QDateTime::currentMSecsSinceEpoch();
      if (nowMs-lastMsTrig>=1000) {
       static quint64 lastMismatch=0,lastMissing=0;
       static quint64 lastTrigProduced=0,lastTrigNZ=0;
       const quint64 dMismatch=trigMismatchCounter-lastMismatch;
       const quint64 dMissing =trigMissingCounter -lastMissing;
       const quint64 dT       =trigProduced       -lastTrigProduced;
       const quint64 dNZ      =trigNonZeroProduced-lastTrigNZ;
       // Always show rate once per second (or gate further if you want)
       qInfo() << "[TRIGRATE] produced/s=" << dT << " trig/s=" << dNZ;
       // Only warn on problems
       if (dNZ||dMismatch||dMissing) {
        qWarning() << "[TRIGSTAT] mismatch/s=" << dMismatch << " missing/s=" << dMissing;
       }
       lastMismatch    =trigMismatchCounter;
       lastMissing     =trigMissingCounter;
       lastTrigProduced=trigProduced;
       lastTrigNZ      =trigNonZeroProduced;
       lastMsTrig      =nowMs;
      }
    }  

    cBufPivotPrev=cBufPivot;

#ifndef EEMAGINE
    msleep(conf->eegProbeMsecs);
#endif

    counter1++;
   } // EEG stream

   for (auto& e:eeAmps) { delete e.str; delete e.amp; } // EEG stream stops..
   qInfo("<AcqThread> Exiting thread..");
  }

 public slots:
  void requestAmpSync() {
   // If already in a sync window, dont allow re-triggering.
   if (conf->syncOngoing.load(std::memory_order_acquire)) {
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
   beginSync();       // sets syncOngoing=true and resets arrays
   sendAmpSyncByte(); // actually write to SparkFun/amps
   qInfo() << "<AmpSync> SYNC sent";
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
   qInfo() << "[TRIG] synthetic trigger sent:" << t;
#endif
  }

 private:
  static inline unsigned pickTrigWithLookaroundExpected(const Sample* buf,quint64 idx,unsigned cBufSz,unsigned expected) {
   const unsigned t0=buf[idx%cBufSz].trigger; if (t0==expected) return expected;
   // Â±1 sample grace (cheap and deterministic)
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

  unsigned int syncTrigRecvd; QVector<unsigned int> ampAlignOffset; // Trigger offsets for syncronization.
  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  quint64 counter0,counter1;

  // --- Hardware pointers -- coming from the sky
  std::vector<amplifier*> *eeAmpsOrig;
  AudioAmp *audioAmp;
  SerialDevice *serDev;

  std::vector<float> audioBlock;

  std::vector<bool> syncSeen;          // size=ampCount
  std::vector<unsigned int> syncAtOff; // size=ampCount (the offset value when TRIG_AMPSYNC observed)
  unsigned int syncSeenCount=0;

  quint64 trigMismatchCounter=0;
  quint64 trigMissingCounter=0;
  quint64 trigProduced=0;
  quint64 trigNonZeroProduced=0;

  qint64 lastSyncSendMs=0;  // only touched by AcqThread
  static constexpr int syncCooldownMs=1000; // 1s of SYNC debouncing/cooling

  std::vector<quint64> syncAtIdx;
};
