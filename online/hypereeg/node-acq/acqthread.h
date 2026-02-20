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
//#include <QDateTime>
//#include <QDataStream>

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
#include "../common/rt_bootstrap.h"

const int CBUF_SIZE_IN_SECS=10;

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(ConfParam *c,QObject *parent=nullptr,
            std::vector<amplifier*> *ee=nullptr,AudioAmp *aud=nullptr,SerialDevice *ser=nullptr) : QThread(parent) {
   conf=c; eeAmpsOrig=ee; audioAmp=aud; serDev=ser;

   conf->syncOngoing=conf->syncPerformed=false;

   smp=Sample(conf->physChnCount); tcpEEG=TcpSample(conf->ampCount,conf->physChnCount);
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&(conf->tcpBuffer);

   cBufPivotPrev=cBufPivot=counter0=counter1=0;
   conf->tcpBufHead=0;

   audioBlock.reserve(size_t(conf->eegRate)*AUDIO_N); // 1sec is worst case
   //outBlock.reserve(size_t(conf->eegRate));
   for (unsigned i=0;i<tcpBufSize;++i) (*tcpBuffer)[i].initSizeOnly(conf->ampCount,conf->physChnCount);
  }

  // --------

  //inline void ensureOutBlockSize(quint64 n) {
  // if (outBlock.size()>=size_t(n)) return;
  // const size_t old=outBlock.size(); outBlock.resize(size_t(n));
  // // Allocate amp+Sample buffers once
  // for (size_t i=old;i<outBlock.size();++i) outBlock[i].initSizeOnly(conf->ampCount,conf->physChnCount);
  //}

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
     if (conf->syncOngoing) {
      if (dst.trigger!=0) {
       if (dst.trigger==(unsigned)TRIG_AMPSYNC) {
        qInfo("<AmpSync> SYNC received by @AMP#%u -- %u",(unsigned)ampIdx+1,(unsigned)dst.offset);
        ampTrigOffset[ampIdx]=dst.offset;
        ++syncTrigRecvd; // arrived to one of amps, wait for the rest... - also; serial here -> no atomics needed
       } else { // Other trigger than SYNC
        qInfo("<AmpSync> Trigger #%u arrived at AMP#%u -- @sampleoffset=%u",(unsigned)dst.trigger,
                                                                            (unsigned)ampIdx+1,
                                                                            (unsigned)dst.offset);
       }
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

#ifdef __linux__
   lock_memory_or_warn();
   pin_thread_to_cpu(pthread_self(),1); // Pin acquisition to core 1
   set_thread_rt(pthread_self(),SCHED_FIFO,80); // Give acquisition RT priority
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
    ampTrigOffset.push_back(0); // Construct trigger counts with "# of triggers=0" for each amp.
    ampIdx++;
   }
   cBufPivotList.resize(eeAmps.size());
#ifdef EEMAGINE
   syncTrigRecvd=0;
#endif

   chkTrig.resize(eeAmps.size()); chkTrigOffset=0;

   // Initialize EEG streams of amps.
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
   sendTrigger(TRIG_AMPSYNC);
   qInfo() << "<AmpSync> SYNC sent..";

   while (true) {
#ifndef EEMAGINE
    unsigned t=nonHWTrig.load(std::memory_order_acquire); // Manual TRIG_AMPSYNC put within fetchEegData() ?
    if (t==TRIG_AMPSYNC) { // consume only if it's still TRIG_AMPSYNC
     unsigned expected=TRIG_AMPSYNC;
     // We set it here for all ampsi -> It will be received later for inter-amp syncronization
     if (nonHWTrig.compare_exchange_strong(expected,0,std::memory_order_acq_rel)) {
      for (EEAmp& e:eeAmps) (e.amp)->setTrigger(TRIG_AMPSYNC);
      syncTrigRecvd=eeAmps.size();
     }
    }
#endif

    fetchEegData();

    // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
    if (syncTrigRecvd==eeAmps.size()) {
     qInfo() << "<AmpSync> SYNC received by all amps.. validating offsets.. ";
     unsigned int trigOffsetMin=*std::min_element(ampTrigOffset.begin(),ampTrigOffset.end());
     // The following table determines the continuous offset additions for each amp to be aligned.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) ampTrigOffset[ampIdx]-=trigOffsetMin;
     unsigned int trigOffsetMax=*std::max_element(ampTrigOffset.begin(),ampTrigOffset.end());
//     if (trigOffsetMax>conf->eegRate) {
//      qWarning() << "<AmpSync> Warning!! SYNC is not recvd within a second for at least one amp!";
//     } else {
     qInfo() << "<AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
     for (int trigIdx=0;trigIdx<ampTrigOffset.size();trigIdx++) qInfo(" AMP#%d -> %d",trigIdx+1,ampTrigOffset[trigIdx]);
     qInfo() << "<AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
//     }
     conf->syncOngoing=false; conf->syncPerformed=true; syncTrigRecvd=0; // Ready for future SYNCing to update ampTrigOffset[i] values
    }

    const quint64 tcpDataSize=cBufPivot-cBufPivotPrev;
    if (tcpDataSize==0) { cBufPivotPrev=cBufPivot; continue; }

    // ensure capacity once; resize can still happen if tcpDataSize grows,
    // but it wont do nested heap churn per sample.
    audioBlock.resize(size_t(tcpDataSize)*AUDIO_N);
    for (quint64 i=0;i<tcpDataSize;++i) {
     float* dst=audioBlock.data()+size_t(i)*AUDIO_N;
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
     // make sure slot has correct shape once (no heavy work)
     for (unsigned ampIdx=0;ampIdx<eeAmps.size();++ampIdx) {
      s.amp[ampIdx].copyFrom(eeAmps[ampIdx].cBuf[(cBufPivotPrev+i+ampTrigOffset[ampIdx])%cBufSz]);
     }
     s.trigger=0;
     // we want trigger only on first produced sample:
     if (i==0) { unsigned t=nonHWTrig.exchange(0,std::memory_order_acq_rel); s.trigger=t; }
     const float* aud=audioBlock.data()+size_t(i)*AUDIO_N;
     for (int k=0;k<AUDIO_N;++k) s.audioN[k]=aud[k];
     s.offset=counter0++; s.timestampMs=0;
    }
    
    {
      QMutexLocker locker(&conf->mutex);
      conf->tcpBufHead=newHead;
      static quint64 lastH=0,lastT=0; static qint64 lastMs=0;
      log_ring_1hz("ACQ:PROD",conf->tcpBufHead,conf->tcpBufTail,lastH,lastT,lastMs);
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

  void sendTrigger(unsigned int t) {
   conf->syncOngoing=true;
   if (t<256) { // also including TRIG_AMPSYNC=255
#ifdef EEMAGINE
    unsigned char trig=(unsigned char)t;
    serDev->write(trig);
#else
    nonHWTrig=t; // The same for eesynth.h -- includes TRIG_AMPSYNC
#endif
   } else { // Non-hardware trigger
    nonHWTrig=t; // Put manually to trigger part in TCPBuffer by within thread loop
   }
  }

 private:
  ConfParam *conf;

  TcpSample tcpEEG; Sample smp;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize;

  std::vector<EEAmp> eeAmps; unsigned int cBufSz,smpCount,chnCount;
  quint64 cBufPivotPrev,cBufPivot; std::vector<unsigned int> cBufPivotList;

  unsigned int syncTrigRecvd; QVector<unsigned int> ampTrigOffset; // Trigger offsets for syncronization.
  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  quint64 counter0,counter1;
  std::atomic_uint nonHWTrig{0};

  // --- Hardware pointers -- coming from the sky
  std::vector<amplifier*> *eeAmpsOrig;
  AudioAmp *audioAmp;
  SerialDevice *serDev;

  std::vector<float> audioBlock;   // size=tcpDataSize*AUDIO_N
};
