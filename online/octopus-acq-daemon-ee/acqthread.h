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
#include <QMutex>
#include <QtNetwork>
#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <stdio.h>

#include <alsa/asoundlib.h>
#include <vector>
#include <cstring>

#include "../acqglobals.h"
#include "../serial_device.h"

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#else
#include "eesynth.h"
#endif

#include "../sample.h"
#include "../tcpsample.h"
#include "../ampinfo.h"
#include "eex.h"

#include "acqdaemon.h"

#include "iirf.h"

// Audio settings
const int AUDIO_SAMPLE_RATE=48000;
const int AUDIO_NUM_CHANNELS=2;
const int AUDIO_BUFFER_SIZE=4800;

/*
const int CIRCULAR_BUFFER_SIZE=8192;

// Circular buffer structure
struct CircularBuffer {
    std::vector<int16_t> buffer;
    size_t head = 0, tail = 0, size;

    CircularBuffer(size_t s) : buffer(s), size(s) {}

    void push(const int16_t* data, size_t frames) {
        for (size_t i = 0; i < frames * NUM_CHANNELS; ++i) {
            buffer[head] = data[i];
            head = (head + 1) % size;
            if (head == tail) { // Overwrite oldest data
                tail = (tail + 1) % size;
            }
        }
    }
};
*/

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(AcqDaemon *acqd,QObject *parent=0) : QThread(parent) {
   acqD=acqd; ampInfo=&(acqD->ampInfo);
   tcpBuffer=&(acqD->tcpBuffer); tcpBufPivot=&(acqD->tcpBufPIdx);
   daemonRunning=&(acqD->daemonRunning); eegImpedanceMode=&(acqD->eegImpedanceMode);
   tcpMutex=&(acqD->tcpMutex); guiMutex=&(acqD->guiMutex);
   extTrig=&(acqD->extTrig);
   tcpS.trigger=0;

   convN=ampInfo->sampleRate/50; convN2=convN/2;
   convL=4*ampInfo->sampleRate; // 4 seconds MA for high pass
   cmL=ampInfo->sampleRate/2; // 0.5s

   filterIIR_1_40=false;

   //CircularBuffer circBuffer(CIRCULAR_BUFFER_SIZE);
   audioOK=false; audioBuffer.resize(AUDIO_BUFFER_SIZE*AUDIO_NUM_CHANNELS);

   counter0=counter1=synthTrigger=synthSyncTrigger=0;
   acqD->registerSendTriggerHandler(this);
   acqD->registerSendSynthTriggerHandler(this);
  }

  void initAlsa() {
   // Open ALSA capture device
   if (snd_pcm_open(&audioPCMHandle,"default",SND_PCM_STREAM_CAPTURE,0)==0) {
    // Configure hardware parameters
    snd_pcm_hw_params_alloca(&audioParams);
    snd_pcm_hw_params_any(audioPCMHandle,audioParams);
    snd_pcm_hw_params_set_access(audioPCMHandle,audioParams,SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audioPCMHandle,audioParams,SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(audioPCMHandle,audioParams,AUDIO_NUM_CHANNELS);
    snd_pcm_hw_params_set_rate(audioPCMHandle,audioParams,AUDIO_SAMPLE_RATE,0);
    snd_pcm_hw_params_set_buffer_size(audioPCMHandle,audioParams,AUDIO_BUFFER_SIZE);
    if (snd_pcm_hw_params(audioPCMHandle,audioParams)==0) audioOK=true;
    else qDebug() << "octopus_acqd: <AlsaAudioInit> Error setting PCM parameters.";
   } else qDebug() << "octopus_acqd: <AlsaAudioInit> Error opening PCM device.";

   if (audioOK) qDebug() << "octopus_acqd: <AlsaAudioInit> Alsa Audio Input successfully activated, audio streaming started.";
  }

  void instreamAudio() { // Call regularly within main loopthread
   int err = snd_pcm_readi(audioPCMHandle,audioBuffer.data(),AUDIO_BUFFER_SIZE);
   if (err==-EPIPE) {
    qDebug() << "octopus_acqd: <AlsaAudioStream> BUFFER OVERRUN!!! Recovering...";
    snd_pcm_prepare(audioPCMHandle);
   } else if (err<0) {
    qDebug() << "octopus_acqd: <AlsaAudioStream> ERROR READING AUDIO! Err.No:" << snd_strerror(err);
   } else {
	   ;
    //circBuffer.push(buffer, BUFFER_SIZE);
//    qDebug() << "octopus_acqd: <AlsaAudioStream> Instreamed" << AUDIO_BUFFER_SIZE << "frames (48kHz, Stereo)";
   }
  }
   
  void stopAudio() {
   // Cleanup
   snd_pcm_drain(audioPCMHandle);
   snd_pcm_close(audioPCMHandle);
   std::cout << "Instreaming stopped.\n";
  }

  // --------

  void switchToImpedanceMode() { // Will be checked for mutual exclusion
   *eegImpedanceMode=true;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   // Dispose previous streams of amps.
   for (eex& e:ee) delete e.str;
   // Initialize impedance streams of amps.
   for (eex& e:ee) e.str=e.amp->OpenImpedanceStream(e.chnList);
   qDebug("octopus_acqd: <acqthread_switch2impedance> Switched to Impedance Measurement mode..");
  }

  void switchToEEGMode() { // Will be checked for mutual exclusion
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   // Dispose previous streams of amps (if already in Impedance Measurement mode).
   if (*eegImpedanceMode) { for (eex& e:ee) delete e.str; *eegImpedanceMode=false; }
   // Initialize EEG streams of amps.
   for (eex& e:ee) e.str=e.amp->OpenEegStream(ampInfo->sampleRate,EE_REF_GAIN,EE_BIP_GAIN,e.chnList);
   qDebug("octopus_acqd: <acqthread_switch2eeg> EEG upstream started..");
  }

  void fetchImpedanceData() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   for (eex& e:ee) {
    try {
     e.buf=e.str->getData();
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception:" << ex.what() << std::endl;
    }
   }
  }

  void fetchEegData0() { float sum0,sum1,data0;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   std::vector<double> v;
   for (unsigned int i=0;i<EE_AMPCOUNT;i++) { // ee.size()
    try {
     ee[i].buf=ee[i].str->getData(); chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=ampInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!" << chnCount << ampInfo->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufIdxList[i]=ee[i].cBufIdx;
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j);
     smp.trigger=ee[i].buf.getSample(chnCount-2,j); // dummy - as no practical possibility for a trigger yet.
     // The very first sample is set as the Epoch
     if (j==0) ee[i].baseSmpIdx=ee[i].buf.getSample(chnCount-1,j);
     smp.offset=ee[i].smpIdx=ee[i].buf.getSample(chnCount-1,j)-ee[i].baseSmpIdx; // Sample# after Epoch
     ee[i].cBuf[(ee[i].cBufIdx+j)%cBufSz]=smp;
    }
    // MA sums -- first computation
    for (unsigned int j=0;j<chnCount-2;j++) {
     for (int k=-convN2;k<(int)(ee[i].smpCount)-convN2;k++) {
      sum0=0.; for (int m=-convN2;m<convN2;m++) sum0+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].data[j];
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].sum0[j]=sum0;
      data0=fabs(ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].data[j]-sum0/convN);
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].com0[j]=data0*data0;

      // Hi-pass
      sum1=0.; for (int m=k-convL;m<k;m++) sum1+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].data[j];
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].sum1[j]=sum1;

      ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].dataF[j]=sum0/convN-sum1/convL;
     }
     for (int k=-convN2;k<(int)(ee[i].smpCount)-convN2;k++) {
      // Current CM value
      sum1=0.; for (int m=k-cmL;m<k;m++) sum1+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].com0[j];
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].curCM[j]=sum1;
      //if (i==0 && j==0) qDebug() << sum1/cmL;
     }
    }
    ee[i].cBufIdx+=ee[i].smpCount;
   }
   cBufPivotP=cBufPivot; cBufPivot=*std::min_element(cBufIdxList.begin(),cBufIdxList.end());

   sendTrigger((unsigned int)(AMP_SYNC_TRIG));
   qDebug() << "octopus_acqd: <AmpSync> SYNC sent..";
  }

  void fetchEegData() { float sum0,sum1,data0;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   std::vector<double> v;

   for (unsigned int i=0;i<EE_AMPCOUNT;i++) { // ee.size()
    try {
     ee[i].buf=ee[i].str->getData(); chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=ampInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!";
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j);
     smp.trigger=ee[i].buf.getSample(chnCount-2,j);
     smp.offset=ee[i].smpIdx=ee[i].buf.getSample(chnCount-1,j)-ee[i].baseSmpIdx; // Sample# after Epoch
     if (smp.trigger != 0) {
      if (EE_AMPCOUNT>1 && smp.trigger==(unsigned int)(AMP_SYNC_TRIG)) {
       qDebug() << "octopus_acqd: <AmpSync> SYNC received by @AMP#" << i+1 << " -- " << smp.offset;
       arrivedTrig[i]=smp.offset; syncTrig++;
      } else { // For single amp the sync_trig will not exist at all, so no problem for falling back to this condition
       qDebug() << "octopus_acqd: <AmpSync> Trigger #" << smp.trigger << " arrived at AMP#" << i+1 << " -- " << smp.offset;
      }
     }
     ee[i].cBuf[(ee[i].cBufIdx+j)%cBufSz]=smp;
    }

    // ----- ONLINE FILTERING -----
    // Past average subtraction for High Pass + Moving Average for 50Hz and harmonics
    for (unsigned int j=0;j<chnCount-2;j++) {
     for (int k=-convN2;k<(int)(ee[i].smpCount)-convN2;k++) {
      sum0=0.; for (int m=-convN2;m<convN2;m++) sum0+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].data[j];
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].sum0[j]=sum0;
      data0=fabs(ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].data[j]-sum0/convN);
      ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].com0[j]=data0*data0;

      // Current CM value
      //sum0b=ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].sum0b[j];
      //data0=ee[i].cBuf[(ee[i].cBufIdx+k-cmL-1)%cBufSz].data[j]-ee[i].cBuf[(ee[i].cBufIdx+k-cmL-1)%cBufSz].sum0[j]/convN;
      //sum0b-=data0*data0;
      //data0=ee[i].cBuf[(ee[i].cBufIdx+k-1)%cBufSz].data[j]-sum0/convN;
      //sum0b+=data0*data0;
      //ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].sum0b[j]=sum0b;
      //if (i==0 && j==0) qDebug() << sum0b;
      //ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].curCM[j]=sqrt(sum0b)/cmL;

      // Hi-pass
      sum1=ee[i].cBuf[(ee[i].cBufIdx+k+convN2-1)%cBufSz].sum1[j];
      sum1-=ee[i].cBuf[(ee[i].cBufIdx+k-convL-1)%cBufSz].data[j];
      sum1+=ee[i].cBuf[(ee[i].cBufIdx+k-1)%cBufSz].data[j];
      ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].sum1[j]=sum1;

      ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].dataF[j]=sum0/convN-sum1/convL;
     }
     for (int k=-convN2;k<(int)(ee[i].smpCount)-convN2;k++) {
      // Current CM value
      sum1=ee[i].cBuf[(ee[i].cBufIdx+k+convN2-1)%cBufSz].curCM[j];
      sum1-=ee[i].cBuf[(ee[i].cBufIdx+k-cmL-1)%cBufSz].com0[j];
      sum1+=ee[i].cBuf[(ee[i].cBufIdx+k-1)%cBufSz].com0[j];
      ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].curCM[j]=sum1;
      //if (i==0 && j==0) qDebug() << sum1/cmL;
     }
    }
    if (filterIIR_1_40) { // Cascade to MA50Hz
     for (unsigned int j=0;j<chnCount-2;j++) {
      v.resize(0);
      for (int k=0;k<(int)(ee[i].smpCount);k++) v.push_back(ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].data[j]);
      bpf.bpf_processFiltFilt(v,ee[i],j);
      for (int k=0;k<(int)(ee[i].smpCount);k++) ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].dataF[j]=v[k];
     }
    }

    // Derive the minimum index among # of samples fetched via any amp (to use later in circular buffer updates)
    cBufIdxList[i]=ee[i].cBufIdx; ee[i].cBufIdx+=ee[i].smpCount;
   }

   // ---------------------

   cBufPivotP=cBufPivot; cBufPivot=*std::min_element(cBufIdxList.begin(),cBufIdxList.end());
  }

// ------------------------------------------------
// ------------------------------------------------
// ------------------------------------------------

  virtual void run() {
   int tcpBufSize=tcpBuffer->size();
#ifdef EEMAGINE
   using namespace eemagine::sdk; factory eeFact("libeego-SDK.so");
#else
   using namespace eesynth; factory eeFact;
#endif

   // --- Initial setup ---

   initAlsa();

   std::vector<amplifier*> eeAmpsU,eeAmps; std::vector<unsigned int> snosU,snos; // Unsorted vs. sorted
   eeAmpsU=eeFact.getAmplifiers();
   if (eeAmpsU.size()<EE_AMPCOUNT) {
    qDebug() << "octopus_acqd: <acqthread_amp_setup> At least one  of the amplifiers is offline!"; *daemonRunning=false; return;
   }

   // Sort amplifiers for their serial numbers
   for (amplifier* eeAmp:eeAmpsU) snosU.push_back(stoi(eeAmp->getSerialNumber()));
   snos=snosU; std::sort(snos.begin(),snos.end());
   for (unsigned int i=0;i<snos.size();i++) for (unsigned int j=0;j<snosU.size();j++)
    if (snos[i]==snosU[j]) eeAmps.push_back(eeAmpsU[j]);

   // Construct main EE structure
   quint64 rMask,bMask; // Create channel selection masks -- subsequent channels assumed..
   rMask=0; for (int i=0;i<REF_CHN_COUNT;i++) { rMask<<=1; rMask|=1; }
   bMask=0; for (int i=0;i<BIP_CHN_COUNT;i++) { bMask<<=1; bMask|=1; }
   //qDebug("%llx %llx",rMask,bMask);
   for (unsigned int i=0;i<eeAmps.size();i++) { eex e; e.idx=i; e.amp=eeAmps[i];
    e.chnList=e.amp->getChannelList(rMask,bMask);
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    //e.chnList=e.amp->getChannelList(0xffffffffffffffff,0x0000000000000003);
    //e.chnList=e.amp->getChannelList(0x0000000000000000,0x0000000000000001);
    cBufSz=ampInfo->sampleRate*CBUF_SIZE_IN_SECS; e.cBufIdx=cBufSz/2; //+convN;
    //e.cBufF.resize(cBufSz);
    e.cBuf.resize(cBufSz); e.imps.resize(ampInfo->physChnCount);
    e.fX.resize(ampInfo->physChnCount); for (unsigned int i=0;i<e.fX.size();i++) for (unsigned int j=0;j<e.fX[i].size();j++) e.fX[i][j]=0.;
    e.fY.resize(ampInfo->physChnCount); for (unsigned int i=0;i<e.fY.size();i++) for (unsigned int j=0;j<e.fY[i].size();j++) e.fY[i][j]=0.;
    ee.push_back(e);
    arrivedTrig.push_back(0); // Construct trigger counts with "# of triggers=0" for each amp.
   }
   cBufIdxList.resize(EE_AMPCOUNT); // ee.size()
   syncTrig=0;

   // ----- List unsorted vs. sorted
   for (unsigned int i=0;i<EE_AMPCOUNT;i++) // ee.size()
    qDebug() << "octopus_acqd: <AmpSerial> Amp#" << i+1 << ":" << stoi(ee[i].amp->getSerialNumber());

   chkTrig.resize(EE_AMPCOUNT); chkTrigOffset=0;

   switchToEEGMode();

   // Main Loop

   if (!(*eegImpedanceMode)) fetchEegData0(); // The first round of acquisition - to preadjust certain things
   while (*daemonRunning) {
    if (*eegImpedanceMode) {
     fetchImpedanceData();
     for (eex& e:ee) for (unsigned int j=0;j<e.chnList.size();j++) e.imps[j]=e.buf.getSample(j,0);
     std::this_thread::sleep_for(std::chrono::milliseconds(ampInfo->probe_cm_msecs));
    } else {
#ifndef EEMAGINE
     if (synthSyncTrigger) { synthSyncTrigger=0; for (eex& e:ee) (e.amp)->sendSynthTrigger((unsigned int)(AMP_SYNC_TRIG)); }
#endif
     fetchEegData();

     // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
     if (EE_AMPCOUNT>1 && syncTrig==EE_AMPCOUNT) { // ee.size()
      qDebug() << "octopus_acqd: <AmpSync> SYNC received by all amps.. validating offsets.. ";
      unsigned int trigOffsetMin=*std::min_element(arrivedTrig.begin(),arrivedTrig.end());
      // The following table determines the continuous offset additions for each amp to be aligned.
      for (unsigned int i=0;i<EE_AMPCOUNT;i++) arrivedTrig[i]-=trigOffsetMin; // ee.size()
      unsigned int trigOffsetMax=*std::max_element(arrivedTrig.begin(),arrivedTrig.end());
      if (trigOffsetMax>ampInfo->sampleRate) {
       qDebug() << "octopus_acqd: <AmpSync> ERROR! SYNC is not recvd within a second for at least one amp!";
      } else {
       qDebug() << "octopus_acqd: <AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
       for (int ii=0;ii<arrivedTrig.size();ii++) qDebug(" AMP#%d -> %d",ii+1,arrivedTrig[ii]);
       qDebug() << "octopus_acqd: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
      }
      syncTrig=0; // Ready for future SYNCing to update arrivedTrig[i] values
     }

     instreamAudio();

     tcpMutex->lock();
      quint64 tcpDataSize=cBufPivot-cBufPivotP; // qDebug() << cBufPivotP << " " << cBufPivot;
      for (quint64 i=0;i<tcpDataSize;i++) {
       // Alignment of each amplifier to the latest offset by +arrivedTrig[i]
       for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++)
        tcpS.amp[ii]=ee[ii].cBuf[(cBufPivotP+i-convN2+arrivedTrig[ii])%cBufSz];
       tcpS.trigger=0;
       if (synthTrigger) { tcpS.trigger=synthTrigger; synthTrigger=0; }


       // Trigger timing check in between amps
       if (EE_AMPCOUNT>1) {
        // Receive current triggers from all amps.. They are supposed to be the same in value.
        for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++) chkTrig[ii]=tcpS.amp[ii].trigger;
        bool syncFlag=true; for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++) if (chkTrig[ii]!=chkTrig[0]) { syncFlag=false; break; }
        if (syncFlag) {
         if (chkTrig[0]!=0) {
          qDebug() << "octopus_acqd: <AmpSync> Yay! Syncronized triggers received!";
          chkTrigOffset++;
	 }
	} else {
         qDebug() << "octopus_acqd: <AmpSync> That's bad. Single offset lag..:" << syncFlag;
         for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++) qDebug() << "Trig" << ii << ":" << chkTrig[ii];
         qDebug() << "-> Offset:" << chkTrigOffset;
         chkTrigOffset=0;
        }
       } else {
        qDebug() << "octopus_acqd: <AmpSync> Only a single amp, and it received the trigger! No big deal.";
       }

       // Copy Audio L and Audio R in tcpS from Audio Circular Buffer

       // Update cmLevels
       if ((counter0%(ampInfo->probe_cm_msecs/ampInfo->probe_eeg_msecs)==0)) {
        for (int i=0;i<acqD->chnInfo.size();i++) {
	 for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++)
          (acqD->chnInfo)[i].cmLevel[ii]=1.0*1e5*(tcpS.amp[ii].curCM[i])/cmL;
        }
       }
       counter0++;

       if (*extTrig) { tcpS.trigger=*extTrig; *extTrig=0; }
       (*tcpBuffer)[(*tcpBufPivot+i)%tcpBufSize]=tcpS;
      }

      (*tcpBufPivot)+=tcpDataSize; // Update producer index
     tcpMutex->unlock();

     // Common Mode Level estimation for both amps; copy to dedicated buffer
     if ((counter1%(ampInfo->probe_cm_msecs/ampInfo->probe_eeg_msecs)==0)) {
      guiMutex->lock(); acqD->updateCMLevels(); guiMutex->unlock();
     }

     cBufPivotP=cBufPivot;

     std::this_thread::sleep_for(std::chrono::milliseconds(ampInfo->probe_eeg_msecs));
    } // eegImpedanceMode or not
    counter1++;
   } // daemonRunning

   // Stop EEG Stream
   //tcpMutex->lock(); tcpMutex->unlock();

   for (eex& e:ee) { delete e.str; delete e.amp; }

   qDebug("octopus_acqd: <acqthread> Exiting thread..");
  }

 public slots:
  void sendTrigger(unsigned int t) {
#ifdef EEMAGINE
   unsigned char trig=(unsigned char)t;
   serial.devname="/dev/ttyACM0"; serial.baudrate=B115200;
   serial.databits=CS8; serial.parity=serial.par_on=0; serial.stopbit=1;
   if ((serial.device=open(serial.devname.toLatin1().data(),O_RDWR|O_NOCTTY))>0) {
    tcgetattr(serial.device,&oldtio);     // Save current port settings..
    bzero(&newtio,sizeof(newtio)); // Clear struct for new settings..

    // CLOCAL: Local connection, no modem control
    // CREAD:  Enable receiving chars
    newtio.c_cflag=serial.baudrate | serial.databits |
                   serial.parity   | serial.par_on   |
                   serial.stopbit  | CLOCAL |CREAD;

    newtio.c_iflag=IGNPAR | ICRNL;  // Ignore bytes with parity errors,
                                    // map CR to NL, as it will terminate the
                                    // input for canonical mode..

    newtio.c_oflag=0;               // Raw Output

    newtio.c_lflag=ICANON;          // Enable Canonical Input

    tcflush(serial.device,TCIFLUSH); // Clear line and activate settings..
    tcsetattr(serial.device,TCSANOW,&newtio); sleep(1);

    write(serial.device,&trig,sizeof(unsigned char));

    tcsetattr(serial.device,TCSANOW,&oldtio); // Restore old serial context..
    close(serial.device);
   }
 #else
   synthSyncTrigger=t; // AMP_SYNC_TRIG
 #endif
  }

  void sendSynthTrigger(unsigned int t) { synthTrigger=t; }

 private:
  AcqDaemon *acqD;
#ifdef EEMAGINE
  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
#else
  std::vector<eesynth::amplifier*> eeAmpsU;
#endif
  QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPivot,*tcpBufCIdx; tcpsample tcpS; sample smp;
  std::vector<eex> ee; AmpInfo *ampInfo; unsigned int cBufSz,smpCount,chnCount;
  std::vector<unsigned int> cBufIdxList;

  int convN,convN2,convL,cmL; quint64 cBufPivot,cBufPivotP;

  QMutex *tcpMutex,*guiMutex; bool *daemonRunning,*eegImpedanceMode; unsigned int *extTrig;

  bool filterIIR_1_40;

  // Butterworth coefficients (replace with MATLAB-generated values)
  IIRF hpf,bpf;

  serial_device serial;
  int sDevice; // Actual serial dev..
  struct termios oldtio,newtio; // place for old & new serial port settings..

  QVector<unsigned int> arrivedTrig; // Trigger offsets for syncronization.
  unsigned int syncTrig;

  QVector<unsigned int> chkTrig; unsigned int chkTrigOffset;

  // Alsa Audio
  snd_pcm_t* audioPCMHandle;
  snd_pcm_hw_params_t* audioParams;
  std::vector<int16_t> audioBuffer;

  bool audioOK;

  quint64 counter0,counter1;
  unsigned int synthTrigger,synthSyncTrigger;
  //unsigned char cmVal;
};

#endif
