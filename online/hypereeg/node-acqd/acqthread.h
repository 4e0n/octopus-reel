/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#ifndef ACQTHREAD_H
#define ACQTHREAD_H

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
#include "../globals.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "../tcpcmarray.h"
#include "../serialdevice.h"
#include "eeamp.h"
#include "audioamp.h"
#include "cmodutility.h"

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c; nonHWTrig=0;

   serDev.init();

   smp=Sample(conf->physChnCount);
   tcpEEGBufSize=conf->tcpBufSize*conf->eegRate; tcpEEG=TcpSample(conf->ampCount,conf->physChnCount);
   tcpEEGBuffer=QVector<TcpSample>(tcpEEGBufSize,TcpSample(conf->ampCount,conf->physChnCount));

   tcpCMBufSize=conf->tcpBufSize*conf->cmRate; tcpCM=TcpCMArray(conf->ampCount,conf->physChnCount);

   // init once at startup:
   tcpCM.init(conf->ampCount, chnCount, /*withReasons=*/true);

   tcpCMBuffer=QVector<TcpCMArray>(tcpCMBufSize,TcpCMArray(conf->ampCount,conf->physChnCount));

   // This buffer contains difference between squares of unfiltered and filtered data back in time for 1 sec.
   cmRMSBufIdx=0; cmRMSBufSize=conf->eegRate; // 1 second
   cmRMSBuf.resize(conf->ampCount);
   for (auto& amp:cmRMSBuf) amp.resize(conf->physChnCount);
   for (auto& amp:cmRMSBuf) for (auto& chn:amp) chn.resize(cmRMSBufSize);

   cBufPivotPrev=cBufPivot=tcpEEGBufHead=tcpEEGBufTail=tcpCMBufHead=tcpCMBufTail=counter0=counter1=0;
  }

  // --------

  void fetchEegData0() {
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
      qWarning() << "hnode_acqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    // -----
    const int eegCh=int(chnCount-2); const unsigned baseIdx=ee.cBufIdx;
    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     auto &bp=ee.bpFilterList[chnIdx]; auto &nf=ee.nFilterList[chnIdx];
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      float xR=ee.buf.getSample(chnIdx,smpIdx);
      float x=bp.filterSample(xR); float xf=nf.filterSample(x);
      float c=std::abs(x*x-xf*xf)*1e10f;
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      dst.dataR[chnIdx]=xR; dst.data[chnIdx]=x; dst.dataF[chnIdx]=xf;
      cmRMSBuf[ampIdx][chnIdx][cmRMSBufIdx%cmRMSBufSize]=c;
     }
    }
    for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) { // set trigger/offset serially
     auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
     dst.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     dst.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx;
    }
    // -----
    ee.cBufIdx+=ee.smpCount; // advance head once per block
    ampIdx++;
   }
   cBufPivotPrev=cBufPivot;
   cBufPivot=*std::min_element(cBufPivotList.begin(),cBufPivotList.end());
   cmRMSBufIdx++;
  }

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
      qWarning() << "hnode_acqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    // -----
    const int eegCh=int(chnCount-2); const unsigned baseIdx=ee.cBufIdx;
    #pragma omp parallel for schedule(static) if(eegCh>=8)
    for (int chnIdx=0;chnIdx<eegCh;++chnIdx) {
     auto &bp=ee.bpFilterList[chnIdx]; auto &nf=ee.nFilterList[chnIdx];
     for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) {
      float xR=ee.buf.getSample(chnIdx,smpIdx);
      float x=bp.filterSample(xR); float xf=nf.filterSample(x);
      float c=std::abs(x*x-xf*xf)*1e10f;
      auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
      dst.dataR[chnIdx]=xR; dst.data[chnIdx]=x; dst.dataF[chnIdx]=xf;
      cmRMSBuf[ampIdx][chnIdx][cmRMSBufIdx%cmRMSBufSize]=c;
     }
    }
    for (unsigned smpIdx=0;smpIdx<ee.smpCount;++smpIdx) { // set trigger/offset serially
     auto &dst=ee.cBuf[(baseIdx+smpIdx)%cBufSz];
     dst.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     dst.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx;
     if (dst.trigger!=0) {
      if (dst.trigger==(unsigned)TRIG_AMPSYNC) {
       qInfo("hnode_acqd: <AmpSync> SYNC received by @AMP#%u -- %u",(unsigned)ampIdx+1,(unsigned)dst.offset);
       arrivedTrig[ampIdx]=dst.offset;
       ++syncTrig; // arrived to one of amps, wait for the rest... - also; serial here -> no atomics needed
      } else { // Other trigger than SYNC
       qInfo("hnode_acqd: <AmpSync> Trigger #%u arrived at AMP#%u -- @sampleoffset=%u",(unsigned)dst.trigger,(unsigned)ampIdx+1,(unsigned)dst.offset);
      }
     }
    }
    // -----
    ee.cBufIdx+=ee.smpCount; // advance head once per block
    ampIdx++;
   }
   cBufPivotPrev=cBufPivot;
   cBufPivot=*std::min_element(cBufPivotList.begin(),cBufPivotList.end());
   cmRMSBufIdx++;
  }

  void run() override {
#ifdef EEMAGINE
   using namespace eemagine::sdk; factory eeFact("libeego-SDK.so");
#else
   using namespace eesynth; factory eeFact(conf->ampCount);
#endif

   // --- Initial setup ---

   if (Ssmooth_.empty()) {
    Ssmooth_.assign(conf->ampCount,std::vector<float>(conf->physChnCount,0.f));
    tmpAmpChn_.assign(conf->ampCount,std::vector<float>(conf->physChnCount,0.f));
    tmpAbsDev_.assign(conf->ampCount,std::vector<float>(conf->physChnCount,0.f));
    cmWarmupFilled_=0;
    isBad_.assign(conf->ampCount,std::vector<uint8_t>(conf->physChnCount,0));
   }


   audioAmp.init();
   if (!audioAmp.initAlsa()) {
    qWarning() << "<AcqThread> AudioAmp ALSA init failed";
   } else {
    audioAmp.start();
   }

   std::vector<amplifier*> eeAmpsUnsorted,eeAmpsSorted; std::vector<unsigned int> sUnsorted,serialNos;
   eeAmpsUnsorted=eeFact.getAmplifiers();
   if (eeAmpsUnsorted.size()<conf->ampCount) {
    qDebug() << "hnode_acqd: FATAL ERROR!!! <acqthread_amp_setup> At least one  of the amplifiers is offline!";
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
   //qInfo("hnode_acqd: <acqthread_amp_setup> RBMask: %llx %llx",rMask,bMask);
   std::size_t ampIdx=0;
   for (auto& ee:eeAmpsSorted) {
    cBufSz=conf->eegRate*CBUF_SIZE_IN_SECS;
    EEAmp e(ampIdx,ee,rMask,bMask,cBufSz,cBufSz/2,conf->physChnCount);
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    eeAmps.push_back(e);
    arrivedTrig.push_back(0); // Construct trigger counts with "# of triggers=0" for each amp.
    ampIdx++;
   }
   cBufPivotList.resize(eeAmps.size());
   syncTrig=0;

   // ----- List unsorted vs. sorted
   { std::size_t ampIdx=0;
    for (auto& ee:eeAmps) {
     qInfo("hnode_acqd: <AmpSerial> Amp#%d: %d",(int)ampIdx+1,stoi(ee.amp->getSerialNumber()));
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
   qInfo() << "hnode_acqd: <acqthread_switch2eeg> EEG upstream started..";

   // EEG stream
   // -----------------------------------------------------------------------------------------------------------------
   fetchEegData0(); // The first round of acquisition - to preadjust certain things

   // Send SYNC

   //audioAmp.expectStartupSync(currentEEGtick); 

   sendTrigger(TRIG_AMPSYNC);
   qInfo() << "hnode_acqd: <AmpSync> SYNC sent..";

   while (true) {
#ifndef EEMAGINE
    if (nonHWTrig==TRIG_AMPSYNC) { nonHWTrig=0; // Manual TRIG_AMPSYNC put within fetchEegData0() ?
     for (EEAmp& e:eeAmps) (e.amp)->setTrigger(TRIG_AMPSYNC); // We set it here for all amps
                                                              // It will be received later for inter-amp syncronization
    }
#endif
    fetchEegData();

    // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
    if (eeAmps.size()>1 && syncTrig==eeAmps.size()) {
     qInfo() << "hnode_acqd: <AmpSync> SYNC received by all amps.. validating offsets.. ";
     unsigned int trigOffsetMin=*std::min_element(arrivedTrig.begin(),arrivedTrig.end());
     // The following table determines the continuous offset additions for each amp to be aligned.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) arrivedTrig[ampIdx]-=trigOffsetMin;
     unsigned int trigOffsetMax=*std::max_element(arrivedTrig.begin(),arrivedTrig.end());
     if (trigOffsetMax>conf->eegRate) {
      qDebug() << "hnode_acqd: ERROR!!! <AmpSync> SYNC is not recvd within a second for at least one amp!";
     } else {
      qInfo() << "hnode_acqd: <AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
      for (int trigIdx=0;trigIdx<arrivedTrig.size();trigIdx++) qDebug(" AMP#%d -> %d",trigIdx+1,arrivedTrig[trigIdx]);
      qInfo() << "hnode_acqd: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
     }
     syncTrig=0; // Ready for future SYNCing to update arrivedTrig[i] values
    }

    // ===================================================================================================================

    quint64 tcpDataSize=cBufPivot-cBufPivotPrev; //qInfo() << cBufPivotPrev << cBufPivot;
    for (quint64 tcpDataIdx=0;tcpDataIdx<tcpDataSize;tcpDataIdx++) {
     // Alignment of each amplifier to the latest offset by +arrivedTrig[i]
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) {
      //qDebug() << "daemon pre-serialize: amp=" << ampIdx
      //         << " raw=" << ee[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz].data[0]
      //         << " flt=" << ee[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz].dataF[0];
      tcpEEG.amp[ampIdx]=eeAmps[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz];
      //if (ampIdx==0) qDebug() << "daemon: filled  amp=" << ampIdx << " ch=0 - val=" << tcpEEG.amp[ampIdx].data[0] << tcpEEG.amp[ampIdx].dataF[0];
     }
     tcpEEG.trigger=0;
     if (nonHWTrig) { tcpEEG.trigger=nonHWTrig; nonHWTrig=0; } // Set NonHW trigger in hyperEEG data struct

     // Receive current triggers from all amps.. They are supposed to be the same in value.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) chkTrig[ampIdx]=tcpEEG.amp[ampIdx].trigger;
     bool syncFlag=true;
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) if (chkTrig[ampIdx]!=chkTrig[0]) { syncFlag=false; break; }
     if (syncFlag) {
      if (chkTrig[0]!=0) {
       qInfo() << "hnode_acqd: <AmpSync> Yay!!! Syncronized triggers received!";
       chkTrigOffset++;
      }
     } else {
      qDebug() << "hnode_acqd: ERROR!!! <AmpSync> That's bad. Single offset lag..:" << syncFlag;
      for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) qDebug() << "Trig" << ampIdx << ":" << chkTrig[ampIdx];
      qDebug() << "-> Offset:" << chkTrigOffset;
      chkTrigOffset=0;
     }

     unsigned trigOff=audioAmp.fetch48(tcpEEG.audio48, /*wait_ms*/ 20);
     //if (trigOff != UINT_MAX) {
     // sample.audioTrigger = trigOff; // trigger offset (0..47)
     //} else {
     // sample.audioTrigger = -1;
     //}

     tcpEEG.timestampMs=QDateTime::currentMSecsSinceEpoch();

     tcpEEG.offset=counter0; counter0++;

     tcpEEGBuffer[(tcpEEGBufHead+tcpDataIdx)%tcpEEGBufSize]=tcpEEG; // Push to Circular Buffer

     //if (counter0%1000==0) qDebug("BUFFER(mod1000) SENT! tcpEEG.offset-> %lld - Magic: %x",tcpEEG.offset,tcpEEG.MAGIC);

     // Record RAW EEG to files..
     if (conf->dumpRaw) {
      chnCount=conf->physChnCount; unsigned int ampCount=eeAmps.size();
      for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
       if (ampIdx==0) conf->hEEGStream << tcpEEG.offset;
       for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) conf->hEEGStream << tcpEEG.amp[ampIdx].data[chnIdx];
       if (ampIdx==ampCount-1) conf->hEEGStream << tcpEEG.trigger;
      }
     }
    }
 
    tcpEEGBufHead+=tcpDataSize; // Update producer index
    cBufPivotPrev=cBufPivot;

    chnCount=conf->physChnCount;

    unsigned int cmSampleSize=conf->eegRate/5;

    // after you push one new sample’s cm feature into cmRMSBuf[...] and increment cmRMSBufIdx
    cmWarmupFilled_++; // count total samples accumulated into cmRMSBuf

    if (counter0%cmSampleSize==0) {
     // ======= warmup guard: wait until we have a full window =======
     if (cmWarmupFilled_<cmSampleSize) {
      // optional: publish a muted frame
      for (unsigned a=0;a<conf->ampCount;++a) for (unsigned c=0;c<chnCount;++c) tcpCM.cmLevel[a][c]=0;
       // push or skip as you wish:
       // tcpCMBuffer[tcpCMBufHead%tcpCMBufSize]=tcpCM; ++tcpCMBufHead;
       goto cm_done; // or `continue;`
     }
     // Compute and populate TcpCM
     for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) {
      auto& ampChn=tmpAmpChn_[ampIdx]; // length=chnCount
      auto& absdev=tmpAbsDev_[ampIdx]; // length=chnCount
      // 1) per-channel mean over last cmSampleSize values from your ring
      for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
       float acc=0.f;
       for (unsigned s=0;s<cmSampleSize;++s) {
        const unsigned ridx=(cmRMSBufIdx+cmRMSBufSize-s)%cmRMSBufSize;
        acc+=cmRMSBuf[ampIdx][chnIdx][ridx];
       }
       ampChn[chnIdx]=acc/float(cmSampleSize);
      }
      // 2) robust center & scale (median & MAD)
      std::vector<float> tmp=ampChn; // local copy for median
      const float med=median_inplace(tmp);
      for (unsigned i=0;i<chnCount;++i) absdev[i]=std::fabs(ampChn[i]-med);
      float mad=median_inplace(absdev);
      if (!std::isfinite(mad)) mad=0.f;
      const float robScale=1.4826f*mad+EPS;
      // 3) robust z+ → EMA, track Smax
      float Smax=0.f;
      for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
       const float zpos=std::max(0.f,(ampChn[chnIdx]-med)/robScale);
       const float S=CM_ALPHA*zpos+(1.f-CM_ALPHA)*Ssmooth_[ampIdx][chnIdx];
       Ssmooth_[ampIdx][chnIdx]=S;
       if (S>Smax) Smax=S;
      }
      // 3.5) Hysteresis update (NEW)
      for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
       float S=Ssmooth_[ampIdx][chnIdx];
       if (!isBad_[ampIdx][chnIdx] && S>BAD_ON) isBad_[ampIdx][chnIdx]=1;
       else if (isBad_[ampIdx][chnIdx] && S<BAD_OFF) isBad_[ampIdx][chnIdx]=0;
      }
      // 4) “all-good” cap: mute only if nobody stands out AND no channel is flagged bad
      bool anyBad=false;
      for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx)
       if (isBad_[ampIdx][chnIdx]) { anyBad=true; break; }
      const bool allGood=(Smax<0.5f) && !anyBad;
      // 5) map to 0..255 for transport (0=good/green, 255=bad/red)
      for (unsigned chnIdx=0;chnIdx<chnCount;++chnIdx) {
       float S=Ssmooth_[ampIdx][chnIdx];
       float Sprime=(S-S_FLOOR)/(S_HIGH-S_FLOOR);
       if (Sprime<0.f) Sprime=0.f;
       if (Sprime>1.f) Sprime=1.f;
       if (allGood) Sprime=0.f; // keep the map quiet when everyone is fine
       // optional: ensure flagged BAD doesn’t look “too green”
       // (gives a subtle visual latch; comment out if you prefer purely continuous color)
       if (isBad_[ampIdx][chnIdx] && Sprime<0.35f) Sprime=0.35f;
       tcpCM.cmLevel[ampIdx][chnIdx]=map01_to_u8(Sprime);
      }
     }

     tcpCMBuffer[tcpCMBufHead%tcpCMBufSize]=tcpCM; // Push to Circular Buffer
     tcpCMBufHead++;
    }
cm_done:
    emit sendData();

    msleep(conf->eegProbeMsecs);
    counter1++;
   } // EEG stream

   for (auto& e:eeAmps) { delete e.str; delete e.amp; } // EEG stream stops..
   qInfo("hnode_acqd: <acqthread> Exiting thread..");
  }

  bool popEEGSample(TcpSample *outSample) {
   if (tcpEEGBufTail<tcpEEGBufHead) {
    *outSample=tcpEEGBuffer[tcpEEGBufTail%tcpEEGBufSize]; tcpEEGBufTail++;
    return true;
   } else return false;
  }

  bool popCMArray(TcpCMArray *outArray) {
   if (tcpCMBufTail<tcpCMBufHead) {
    *outArray=tcpCMBuffer[tcpCMBufTail%tcpCMBufSize]; tcpCMBufTail++;
    return true;
   } else return false;
  }

  void sendTrigger(unsigned int t) {
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

 signals:
  void sendData();
 
 private:
  ConfParam *conf;

  TcpSample tcpEEG; TcpCMArray tcpCM; Sample smp;
  QVector<TcpSample> tcpEEGBuffer; QVector<TcpCMArray> tcpCMBuffer;
  int tcpEEGBufSize,tcpCMBufSize; quint64 tcpEEGBufHead,tcpEEGBufTail,tcpCMBufHead,tcpCMBufTail;

  QVector<QVector<QVector<float> > > cmRMSBuf; quint64 cmRMSBufIdx; unsigned int cmRMSBufSize;

  std::vector<EEAmp> eeAmps; unsigned int cBufSz,smpCount,chnCount; quint64 cBufPivotPrev,cBufPivot; std::vector<unsigned int> cBufPivotList;

  SerialDevice serDev;

  unsigned int syncTrig; QVector<unsigned int> arrivedTrig; // Trigger offsets for syncronization.
  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  quint64 counter0,counter1; unsigned int nonHWTrig;

  AudioAmp audioAmp;

  // CM state (per-amp, per-chn)
  uint64_t cmWarmupFilled_=0; // samples seen in cmRMSBuf
  std::vector<std::vector<float>> Ssmooth_;   // EMA of robust z+
  std::vector<std::vector<float>> tmpAmpChn_; // scratch: per-amp [chnCount]
  std::vector<std::vector<float>> tmpAbsDev_; // scratch: per-amp [chnCount]

  std::vector<std::vector<uint8_t>> isBad_; // hysteresis state: 0/1
};

#endif
