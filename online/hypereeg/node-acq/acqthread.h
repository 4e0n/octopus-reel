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
#include <QDataStream>

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

#include "confparam.h"
#include "../common/globals.h"
#include "../common/sample.h"
#include "../common/tcpsample.h"
#include "serialdevice.h"
#include "eeamp.h"

#ifdef AUDIODEV
#include "audioamp.h"
#endif

const int CBUF_SIZE_IN_SECS=10;

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c; nonHWTrig=0;

   conf->syncOngoing=conf->syncPerformed=false;

   serDev.init();

   smp=Sample(conf->physChnCount); tcpEEG=TcpSample(conf->ampCount,conf->physChnCount);
   tcpBufSize=conf->tcpBufSize; tcpBuffer=&(conf->tcpBuffer);

   cBufPivotPrev=cBufPivot=counter0=counter1=0;
   conf->tcpBufHead=0;
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
      qWarning() << "node-acq: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    // -----
    const int eegCh=int(chnCount-2); const unsigned baseIdx=ee.cBufIdx;

    // First compute for raw and notch filtered version separately
    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     auto &notch=ee.filterListN[chnIdx];
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      float xR=ee.buf.getSample(chnIdx,smpIdx); // Raw data
      //float xN=xR; // no pre-notch applied
      float xN=notch.filterSample(xR); // pre-notch applied
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      dst.dataR[chnIdx]=xR; dst.dataN[chnIdx]=xN;
     }
    }

    // Retouch it for interpolated and switched-off electrodes,
    // As the interpolated channels are potentially sparse in number, this part, currently traversing all,
    // will be converted to a specific/to-the-point list of interpolated channels.
    {
     QMutexLocker locker(&conf->chnInterMutex);
     for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
      for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
       auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
       // Interpolation
       float xN=dst.dataN[chnIdx];
       unsigned int interMode=conf->chnInfo[chnIdx].interMode[ee.id];
       if (interMode==2) { xN=0.;
        int eSz=conf->chnInfo[chnIdx].interElec.size();
        for (int idx=0;idx<eSz;idx++) xN+=dst.dataN[ conf->chnInfo[chnIdx].interElec[idx] ];
        dst.dataN[chnIdx]=xN/(float)(eSz);
       } else if (interMode==0) {
        xN=0.; dst.dataN[chnIdx]=xN;
       }
      }
     }
/*
     for (int interIdx=0;interIdx<conf->interList.size();++interIdx) {
      int chnIdx=conf->interList[ampIdx][interIdx];
      for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
       auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
       // Interpolate
       float xN=0; int eSz=conf->chnInfo[chnIdx].interElec.size();
       for (int idx=0;idx<eSz;idx++) xN+=dst.dataN[ conf->chnInfo[chnIdx].interElec[idx] ];
       dst.dataN[chnIdx]=xN/(float)(eSz);
      }
     }
     for (int offIdx=0;offIdx<conf->offList.size();++offIdx) {
      int chnIdx=conf->offList[ampIdx][offIdx];
      for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
       auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
       // Switch off
       float xN=0; dst.dataN[chnIdx]=xN;
      }
     }
*/
    }

    // Then compute 2-40 Hz and delta,theta,alpha,beta,gamma bands from the notch filtered version
    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     auto &bp=ee.filterListBP[chnIdx];
     auto &delta=ee.filterListD[chnIdx]; auto &theta=ee.filterListT[chnIdx];
     auto &alpha=ee.filterListA[chnIdx]; auto &beta=ee.filterListB[chnIdx]; auto &gamma=ee.filterListG[chnIdx];
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      float xN=dst.dataN[chnIdx]; float xBP=bp.filterSample(xN);
      //float c=std::abs(x*x-xN*xN)*1e10f;
      float xD=delta.filterSample(xN); float xT=theta.filterSample(xN);
      float xA=alpha.filterSample(xN); float xB=beta.filterSample(xN); float xG=gamma.filterSample(xN);
      dst.dataBP[chnIdx]=xBP;
      dst.dataD[chnIdx]=xD; dst.dataT[chnIdx]=xT;
      dst.dataA[chnIdx]=xA; dst.dataB[chnIdx]=xB; dst.dataG[chnIdx]=xG;
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
        qInfo("node-acq: <AmpSync> SYNC received by @AMP#%u -- %u",(unsigned)ampIdx+1,(unsigned)dst.offset);
        ampTrigOffset[ampIdx]=dst.offset;
        ++syncTrigRecvd; // arrived to one of amps, wait for the rest... - also; serial here -> no atomics needed
       } else { // Other trigger than SYNC
        qInfo("node-acq: <AmpSync> Trigger #%u arrived at AMP#%u -- @sampleoffset=%u",(unsigned)dst.trigger,
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
#ifdef EEMAGINE
   using namespace eemagine::sdk; factory eeFact("libeego-SDK.so");
#else
   using namespace eesynth; factory eeFact(conf->ampCount);
#endif

   // --- Initial setup ---

#ifdef AUDIODEV
   audioAmp.init();
   if (!audioAmp.initAlsa()) {
    qWarning() << "<AcqThread> AudioAmp ALSA init failed";
   } else {
    audioAmp.start();
   }
#endif

   std::vector<amplifier*> eeAmpsUnsorted,eeAmpsSorted; std::vector<unsigned int> sUnsorted,serialNos;
   eeAmpsUnsorted=eeFact.getAmplifiers();
   if (eeAmpsUnsorted.size()<conf->ampCount) {
    qDebug() << "node-acq: FATAL ERROR!!! <acqthread_amp_setup> At least one  of the amplifiers is offline!";
    return;
   }

   // Sort amplifiers for their serial numbers
   for (auto& ee:eeAmpsUnsorted) sUnsorted.push_back(stoi(ee->getSerialNumber()));
   serialNos=sUnsorted; std::sort(sUnsorted.begin(),sUnsorted.end());
   for (auto& sNo:serialNos) { std::size_t ampIdx=0;
    for (auto& sNoU:sUnsorted) {
     if (sNo==sNoU) eeAmpsSorted.push_back(eeAmpsUnsorted[ampIdx]);
     ampIdx++;
    }
   }

   // Construct main EE structure
   quint64 rMask,bMask; // Create channel selection masks -- subsequent channels assumed..
   rMask=0; for (unsigned int i=0;i<conf->refChnCount;i++) { rMask<<=1; rMask|=1; }
   bMask=0; for (unsigned int i=0;i<conf->bipChnCount;i++) { bMask<<=1; bMask|=1; }
   //qInfo("node-acq: <acqthread_amp_setup> RBMask: %llx %llx",rMask,bMask);
   std::size_t ampIdx=0;
   for (auto& ee:eeAmpsSorted) {
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

   // ----- List unsorted vs. sorted
   { std::size_t ampIdx=0;
    for (auto& ee:eeAmps) {
     qInfo("node-acq: <AmpSerial> Amp#%d: %d",(int)ampIdx+1,stoi(ee.amp->getSerialNumber()));
     ampIdx++;
    }
   }

   chkTrig.resize(eeAmps.size()); chkTrigOffset=0;

#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   // Initialize EEG streams of amps.
   //for (auto& e:eeAmps) e.str=e.amp->OpenEegStream(conf->eegRate,conf->refGain,conf->bipGain,e.chnList);
   for (auto& e:eeAmps) e.str=e.OpenEegStream(conf->eegRate,conf->refGain,conf->bipGain,e.chnList);
   qInfo() << "node-acq: <acqthread_switch2eeg> EEG upstream started..";

   // EEG stream
   // -----------------------------------------------------------------------------------------------------------------
   fetchEegData(); // The first round of acquisition - to preadjust certain things

   // Send SYNC
   sendTrigger(TRIG_AMPSYNC);
   qInfo() << "node-acq: <AmpSync> SYNC sent..";

   while (true) {
#ifndef EEMAGINE
    if (nonHWTrig==TRIG_AMPSYNC) { nonHWTrig=0; // Manual TRIG_AMPSYNC put within fetchEegData0() ?
     for (EEAmp& e:eeAmps) (e.amp)->setTrigger(TRIG_AMPSYNC); // We set it here for all amps
                                                              // It will be received later for inter-amp syncronization
     syncTrigRecvd=eeAmps.size();
    }
#endif
    fetchEegData();

    // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
    if (eeAmps.size()>1 && syncTrigRecvd==eeAmps.size()) {
     qInfo() << "node-acq: <AmpSync> SYNC received by all amps.. validating offsets.. ";
     unsigned int trigOffsetMin=*std::min_element(ampTrigOffset.begin(),ampTrigOffset.end());
     // The following table determines the continuous offset additions for each amp to be aligned.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) ampTrigOffset[ampIdx]-=trigOffsetMin;
     unsigned int trigOffsetMax=*std::max_element(ampTrigOffset.begin(),ampTrigOffset.end());
//     if (trigOffsetMax>conf->eegRate) {
//      qDebug() << "node-acq: ERROR!!! <AmpSync> SYNC is not recvd within a second for at least one amp!";
//     } else {
     qInfo() << "node-acq: <AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
     for (int trigIdx=0;trigIdx<ampTrigOffset.size();trigIdx++) qDebug(" AMP#%d -> %d",trigIdx+1,ampTrigOffset[trigIdx]);
     qInfo() << "node-acq: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
//     }
     conf->syncOngoing=false; conf->syncPerformed=true; syncTrigRecvd=0; // Ready for future SYNCing to update ampTrigOffset[i] values
    }

#ifdef AUDIODEV
    quint64 tcpDataSize=cBufPivot-cBufPivotPrev; audioN.resize(tcpDataSize);
    for (quint64 tcpDataIdx=0;tcpDataIdx<tcpDataSize;tcpDataIdx++) {
     audioN[tcpDataIdx].resize(AUDIO_N);
     unsigned trigOff=audioAmp.fetchN(audioN[tcpDataIdx].data(),20); // wait_ms
     //if (trigOff != UINT_MAX) {
     // sample.audioTrigger = trigOff; // trigger offset (0..47)
     //} else {
     // sample.audioTrigger = -1;
     //}
    }
#endif

    // ===================================================================================================================

    {
     QMutexLocker locker(&conf->mutex);
     for (quint64 tcpDataIdx=0;tcpDataIdx<tcpDataSize;tcpDataIdx++) {

      for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) { // Align each amp to the latest ampTrigOffset[i]
       tcpEEG.amp[ampIdx]=eeAmps[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+ampTrigOffset[ampIdx])%cBufSz];
      }

      tcpEEG.trigger=0;
      if (nonHWTrig) { tcpEEG.trigger=nonHWTrig; nonHWTrig=0; } // Set NonHW trigger in hyperEEG data struct

    // Receive current triggers from all amps.. They are supposed to be the same in value.
    //for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) chkTrig[ampIdx]=tcpEEG.amp[ampIdx].trigger;
    //bool syncFlag=true;
    //for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) if (chkTrig[ampIdx]!=chkTrig[0]) { syncFlag=false; break; }
    //if (syncFlag) {
    // if (chkTrig[0]!=0) {
    //  qInfo() << "node-acq: <AmpSync> Yay!!! Syncronized triggers received!";
    //  chkTrigOffset++;
    // }
    //} else {
    // qDebug() << "node-acq: ERROR!!! <AmpSync> That's bad. Single offset lag..:" << syncFlag;
    // for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) qDebug() << "Trig" << ampIdx << ":" << chkTrig[ampIdx];
    // qDebug() << "-> Offset:" << chkTrigOffset;
    // chkTrigOffset=0;
    //}

//      if (conf->syncPerformed) {
//       bool trigErrorFlag=false;
//       unsigned int trig=tcpEEG.amp[0].trigger;
//       if (trig!=0) {
//        for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) {
//         if (trig!=tcpEEG.amp[ampIdx].trigger) {
//          trigErrorFlag=true;
//          break;
//         }
//        }
//        if (trigErrorFlag) {
//         conf->syncPerformed=false;
//         qDebug() << "node-acq: ERROR!!! <AmpSync> That's bad. Some offset lag(s) exist between amps..";
//        } else {
//         tcpEEG.trigger=trig;
//         qDebug() << "node-acq: <AmpSync> Good. All triggers in amps coincide.";
//        }
//       }
//      }

#ifdef AUDIODEV
      for (int smpIdx=0;smpIdx<AUDIO_N;smpIdx++) tcpEEG.audioN[smpIdx]=audioN[tcpDataIdx][smpIdx];
#endif

      tcpEEG.offset=counter0; counter0++;

      tcpEEG.trigger=0.; // Reset trigger in any case
      //tcpEEG.timestampMs=QDateTime::currentMSecsSinceEpoch();
      tcpEEG.timestampMs=0;

//      const qint64 avail = conf->tcpBufHead - conf->tcpBufTail;
//      const qint64 free  = conf->tcpBufSize - avail;
//      if (free < tcpDataSize) {
//       // choose policy:
//       // 1) drop oldest (advance tail)
//       // 2) drop newest (skip writing)
//       // 3) block (not recommended in acq)
//       const qint64 need = tcpDataSize - free;
//       conf->tcpBufTail += need;   // drop oldest 'need' samples
//       conf->droppedSamples += need;  // optional counter
//      }

      (*tcpBuffer)[(conf->tcpBufHead+tcpDataIdx)%tcpBufSize]=tcpEEG; // Push to Circular Buffer

      //static qint64 t0=0;
      //qint64 now=QDateTime::currentMSecsSinceEpoch();
      //if (now-t0>1000) { t0=now;
      // const auto &s = tcpEEG;
      // qDebug() << "[PROD] off" << s.offset << "mag" << QString::number(s.MAGIC,16)
      //          << "eeg0" << s.amp[0].dataBP[0]
      //          << "aud0" << s.audioN[0];
      //}

     }
     conf->tcpBufHead+=tcpDataSize; // Update producer index
    }

    cBufPivotPrev=cBufPivot;

    //chnCount=conf->physChnCount;

#ifndef EEMAGINE
    msleep(conf->eegProbeMsecs);
#endif

    counter1++;
   } // EEG stream

   for (auto& e:eeAmps) { delete e.str; delete e.amp; } // EEG stream stops..
   qInfo("node-acq: <acqthread> Exiting thread..");
  }

  void sendTrigger(unsigned int t) {
   conf->syncOngoing=true;
   if (t<256) { // also including TRIG_AMPSYNC=255
#ifdef EEMAGINE
    unsigned char trig=(unsigned char)t;
    serDev.write(trig);
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

  SerialDevice serDev;

  unsigned int syncTrigRecvd; QVector<unsigned int> ampTrigOffset; // Trigger offsets for syncronization.
  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  quint64 counter0,counter1; unsigned int nonHWTrig;

#ifdef AUDIODEV
  AudioAmp audioAmp;
  std::vector<std::vector<float>> audioN;
#endif
};
