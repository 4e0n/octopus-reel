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

#ifndef HACQTHREAD_H
#define HACQTHREAD_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <alsa/asoundlib.h>

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#else
#include "eesynth.h"
#endif

#include "confparam.h"
#include "../hacqglobals.h"
#include "../serialdevice.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "eeamp.h"

#define DUMPSTREAM

// Audio settings
const int AUDIO_SAMPLE_RATE=48000;
const int AUDIO_NUM_CHANNELS=2;
const int AUDIO_BUFFER_SIZE=4800;

const unsigned int CBUF_SIZE_IN_SECS=10;

class HAcqThread : public QThread {
 Q_OBJECT
 public:
  HAcqThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c; nonHWTrig=0;

   serDev.init();

   audioBuffer.resize(AUDIO_BUFFER_SIZE*AUDIO_NUM_CHANNELS); audioOK=false;

   smp=Sample(conf->physChnCount); tcpBufSize=conf->tcpBufSize*conf->sampleRate;
   tcpS=TcpSample(conf->ampCount,conf->physChnCount);
   tcpBuffer=QVector<TcpSample>(tcpBufSize,TcpSample(conf->ampCount,conf->physChnCount));

   cBufPivotPrev=cBufPivot=tcpBufHead=tcpBufTail=counter0=counter1=0;
  }

  void initAlsa() {
   if (snd_pcm_open(&audioPCMHandle,"default",SND_PCM_STREAM_CAPTURE,0)==0) { // Open ALSA capture device
    // Configure hardware parameters
    snd_pcm_hw_params_alloca(&audioParams);
    snd_pcm_hw_params_any(audioPCMHandle,audioParams);
    snd_pcm_hw_params_set_access(audioPCMHandle,audioParams,SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audioPCMHandle,audioParams,SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(audioPCMHandle,audioParams,AUDIO_NUM_CHANNELS);
    snd_pcm_hw_params_set_rate(audioPCMHandle,audioParams,AUDIO_SAMPLE_RATE,0);
    snd_pcm_hw_params_set_buffer_size(audioPCMHandle,audioParams,AUDIO_BUFFER_SIZE);
    if (snd_pcm_hw_params(audioPCMHandle,audioParams)==0) audioOK=true;
    else qDebug() << "octopus_hacqd: ERROR!!! <AlsaAudioInit> while setting PCM parameters.";
   } else qDebug() << "octopus_hacqd: ERROR!!! <AlsaAudioInit> while opening PCM device.";
   if (audioOK) qInfo() << "octopus_hacqd: <AlsaAudioInit> Alsa Audio Input successfully activated, audio streaming started.";
  }

  void instreamAudio() { // Call regularly within main loopthread
   int err = snd_pcm_readi(audioPCMHandle,audioBuffer.data(),AUDIO_BUFFER_SIZE);
   if (err==-EPIPE) {
    qWarning() << "octopus_hacqd: WARNING!!! <AlsaAudioStream> BUFFER OVERRUN!!! Recovering...";
    snd_pcm_prepare(audioPCMHandle);
   } else if (err<0) {
    qWarning() << "octopus_hacqd: WARNING!!! <AlsaAudioStream> ERROR READING AUDIO! Err.No:" << snd_strerror(err);
   } else { ;
    //qInfo() << "octopus_hacqd: <AlsaAudioStream> Instreamed" << AUDIO_BUFFER_SIZE << "frames (48kHz, Stereo)";
   }
  }
   
  void stopAudio() {
   snd_pcm_drain(audioPCMHandle); // Clean up
   snd_pcm_close(audioPCMHandle);
   std::cout << "octopus_hacqd: <AlsaAudioStream> Instreaming stopped.\n";
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
      qWarning() << "octopus_hacqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    for (unsigned int smpIdx=0;smpIdx<ee.smpCount;smpIdx++) { smp.marker=M_PI;
     for (unsigned int chnIdx=0;chnIdx<chnCount-2;chnIdx++) {
      float dummy=ee.bpFilterList[chnIdx].filterSample(ee.buf.getSample(chnIdx,smpIdx));
      smp.data[chnIdx]=dummy; // BP filtered
      smp.dataF[chnIdx]=ee.nFilterList[chnIdx].filterSample(dummy); // BP+notch filtered
      //if (ampIdx==0 && chnIdx==0)
      // qDebug() << "fetchData: amp=" << ampIdx << " smpIdx=" << smpIdx << " ch=" << chnIdx << " val=" << dummy << dummyF;
     }
     smp.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     if (smpIdx==0) ee.baseSmpIdx=ee.buf.getSample(chnCount-1,smpIdx); // -- The very 1st sample is set as EPOCH
     smp.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx; // Sample# after Epoch
     //--> SYNC received for individual amps here in loop version
     ee.cBuf[(ee.cBufIdx+smpIdx)%cBufSz]=smp;
    }
    ee.cBufIdx+=ee.smpCount;
    ampIdx++;
   }
   cBufPivotPrev=cBufPivot;
   cBufPivot=*std::min_element(cBufPivotList.begin(),cBufPivotList.end());
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
      qWarning() << "octopus_hacqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << conf->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[ampIdx]=ee.cBufIdx;
    for (unsigned int smpIdx=0;smpIdx<ee.smpCount;smpIdx++) { smp.marker=M_PI;
     for (unsigned int chnIdx=0;chnIdx<chnCount-2;chnIdx++) {
      float dummy=ee.bpFilterList[chnIdx].filterSample(ee.buf.getSample(chnIdx,smpIdx));
      smp.data[chnIdx]=dummy; // BP filtered
      smp.dataF[chnIdx]=ee.nFilterList[chnIdx].filterSample(dummy); // BP+notch filtered
      //if (ampIdx==0 && chnIdx==0) qDebug() << "fetchData: amp=" << ampIdx << " smpIdx=" << smpIdx << " ch=" << chnIdx << " val=" << dummy << dummyF;
     }
     smp.trigger=ee.buf.getSample(chnCount-2,smpIdx);
     //--> The very 1st sample is set as EPOCH in init version
     smp.offset=ee.smpIdx=ee.buf.getSample(chnCount-1,smpIdx)-ee.baseSmpIdx; // Sample# after Epoch
     // Receive SYNC for individual amps
     if (smp.trigger != 0) {
      if (smp.trigger==(unsigned int)(TRIG_AMPSYNC)) {
       qInfo("octopus_hacqd: <AmpSync> SYNC received by @AMP#%d -- %d",(int)ampIdx+1,smp.offset);
       arrivedTrig[ampIdx]=smp.offset; syncTrig++; // arrived to one of amps, wait for the rest..
      } else { // Other trigger than SYNC
       qInfo("octopus_hacqd: <AmpSync> Trigger #%d arrived at AMP#%d -- @sampleoffset=%d",smp.trigger,(int)ampIdx+1,smp.offset);
      }
     }
     ee.cBuf[(ee.cBufIdx+smpIdx)%cBufSz]=smp;
    } // Derive the minimum index among # of samples fetched via any amp (to use later in circular buffer updates)
    ee.cBufIdx+=ee.smpCount;
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

   initAlsa();

   std::vector<amplifier*> eeAmpsUnsorted,eeAmpsSorted; std::vector<unsigned int> sUnsorted,serialNos;
   eeAmpsUnsorted=eeFact.getAmplifiers();
   if (eeAmpsUnsorted.size()<conf->ampCount) {
    qDebug() << "octopus_hacqd: FATAL ERROR!!! <acqthread_amp_setup> At least one  of the amplifiers is offline!";
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
   //qInfo("octopus_hacqd: <acqthread_amp_setup> RBMask: %llx %llx",rMask,bMask);
   std::size_t ampIdx=0;
   for (auto& ee:eeAmpsSorted) {
    cBufSz=conf->sampleRate*CBUF_SIZE_IN_SECS;
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
     qInfo("octopus_hacqd: <AmpSerial> Amp#%d: %d",(int)ampIdx+1,stoi(ee.amp->getSerialNumber()));
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
   for (auto& e:eeAmps) e.str=e.amp->OpenEegStream(conf->sampleRate,conf->refGain,conf->bipGain,e.chnList);
   qInfo() << "octopus_hacqd: <acqthread_switch2eeg> EEG upstream started..";

   // EEG stream
   // -----------------------------------------------------------------------------------------------------------------
   fetchEegData0(); // The first round of acquisition - to preadjust certain things

   // Send SYNC
   sendTrigger(TRIG_AMPSYNC);
   qInfo() << "octopus_hacqd: <AmpSync> SYNC sent..";

#ifdef DUMPSTREAM
   QFile file("eegdump.txt");
   file.open(QIODevice::Append|QIODevice::Text); QTextStream outStream(&file);
#endif

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
     qInfo() << "octopus_hacqd: <AmpSync> SYNC received by all amps.. validating offsets.. ";
     unsigned int trigOffsetMin=*std::min_element(arrivedTrig.begin(),arrivedTrig.end());
     // The following table determines the continuous offset additions for each amp to be aligned.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) arrivedTrig[ampIdx]-=trigOffsetMin;
     unsigned int trigOffsetMax=*std::max_element(arrivedTrig.begin(),arrivedTrig.end());
     if (trigOffsetMax>conf->sampleRate) {
      qDebug() << "octopus_hacqd: ERROR!!! <AmpSync> SYNC is not recvd within a second for at least one amp!";
     } else {
      qInfo() << "octopus_hacqd: <AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
      for (int trigIdx=0;trigIdx<arrivedTrig.size();trigIdx++) qDebug(" AMP#%d -> %d",trigIdx+1,arrivedTrig[trigIdx]);
      qInfo() << "octopus_hacqd: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
     }
     syncTrig=0; // Ready for future SYNCing to update arrivedTrig[i] values
    }

    instreamAudio();

    //tcpMutex.lock();
    quint64 tcpDataSize=cBufPivot-cBufPivotPrev; //qInfo() << cBufPivotPrev << cBufPivot;
    for (quint64 tcpDataIdx=0;tcpDataIdx<tcpDataSize;tcpDataIdx++) {
     // Alignment of each amplifier to the latest offset by +arrivedTrig[i]
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) {
      //qDebug() << "daemon pre-serialize: amp=" << ampIdx
      //         << " raw=" << ee[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz].data[0]
      //         << " flt=" << ee[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz].dataF[0];
      tcpS.amp[ampIdx]=eeAmps[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz];
      //if (ampIdx==0) qDebug() << "daemon: filled tcpS amp=" << ampIdx << " ch=0 - val=" << tcpS.amp[ampIdx].data[0] << tcpS.amp[ampIdx].dataF[0];
     }
     tcpS.trigger=0;
     if (nonHWTrig) { tcpS.trigger=nonHWTrig; nonHWTrig=0; } // Set NonHW trigger in hyperEEG data struct

     // Receive current triggers from all amps.. They are supposed to be the same in value.
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) chkTrig[ampIdx]=tcpS.amp[ampIdx].trigger;
     bool syncFlag=true;
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) if (chkTrig[ampIdx]!=chkTrig[0]) { syncFlag=false; break; }
     if (syncFlag) {
      if (chkTrig[0]!=0) {
       qInfo() << "octopus_hacqd: <AmpSync> Yay!!! Syncronized triggers received!";
       chkTrigOffset++;
      }
     } else {
      qDebug() << "octopus_hacqd: ERROR!!! <AmpSync> That's bad. Single offset lag..:" << syncFlag;
      for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) qDebug() << "Trig" << ampIdx << ":" << chkTrig[ampIdx];
      qDebug() << "-> Offset:" << chkTrigOffset;
      chkTrigOffset=0;
     }

     // Copy Audio L and Audio R in tcpS from Audio Circular Buffer

     counter0++;

     tcpBuffer[(tcpBufHead+tcpDataIdx)%tcpBufSize]=tcpS; // Push to Circular Buffer
#ifdef DUMPSTREAM
     chnCount=conf->physChnCount;
     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) {
      for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) outStream << QString::number(tcpS.amp[ampIdx].data[chnIdx]) << " ";
      //outStream << QString::number(tcpS.amp[ampIdx].data[chnIdx]);
      outStream << "\n";
     }
#endif
    }
    tcpBufHead+=tcpDataSize; // Update producer index
    cBufPivotPrev=cBufPivot;
    //tcpMutex.unlock();
    emit sendData();

    msleep(conf->eegProbeMsecs);
    // --Alternatives:
    //usleep(conf->eegProbeMsecs*1000);
    //std::this_thread::sleep_for(std::chrono::milliseconds(conf->eegProbeMsecs));
    counter1++;
   } // EEG stream
   // -----------------------------------------------------------------------------------------------------------------

   for (auto& e:eeAmps) { delete e.str; delete e.amp; } // EEG stream stops..

   qInfo("octopus_hacqd: <acqthread> Exiting thread..");
  }

  bool popSample(TcpSample *outSample) {
   if (tcpBufTail<tcpBufHead) {
    *outSample=tcpBuffer[tcpBufTail%tcpBufSize]; tcpBufTail++;
    return true;
   } else return false;
  }

  void sendTrigger(unsigned int t) {
   if (t<256) { // also including TRIG_AMPSYNC=255
#ifdef EEMAGINE
    unsigned char trig=(unsigned char)t;
    if ((serDev.open()>0) { serDev.write(trig); }
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
//#ifdef EEMAGINE
//  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
//#else
//  std::vector<eesynth::amplifier*> eeAmpsU;
//#endif

  ConfParam *conf;

  TcpSample tcpS;
  Sample smp;
  QVector<TcpSample> tcpBuffer;
  int tcpBufSize;
  quint64 tcpBufHead,tcpBufTail;

  std::vector<EEAmp> eeAmps;
  unsigned int cBufSz,smpCount,chnCount;
  quint64 cBufPivotPrev,cBufPivot;
  std::vector<unsigned int> cBufPivotList;

  SerialDevice serDev;

  QVector<unsigned int> arrivedTrig; // Trigger offsets for syncronization.
  unsigned int syncTrig;

  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  // Alsa Audio
  snd_pcm_t* audioPCMHandle;
  snd_pcm_hw_params_t* audioParams;
  std::vector<int16_t> audioBuffer;

  bool audioOK;

  quint64 counter0,counter1;
  unsigned int nonHWTrig;
  //QMutex tcpMutex;
};

#endif
