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

//#define EEMAGINE

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#endif

#include "../chninfo.h"
#include "../sample.h"
#include "../tcpsample.h"

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(QObject* parent,chninfo *ci,QVector<tcpsample> *tb,quint64 *pidx,quint64 *cidx,
            bool *r,bool *im,QMutex *m) : QThread(parent) {
   mutex=m; chnInfo=ci; tcpBuffer=tb; tcpBufPIdx=pidx; tcpBufCIdx=cidx; daemonRunning=r; eegImpedanceMode=im;

   convN=chnInfo->sampleRate/50; convN2=convN/2; buf0idx=buf1idx=convN; buf0idxP=buf1idxP=pivot0=pivot1=0;
   bufSize=chnInfo->probe_msecs*10;
   buf0.resize(bufSize); buf1.resize(bufSize); buf0F.resize(bufSize); buf1F.resize(bufSize);
   tcpS.marker1=M_PI; tcpS.marker2=M_E; tcpS.trigger=0;
  }

  void amplifiersInitialSetup() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   if (amplifiers.size()<2) {
    qDebug() << "octopus_acqd: <acqthread_amp_setup> Either of the amplifiers is offline!"; *daemonRunning=false; return;
   } eegBufs.resize(amplifiers.size());

   // Sort the two amplifiers in vector for their serial numbers
   std::vector<int> snos;
   for (quint64 i=0;i<amplifiers.size();i++)
    snos.push_back(stoi(amplifiers[i]->getSerialNumber()));
   if (snos[0]>snos[1]) {
    amp=amplifiers[1]; amplifiers[1]=amplifiers[0]; amplifiers[0]=amp;
   }

   // Select the channels to acquire afterwards (i.e. All 64 referentials and all 2/24 of bipolars)
   // and Initialize separate buffers for the two amps.
   for (quint64 i=0;i<amplifiers.size();i++) {
    amp=amplifiers[i];
    std::vector<channel> c=amp->getChannelList(0xffffffffffffffff,0x0000000000000003);
    chnLists.push_back(c);
   }
#endif
   switchToEEGMode();
  }

  void switchToImpedanceMode() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#endif
   ;
  }

  void switchToEEGMode() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   for (quint64 i=0;i<amplifiers.size();i++) {
    //std::cout << "Amp " << i << std::endl;
    amp=amplifiers[i];
    stream *s=amp->OpenEegStream(chnInfo->sampleRate,1.,4.,chnLists[i]);
    eegStreams.push_back(s);
   }
   *eegImpedanceMode=false;
   qDebug("octopus_acqd: <acqthread_switch2eeg> EEG upstream started..");
#else
   amp0offset=amp1offset=20; t0=t1=0.0; dt=0.001; frqA=10.0; frqB=48.0;
#endif
  }

  virtual void run() {
   int sc,cc,sc0,sc1;
   int tcpBufSize=tcpBuffer->size();

#ifdef EEMAGINE
   using namespace eemagine::sdk;
   factory fact("libeego-SDK.so"); amplifiers=fact.getAmplifiers();
   amplifiersInitialSetup();
#endif

   while (*daemonRunning) {
    if (!(*eegImpedanceMode)) {
#ifdef EEMAGINE
     for (quint64 i=0;i<eegStreams.size();i++) {
      try {
       eegBufs[i]=eegStreams[i]->getData();
       sc=eegBufs[i].getSampleCount(); cc=eegBufs[i].getChannelCount();
       //qDebug() << "Amp " << i+1 << ": " << sc << " samples," << cc << " channels.";
      } catch (const eemagine::sdk::exceptions::internalError& e) {
       std::cout << "Exception" << e.what() << std::endl;
      }
     }
     sc0=eegBufs[0].getSampleCount(); sc1=eegBufs[1].getSampleCount(); cc=eegBufs[0].getChannelCount();
#else
     sc=chnInfo->probe_msecs; sc0=sc1=sc; cc=68;
#endif
    
     buf0idxP=buf0idx; buf1idxP=buf1idx;

// ---------------------------------------------------------------------------------------------------
#ifdef EEMAGINE
     for (int i=0;i<sc0;i++) {
      for (int j=0;j<cc-2;j++) { sampleData=eegBufs[0].getSample(j,i); smp.data[j]=sampleData; }
      smp.trigger=eegBufs[0].getSample(cc-2,i); smp.offset=buf0chk=eegBufs[0].getSample(cc-1,i);
      buf0[(buf0idx+i)%bufSize]=smp;
      //if (i==sc0-1) buf0chk=smp.offset;
     }
#else
     for (int i=0;i<sc0;i++) {
      for (int j=0;j<cc-2;j++) { sampleData=1.0*cos(2.0*M_PI*frqA*t0)+0.5*sin(2.0*M_PI*frqB*t0); smp.data[j]=sampleData; }
      smp.trigger=0; smp.offset=amp0offset; t0+=dt; amp0offset++;
      buf0[(buf0idx+i)%bufSize]=smp;
     }
#endif
     for (int i=-convN2;i<sc0-convN2;i++) {
      for (int j=0;j<cc-2;j++) {
       sum=0.; for (int k=0;k<convN;k++) sum+=buf0[(buf0idx+i+k)%bufSize].data[j];
       buf0[(buf0idx+i+convN2)%bufSize].dataF[j]=sum/convN;
      }
     }
     buf0idx+=sc0;

#ifdef EEMAGINE
     for (int i=0;i<sc1;i++) {
      for (int j=0;j<cc-2;j++) { sampleData=eegBufs[1].getSample(j,i); smp.data[j]=sampleData; }
      smp.trigger=eegBufs[1].getSample(cc-2,i); smp.offset=buf1chk=eegBufs[1].getSample(cc-1,i);
      buf1[(buf1idx+i)%bufSize]=smp;
      //if (i==sc1-1) buf1chk=smp.offset;
     }
#else
     for (int i=0;i<sc1;i++) {
      for (int j=0;j<cc-2;j++) { sampleData=1.0*sin(2.0*M_PI*frqA*t1)+0.5*cos(2.0*M_PI*frqB*t1); smp.data[j]=sampleData; }
      smp.trigger=0; smp.offset=amp1offset; t1+=dt; amp1offset++;
      buf1[(buf1idx+i)%bufSize]=smp;
     }
#endif
     for (int i=-convN2;i<sc1-convN2;i++) {
      for (int j=0;j<cc-2;j++) {
       sum=0.; for (int k=0;k<convN;k++) sum+=buf1[(buf1idx+i+k)%bufSize].data[j];
       buf1[(buf1idx+i+convN2)%bufSize].dataF[j]=sum/convN;
      }
     }
     buf1idx+=sc1;

#ifdef EEMAGINE
     qDebug() << "CHK0: " << sc0 << " " << buf0chk << " -- CHK1:" << sc1 << " " <<buf1chk;
#endif
// ---------------------------------------------------------------------------------------------------

     if (buf0idxP<buf1idxP) { pivot0=buf0idxP; } else { pivot0=buf1idxP; }
     if (buf0idx<buf1idx)   { pivot1=buf0idx;  } else { pivot1=buf1idx;  }

#ifdef DEBUG
     qDebug() << "Indices (idx, Gidx Previous vs. actual)";
     qDebug() << "Buf0Idx P,N (Amp1): " << buf0idxP << " " << buf0idx << " -- (Amp 2):" << buf1idxP << " " << buf1idx;
     qDebug() << "Pivot: " << pivot0 << " " << pivot1;
     qDebug() << "---------";
#endif

     mutex->lock();
      quint64 tcpDataSize=pivot1-pivot0;
      for (quint64 i=0;i<tcpDataSize;i++) {
       tcpS.amp1=buf0[(pivot0+i-convN2)%bufSize]; tcpS.amp2=buf1[(pivot0+i-convN2)%bufSize];
       (*tcpBuffer)[(*tcpBufPIdx+i)%tcpBufSize]=tcpS;
      }
      (*tcpBufPIdx)+=tcpDataSize; // Update producer index
     mutex->unlock();
     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_msecs));
     //qDebug("octopus_acqd: <acqthread> Data validated to buffer..");
    } // eegImpedanceMode
   } // daemonRunning

   // Stop EEG Stream
   //mutex->lock(); mutex->unlock();
#ifdef EEMAGINE
   if (!(*eegImpedanceMode)) for (quint64 i=0;i<eegStreams.size();i++) delete eegStreams[i];
   for (quint64 i=0;i<amplifiers.size();i++) delete amplifiers[i];
#endif
   qDebug("octopus_acqd: <acqthread> Exiting thread..");
  }

 private:
  bool *daemonRunning,*eegImpedanceMode; QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPIdx,*tcpBufCIdx;
  chninfo *chnInfo; QMutex *mutex;
  
  sample smp; tcpsample tcpS;
  std::vector<sample> buf0,buf1,buf0F,buf1F;

  float sampleData,sum;

  quint64 buf0idx,buf1idx,buf0idxP,buf1idxP,pivot0,pivot1;
  int convN2,convN,buf0Idx,buf1Idx,bufSize;

#ifdef EEMAGINE
  eemagine::sdk::amplifier *amp; std::vector<eemagine::sdk::amplifier*> amplifiers;
  std::vector<std::vector<eemagine::sdk::channel> > chnLists;
  std::vector<eemagine::sdk::stream*> eegStreams;
  std::vector<eemagine::sdk::buffer> eegBufs;
  int buf0chk,buf1chk;
#else
   quint64 amp0offset,amp1offset; float frqA,frqB,t0,t1,dt;
#endif

};

#endif
