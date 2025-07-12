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
#include <termios.h>
#include <alsa/asoundlib.h>

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#else
#include "eesynth.h"
#endif

#include "../hacqglobals.h"
#include "../ampinfo.h"
#include "../serial_device.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "eex.h"

// Audio settings
const int AUDIO_SAMPLE_RATE=48000;
const int AUDIO_NUM_CHANNELS=2;
const int AUDIO_BUFFER_SIZE=4800;

const unsigned int CBUF_SIZE_IN_SECS=10;

const std::vector<float> b={0.06734199,0.          ,-0.13468398,0.         ,0.06734199};
const std::vector<float> a={1.        , -3.14315078, 3.69970174,-1.96971135,0.41316049};

class HAcqThread : public QThread {
 Q_OBJECT
 public:
  HAcqThread(AmpInfo *ai,int tbs,QMutex *m,QObject *parent=nullptr) : QThread(parent) {
   ampInfo=ai; tcpBufSize=tbs; tcpMutex=m; nonHWTrig=0;
   audioOK=false; audioBuffer.resize(AUDIO_BUFFER_SIZE*AUDIO_NUM_CHANNELS);

   smp=Sample(ampInfo->physChnCount);
   tcpS=TcpSample(ampInfo->ampCount,ampInfo->physChnCount);
   tcpBuffer=QVector<TcpSample>(tcpBufSize,TcpSample(ampInfo->ampCount,ampInfo->physChnCount));

   cBufPivotPrev=cBufPivot=tcpBufHead=tcpBufTail=counter0=counter1=0;
   convN2=(ampInfo->sampleRate/50)/2;
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
//    qInfo() << "octopus_hacqd: <AlsaAudioStream> Instreamed" << AUDIO_BUFFER_SIZE << "frames (48kHz, Stereo)";
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
   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData();
     chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=ampInfo->totalChnCount)
      qWarning() << "octopus_hacqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << ampInfo->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[i]=ee[i].cBufIdx;
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     //for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j); // Get Raw
     //for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].filterList[k].filterSample(ee[i].buf.getSample(k,j)); // IIR filtered
     for (unsigned int k=0;k<chnCount-2;k++) {
      float dummy=ee[i].buf.getSample(k,j);
      float dummyF=ee[i].filterList[k].filterSample(dummy);
      smp.data[k]=dummyF; // Non-filtered
      smp.dataF[k]=dummyF; // IIR filtered
      //if (i==0 && k==0) qDebug() << "fetchData: amp=" << i << " smpIdx=" << j << " ch=" << k << " val=" << dummy << dummyF;
     }
     smp.trigger=ee[i].buf.getSample(chnCount-2,j);
     if (j==0) ee[i].baseSmpIdx=ee[i].buf.getSample(chnCount-1,j); // -- The very 1st sample is set as EPOCH
     smp.offset=ee[i].smpIdx=ee[i].buf.getSample(chnCount-1,j)-ee[i].baseSmpIdx; // Sample# after Epoch
     //--> SYNC received for individual amps here in loop version
     ee[i].cBuf[(ee[i].cBufIdx+j)%cBufSz]=smp;
    }
    ee[i].cBufIdx+=ee[i].smpCount;
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
   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData();
     chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     if (chnCount!=ampInfo->totalChnCount)
      qWarning() << "octopus_hacqd: WARNING!!! <fetchEegData> Channel count mismatch!!!" << chnCount << ampInfo->totalChnCount;
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    cBufPivotList[i]=ee[i].cBufIdx;
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     //for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j); // Get Raw
     //for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].filterList[k].filterSample(ee[i].buf.getSample(k,j)); // IIR filtered
     for (unsigned int k=0;k<chnCount-2;k++) {
      float dummy=ee[i].buf.getSample(k,j);
      float dummyF=ee[i].filterList[k].filterSample(dummy);
      smp.data[k]=dummyF; // Non-filtered
      smp.dataF[k]=dummyF; // IIR filtered
      //if (i==0 && k==0) qDebug() << "fetchData: amp=" << i << " smpIdx=" << j << " ch=" << k << " val=" << dummy << dummyF;
     }
     smp.trigger=ee[i].buf.getSample(chnCount-2,j);
     //--> The very 1st sample is set as EPOCH in init version
     smp.offset=ee[i].smpIdx=ee[i].buf.getSample(chnCount-1,j)-ee[i].baseSmpIdx; // Sample# after Epoch
     // Receive SYNC for individual amps
     if (smp.trigger != 0) {
      if (smp.trigger==(unsigned int)(TRIG_AMPSYNC)) {
       qInfo("octopus_hacqd: <AmpSync> SYNC received by @AMP#%d -- %d",i+1,smp.offset);
       arrivedTrig[i]=smp.offset; syncTrig++; // arrived to one of amps, wait for the rest..
      } else { // Other trigger than SYNC
       qInfo("octopus_hacqd: <AmpSync> Trigger #%d arrived at AMP#%d -- @sampleoffset=%d",smp.trigger,i+1,smp.offset);
      }
     }
     ee[i].cBuf[(ee[i].cBufIdx+j)%cBufSz]=smp;
    } // Derive the minimum index among # of samples fetched via any amp (to use later in circular buffer updates)
    ee[i].cBufIdx+=ee[i].smpCount;
   }
   cBufPivotPrev=cBufPivot;
   cBufPivot=*std::min_element(cBufPivotList.begin(),cBufPivotList.end());
  }

  void run() override {
#ifdef EEMAGINE
   using namespace eemagine::sdk; factory eeFact("libeego-SDK.so");
#else
   using namespace eesynth; factory eeFact(ampInfo->ampCount);
#endif

   // --- Initial setup ---

   initAlsa();

   std::vector<amplifier*> eeAmpsUnsorted,eeAmps; std::vector<unsigned int> sUnsorted,serialNos;
   eeAmpsUnsorted=eeFact.getAmplifiers();
   if (eeAmpsUnsorted.size()<ampInfo->ampCount) {
    qDebug() << "octopus_hacqd: FATAL ERROR!!! <acqthread_amp_setup> At least one  of the amplifiers is offline!";
    return;
   }

   // Sort amplifiers for their serial numbers
   for (amplifier* eeAmp:eeAmpsUnsorted) sUnsorted.push_back(stoi(eeAmp->getSerialNumber()));
   serialNos=sUnsorted; std::sort(sUnsorted.begin(),sUnsorted.end());
   for (unsigned int i=0;i<serialNos.size();i++) for (unsigned int j=0;j<sUnsorted.size();j++)
    if (serialNos[i]==sUnsorted[j]) eeAmps.push_back(eeAmpsUnsorted[j]);

   // Construct main EE structure
   quint64 rMask,bMask; // Create channel selection masks -- subsequent channels assumed..
   rMask=0; for (unsigned int i=0;i<ampInfo->refChnCount;i++) { rMask<<=1; rMask|=1; }
   bMask=0; for (unsigned int i=0;i<ampInfo->bipChnCount;i++) { bMask<<=1; bMask|=1; }
   //qInfo("octopus_hacqd: <acqthread_amp_setup> RBMask: %llx %llx",rMask,bMask);
   for (unsigned int i=0;i<eeAmps.size();i++) {
    cBufSz=ampInfo->sampleRate*CBUF_SIZE_IN_SECS;
    EEx e(i,eeAmps[i],rMask,bMask,cBufSz,cBufSz/2,ampInfo->physChnCount,b,a); // cBufSz/2+convN
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    ee.push_back(e);
    arrivedTrig.push_back(0); // Construct trigger counts with "# of triggers=0" for each amp.
   }
   cBufPivotList.resize(ee.size());
   syncTrig=0;

   // ----- List unsorted vs. sorted
   for (unsigned int i=0;i<ee.size();i++)
    qInfo("octopus_hacqd: <AmpSerial> Amp#%d: %d",i+1,stoi(ee[i].amp->getSerialNumber()));

   chkTrig.resize(ee.size()); chkTrigOffset=0;

#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   // Initialize EEG streams of amps.
   for (EEx& e:ee) e.str=e.amp->OpenEegStream(ampInfo->sampleRate,ampInfo->refGain,ampInfo->bipGain,e.chnList);
   qInfo() << "octopus_hacqd: <acqthread_switch2eeg> EEG upstream started..";

   // EEG stream
   // -----------------------------------------------------------------------------------------------------------------
   fetchEegData0(); // The first round of acquisition - to preadjust certain things

   // Send SYNC
   sendTrigger(TRIG_AMPSYNC);
   qInfo() << "octopus_hacqd: <AmpSync> SYNC sent..";

   while (true) {
#ifndef EEMAGINE
    if (nonHWTrig==TRIG_AMPSYNC) { nonHWTrig=0; // Manual TRIG_AMPSYNC put within fetchEegData0() ?
     for (EEx& e:ee) (e.amp)->setTrigger(TRIG_AMPSYNC); // We set it here for all amps
							// It will be received later for inter-amp syncronization
    }
#endif
    fetchEegData();

    // If all amps have received the SYNC trigger already, align their buffers according to the trigger instant
    if (ee.size()>1 && syncTrig==ee.size()) {
     qInfo() << "octopus_hacqd: <AmpSync> SYNC received by all amps.. validating offsets.. ";
     unsigned int trigOffsetMin=*std::min_element(arrivedTrig.begin(),arrivedTrig.end());
     // The following table determines the continuous offset additions for each amp to be aligned.
     for (unsigned int i=0;i<ee.size();i++) arrivedTrig[i]-=trigOffsetMin;
     unsigned int trigOffsetMax=*std::max_element(arrivedTrig.begin(),arrivedTrig.end());
     if (trigOffsetMax>ampInfo->sampleRate) {
      qDebug() << "octopus_hacqd: ERROR!!! <AmpSync> SYNC is not recvd within a second for at least one amp!";
     } else {
      qInfo() << "octopus_hacqd: <AmpSync> SYNC retro-adjustments (per-amp alignment sample offsets) to be made:";
      for (int ii=0;ii<arrivedTrig.size();ii++) qDebug(" AMP#%d -> %d",ii+1,arrivedTrig[ii]);
      qInfo() << "octopus_hacqd: <AmpSync> SUCCESS. Offsets are now being synced on-the-fly to the earliest amp at TCPsample package level.";
     }
     syncTrig=0; // Ready for future SYNCing to update arrivedTrig[i] values
    }

    instreamAudio();

    tcpMutex->lock();
    quint64 tcpDataSize=cBufPivot-cBufPivotPrev;
    //qInfo() << cBufPivotPrev << cBufPivot;
    for (quint64 i=0;i<tcpDataSize;i++) {
     // Alignment of each amplifier to the latest offset by +arrivedTrig[i]
     for (unsigned int ii=0;ii<ee.size();ii++) {
//          qDebug() << "daemon pre-serialize: amp=" << ii
//         << " raw=" << ee[ii].cBuf[(cBufPivotPrev+i-convN2+arrivedTrig[ii])%cBufSz].data[0]
//         << " flt=" << ee[ii].cBuf[(cBufPivotPrev+i-convN2+arrivedTrig[ii])%cBufSz].dataF[0];
      tcpS.amp[ii]=ee[ii].cBuf[(cBufPivotPrev+i-convN2+arrivedTrig[ii])%cBufSz];
      //if (ii==0) qDebug() << "daemon: filled tcpS amp=" << ii << " ch=0 - val=" << tcpS.amp[ii].data[0] << tcpS.amp[ii].dataF[0];
     }
     tcpS.trigger=0;
     if (nonHWTrig) { tcpS.trigger=nonHWTrig; nonHWTrig=0; } // Set NonHW trigger in hyperEEG data struct

     // Receive current triggers from all amps.. They are supposed to be the same in value.
     for (unsigned int ii=0;ii<ee.size();ii++) chkTrig[ii]=tcpS.amp[ii].trigger;
     bool syncFlag=true; for (unsigned int ii=0;ii<ee.size();ii++) if (chkTrig[ii]!=chkTrig[0]) { syncFlag=false; break; }
     if (syncFlag) {
      if (chkTrig[0]!=0) {
       qInfo() << "octopus_hacqd: <AmpSync> Yay!!! Syncronized triggers received!";
       chkTrigOffset++;
      }
     } else {
      qDebug() << "octopus_hacqd: ERROR!!! <AmpSync> That's bad. Single offset lag..:" << syncFlag;
      for (unsigned int ii=0;ii<ee.size();ii++) qDebug() << "Trig" << ii << ":" << chkTrig[ii];
      qDebug() << "-> Offset:" << chkTrigOffset;
      chkTrigOffset=0;
     }

     // Copy Audio L and Audio R in tcpS from Audio Circular Buffer

     counter0++;

     tcpBuffer[(tcpBufHead+i)%tcpBufSize]=tcpS; // Push to Circular Buffer
     //filterAndPushToBuffer(tcpS,b,a,tcpBuffer,tcpBufHead,i,tcpBufSize);
    }
    tcpBufHead+=tcpDataSize; // Update producer index
    cBufPivotPrev=cBufPivot;
    tcpMutex->unlock();
    emit sendData();

    msleep(ampInfo->eegProbeMsecs);
    //usleep(ampInfo->probe_eeg_msecs*1000);
    //std::this_thread::sleep_for(std::chrono::milliseconds(ampInfo->probe_eeg_msecs));
    counter1++;

   } // EEG stream
   // -----------------------------------------------------------------------------------------------------------------

   for (EEx& e:ee) { delete e.str; delete e.amp; } // EEG stream stops..

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
    nonHWTrig=t; // The same for eesynth.h -- includes TRIG_AMPSYNC
 #endif
   } else { // Non-hardware trigger
    nonHWTrig=t; // Put manually to trigger part in TCPBuffer by within thread loop
   }
  }

 signals:
  void sendData();
 
 private:
#ifdef EEMAGINE
  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
#else
  std::vector<eesynth::amplifier*> eeAmpsU;
#endif

  TcpSample tcpS;
  Sample smp;
  QVector<TcpSample> tcpBuffer;
  int tcpBufSize;
  quint64 tcpBufHead,tcpBufTail;

  std::vector<EEx> ee;
  AmpInfo *ampInfo;
  unsigned int cBufSz,smpCount,chnCount;
  quint64 cBufPivotPrev,cBufPivot;
  std::vector<unsigned int> cBufPivotList;

  int convN2;

  QMutex *tcpMutex;

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
  unsigned int nonHWTrig;
};

#endif
