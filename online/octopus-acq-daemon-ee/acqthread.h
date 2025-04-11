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

#include "../chninfo.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "eex.h"

#include "acqdaemon.h"

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

class HighPassFilter {
 private:
  std::array<double,3> b={ 0.9956,-1.9911, 0.9956};
  std::array<double,3> a={ 1.0   ,-1.9911, 0.9912};
  //std::array<double,3> b; // Numerator coefficients
  //std::array<double,3> a; // Denominator coefficients
  std::array<double,2> xh={0,0}; // Input history
  std::array<double,2> yh={0,0}; // Output history
 public:
  //HighPassFilter(const std::array<double,3>& b_coeffs,const std::array<double,3>& a_coeffs):b(b_coeffs),a(a_coeffs) {}
  HighPassFilter() {}
  double process(double x) { // Process single sample
   double y=(b[0]*x+b[1]*xh[0]+b[2]*xh[1])-(a[1]*yh[0]+a[2]*yh[1]);
   // Update history
   xh[1]=xh[0]; xh[0]=x;
   yh[1]=yh[0]; yh[0]=y;
   return y;
  }
  void processBuffer(std::vector<double>& signal) { // Process chunk - forward filtering
   for (size_t i=0;i<signal.size();i++) { signal[i]=process(signal[i]); }
  }
  void processFiltFilt(std::vector<double>& signal,eex& e,int chn) {
   for (unsigned int i=0;i<2;i++) { xh[i]=e.fX[chn][i]; yh[i]=e.fX[chn][i]; }
   processBuffer(signal); // Forward
   std::reverse(signal.begin(),signal.end()); processBuffer(signal); // Reverse
   std::reverse(signal.begin(),signal.end());
   for (unsigned int i=0;i<2;i++) { e.fX[chn][i]=xh[i]; e.fX[chn][i]=yh[i]; }
  }
};

class BandPassFilter {
 private:
  std::array<double,5> b={ 0.0127, 0.0   ,-0.0255, 0.0   , 0.0127};
  std::array<double,5> a={ 1.0000,-3.6533, 5.0141,-3.0680, 0.7071};
  //std::array<double,5> b; // Numerator coefficients
  //std::array<double,5> a; // Denominator coefficients
  std::array<double,4> xh={0,0,0,0}; // Input history
  std::array<double,4> yh={0,0,0,0}; // Output history
 public:
  //BandPassFilter(const std::array<double,5>& b_coeffs,const std::array<double,5>& a_coeffs):b(b_coeffs),a(a_coeffs) {}
  BandPassFilter() {}
  double process(double x) { // Process single sample
   double y=(b[0]*x+b[1]*xh[0]+b[2]*xh[1]+b[3]*xh[2]+b[4]*xh[3])-(a[1]*yh[0]+a[2]*yh[1]+a[3]*yh[2]+a[4]*yh[3]);
   // Update history
   xh[3]=xh[2]; xh[2]=xh[1]; xh[1]=xh[0]; xh[0]=x;
   yh[3]=yh[2]; yh[2]=yh[1]; yh[1]=yh[0]; yh[0]=y;
   return y;
  }
  void processBuffer(std::vector<double>& signal) { // Process chunk - forward filtering
   for (size_t i=0;i<signal.size();i++) { signal[i]=process(signal[i]); }
  }
  void processFiltFilt(std::vector<double>& signal,eex& e,int chn) {
   //if (e.idx==0 && chn==0) { for (unsigned int i=0;i<signal.size();i++) printf("%f ",signal[i]); printf("\n"); }
   if (e.idx==0 && chn==0) {
    for (unsigned int i=0;i<e.fX[0].size();i++) printf("%f ",e.fX[chn][i]);
    for (unsigned int i=0;i<e.fY[0].size();i++) printf("%f ",e.fY[chn][i]);
    printf("\n");
   }
   for (unsigned int i=0;i<4;i++) { xh[i]=e.fX[chn][i]; yh[i]=e.fY[chn][i]; }
   processBuffer(signal); // Forward
   //std::reverse(signal.begin(),signal.end()); processBuffer(signal); // Reverse
   //std::reverse(signal.begin(),signal.end());
   for (unsigned int i=0;i<4;i++) { e.fX[chn][i]=xh[i]; e.fY[chn][i]=yh[i]; }
  }
};

class AcqThread : public QThread {
 Q_OBJECT
 public:
  //AcqThread(QObject* parent,chninfo *ci,QVector<tcpsample> *tb,quint64 *pidx,
  //          bool *r,bool *im,QMutex *tcpm,QMutex *guim,unsigned int *et) : QThread(parent) {
  AcqThread(AcqDaemon *acqd,QObject *parent=0) : QThread(parent) {
   acqD=acqd; chnInfo=&(acqD->chnInfo);
   tcpBuffer=&(acqD->tcpBuffer); tcpBufPivot=&(acqD->tcpBufPIdx);
   daemonRunning=&(acqD->daemonRunning); eegImpedanceMode=&(acqD->eegImpedanceMode);
   tcpMutex=&(acqD->tcpMutex); guiMutex=&(acqD->guiMutex);
   extTrig=&(acqD->extTrig);
   tcpS.trigger=0;
   convN=chnInfo->sampleRate/50; convN2=convN/2;
   convL=4*chnInfo->sampleRate; // 4 seconds MA for high pass
   cmL=chnInfo->sampleRate/2; // 0.5s

   filterIIR_1_40=false;

   toff=0;

   //CircularBuffer circBuffer(CIRCULAR_BUFFER_SIZE);
   audioOK=false; audioBuffer.resize(AUDIO_BUFFER_SIZE*AUDIO_NUM_CHANNELS);

   counter0=counter1=0;
   acqD->registerSendTriggerHandler(this);
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
   for (eex& e:ee) e.str=e.amp->OpenEegStream(chnInfo->sampleRate,EE_REF_GAIN,EE_BIP_GAIN,e.chnList);
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
   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData(); chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=chnInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!";
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

   sendTrigger(AMP_SYNC_TRIG);
   qDebug() << "octopus_acqd: <AmpSync> SYNC sent..";
  }

  void fetchEegData() { float sum0,sum1,data0;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   std::vector<double> v;

   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData(); chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=chnInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!";
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j);
     smp.trigger=ee[i].buf.getSample(chnCount-2,j);
     smp.offset=ee[i].smpIdx=ee[i].buf.getSample(chnCount-1,j)-ee[i].baseSmpIdx; // Sample# after Epoch
     if (smp.trigger != 0) {
      if (smp.trigger == (unsigned int)(AMP_SYNC_TRIG)) {
       qDebug() << "octopus_acqd: <AmpSync> SYNC received by @AMP#" << i+1 << " -- " << smp.offset;
       arrivedTrig[i]=smp.offset; syncTrig++;
      } else {
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
      bpf.processFiltFilt(v,ee[i],j);
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
    cBufSz=chnInfo->sampleRate*CBUF_SIZE_IN_SECS; e.cBufIdx=cBufSz/2; //+convN;
    e.cBuf.resize(cBufSz); e.cBufF.resize(cBufSz); e.imps.resize(chnInfo->physChnCount);
    e.fX.resize(chnInfo->physChnCount); for (unsigned int i=0;i<e.fX.size();i++) for (unsigned int j=0;j<e.fX[i].size();j++) e.fX[i][j]=0.;
    e.fY.resize(chnInfo->physChnCount); for (unsigned int i=0;i<e.fY.size();i++) for (unsigned int j=0;j<e.fY[i].size();j++) e.fY[i][j]=0.;
    ee.push_back(e);
    arrivedTrig.push_back(0); // Zero trigger for each amp.
   }
   cBufIdxList.resize(ee.size()); syncTrig=0;

   // ----- List unsorted vs. sorted
   for (unsigned int i=0;i<ee.size();i++) qDebug() << "octopus_acqd: <AmpSerial> Amp#" << i+1 << ":" << stoi(ee[i].amp->getSerialNumber());

   switchToEEGMode();

   // Main Loop

   if (!(*eegImpedanceMode)) fetchEegData0(); // The first round of acquisition - to preadjust certain things
   while (*daemonRunning) {
    if (*eegImpedanceMode) {
     fetchImpedanceData();
     for (eex& e:ee) for (unsigned int j=0;j<e.chnList.size();j++) e.imps[j]=e.buf.getSample(j,0);
     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_cm_msecs));
    } else {
     fetchEegData();

     // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
     if (syncTrig==ee.size()) {
      qDebug() << "octopus_acqd: <AmpSync> SYNC received by all amps.. validating offsets.. ";
      unsigned int trigOffsetMin=*std::min_element(arrivedTrig.begin(),arrivedTrig.end());
      for (unsigned int i=0;i<ee.size();i++) arrivedTrig[i]-=trigOffsetMin;
      unsigned int trigOffsetMax=*std::max_element(arrivedTrig.begin(),arrivedTrig.end());
      if (trigOffsetMax>chnInfo->sampleRate) {
       qDebug() << "octopus_acqd: <AmpSync> ERROR! SYNC is not recvd within a second for at least one amp!";
      } else {
       qDebug() << "octopus_acqd: <AmpSync> SYNC retro-adjustments to be made: AMP#1->" << arrivedTrig[0] << " AMP#2->" << arrivedTrig[1];
       qDebug() << "octopus_acqd: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
      }
      syncTrig=0; // Ready for future SYNCing to update arrivedTrig[i] values
     }

     instreamAudio();

     tcpMutex->lock();
      quint64 tcpDataSize=cBufPivot-cBufPivotP; // qDebug() << cBufPivotP << " " << cBufPivot;
      for (quint64 i=0;i<tcpDataSize;i++) {
       // Baseline alignment to the latest offset by (+arrivedTrig[i])
       tcpS.amp[0]=ee[0].cBuf[(cBufPivotP+i-convN2+arrivedTrig[0])%cBufSz];
       tcpS.amp[1]=ee[1].cBuf[(cBufPivotP+i-convN2+arrivedTrig[1])%cBufSz];

       // Trigger timing check in between amps
       trig0=tcpS.amp[0].trigger; trig1=tcpS.amp[1].trigger; toff++;
       if (trig0!=0 && trig1!=0) qDebug() << "octopus_acqd: <AmpSync> Yay! Syncronized triggers received!";
       else if (trig0!=0 || trig1!=0) {
        qDebug() << "octopus_acqd: <AmpSync> That's bad. Single offset lag.." << trig0 << "vs." << trig1 << "-> Offset:" << toff; toff=0;
       }

       // Copy Audio L and Audio R in tcpS from Audio Circular Buffer

       // Update cmLevels
       if ((counter0%(chnInfo->probe_cm_msecs/chnInfo->probe_eeg_msecs)==0)) {
        for (int i=0;i<acqD->chnTopo.size();i++) {
         (acqD->chnTopo)[i].cmLevel[0]=1.0*1e5*(tcpS.amp[0].curCM[i])/cmL;
         (acqD->chnTopo)[i].cmLevel[1]=1.0*1e5*(tcpS.amp[1].curCM[i])/cmL;
        }
       }
       counter0++;

       if (*extTrig) { tcpS.trigger=*extTrig; *extTrig=0; }
       (*tcpBuffer)[(*tcpBufPivot+i)%tcpBufSize]=tcpS;
      }

      (*tcpBufPivot)+=tcpDataSize; // Update producer index
     tcpMutex->unlock();

     // Common Mode Level estimation for both amps; copy to dedicated buffer
     if ((counter1%(chnInfo->probe_cm_msecs/chnInfo->probe_eeg_msecs)==0)) {
      guiMutex->lock(); acqD->updateCMLevels(); guiMutex->unlock();
     }

     cBufPivotP=cBufPivot;

     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_eeg_msecs));
    } // eegImpedanceMode or not
    counter1++;
   } // daemonRunning

   // Stop EEG Stream
   //tcpMutex->lock(); tcpMutex->unlock();

   for (eex& e:ee) { delete e.str; delete e.amp; }

   qDebug("octopus_acqd: <acqthread> Exiting thread..");
  }

 public slots:
  void sendTrigger(unsigned char t) {
   unsigned char trig=t;
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
  }

 private:
  AcqDaemon *acqD;
#ifdef EEMAGINE
  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
#else
  std::vector<eesynth::amplifier*> eeAmpsU;
#endif
  QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPivot,*tcpBufCIdx; tcpsample tcpS; sample smp;
  std::vector<eex> ee; chninfo *chnInfo; unsigned int cBufSz,smpCount,chnCount;
  std::vector<unsigned int> cBufIdxList;

  int convN,convN2,convL,cmL; quint64 cBufPivot,cBufPivotP;

  QMutex *tcpMutex,*guiMutex; bool *daemonRunning,*eegImpedanceMode; unsigned int *extTrig;

  bool filterIIR_1_40;

  // Butterworth coefficients (replace with MATLAB-generated values)
  HighPassFilter hpf;
  BandPassFilter bpf;

  serial_device serial;
  int sDevice; // Actual serial dev..
  struct termios oldtio,newtio; // place for old & new serial port settings..

  QVector<unsigned int> arrivedTrig; // Trigger offsets for syncronization.
  unsigned int syncTrig;

  unsigned int trig0,trig1,toff;

  // Alsa Audio
  snd_pcm_t* audioPCMHandle;
  snd_pcm_hw_params_t* audioParams;
  std::vector<int16_t> audioBuffer;

  bool audioOK;

  quint64 counter0,counter1;
  //unsigned char cmVal;
};

#endif
