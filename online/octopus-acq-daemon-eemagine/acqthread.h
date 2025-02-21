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

#define EEMAGINE

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#endif

const int EE_AMPCOUNT=2;
const int EE_REF_GAIN=1.;
const int EE_BIP_GAIN=4.;

#include "../chninfo.h"
#include "../sample.h"
#include "../tcpsample.h"

#include "eemulti.h"

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(QObject* parent,chninfo *ci,QVector<tcpsample> *tb,quint64 *pidx,quint64 *cidx,
            bool *r,bool *im,QMutex *m) : QThread(parent) {
   mutex=m; chnInfo=ci; tcpBuffer=tb; tcpBufPIdx=pidx; tcpBufCIdx=cidx; daemonRunning=r; eegImpedanceMode=im;

   convN=chnInfo->sampleRate/50; convN2=convN/2;
   tcpS.marker1=M_PI; tcpS.marker2=M_E; tcpS.trigger=0;
  }

  void switchToImpedanceMode() { // Will be checked for mutual exclusion
   *eegImpedanceMode=true;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   // Dispose previous streams of amps.
   for (eemulti& e:ee) delete e.stream;
   // Initialize impedance streams of amps.
   for (eemulti& e:ee) e.stream=e.amp->OpenImpedanceStream(e.chnList);
   qDebug("octopus_acqd: <acqthread_switch2impedance> Switched to Impedance Measurement mode..");
#endif
  }

  void switchToEEGMode() { // Will be checked for mutual exclusion
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   // Dispose previous streams of amps (if already in Impedance Measurement mode).
   if (*eegImpedanceMode) { for (eemulti& e:ee) delete e.stream; *eegImpedanceMode=false; }
   // Initialize EEG streams of amps.
   for (eemulti& e:ee) e.stream=e.amp->OpenEegStream(chnInfo->sampleRate,EE_REF_GAIN,EE_BIP_GAIN,e.chnList);
   qDebug("octopus_acqd: <acqthread_switch2eeg> EEG upstream started..");
#else
   amp0offset=amp1offset=20;
   dc0=dc1=0.0; // to simulate High-Pass
   ampl0=ampl1=0.000100; // 100uV mimicks EEG
   dt=1.0/(float)(chnInfo->sampleRate); frqA=10.0; frqB=48.0;
#endif
  }

  void getImpedanceData() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   for (eemulti& e:ee) {
    try {
     e.buffer=e.stream->getData();
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception:" << ex.what() << std::endl;
    }
   }
#endif
  }

  void getEegData() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
   for (eemulti& e:ee) {
    try {
     e.buffer=e.stream->getData();
     e.smpCount=e.buffer.getSampleCount(); chnCount=e.buffer.getChannelCount();
     if (chnCount!=chnInfo->totalChnCount) qDebug() << "octopus_acqd: <getEegData> Channel count mismatch!!!";
     //qDebug() << "Amp " << i+1 << ": " << smpCount << " samples," << chnCount << " channels.";
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    e.cBufIdxP=e.cBufIdx;
   }
#else
   for (eemulti& e:ee) e.smpCount=chnInfo->probe_eeg_msecs;
   chnCount=TOTAL_CHN_COUNT;
#endif
  }

// ------------------------------------------------

  virtual void run() {
   int tcpBufSize=tcpBuffer->size();

   // Initial setup

#ifdef EEMAGINE
   using namespace eemagine::sdk;
   factory eeFact("libeego-SDK.so");
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
   for (amplifier* eeAmp:eeAmps) { eemulti e; e.amp=eeAmp;
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    //e.chnList=e.amp->getChannelList(0xffffffffffffffff,0x0000000000000003);
    e.chnList=e.amp->getChannelList(0x0000000000000000,0x0000000000000001);
    e.cBufIdx=convN; e.cBufIdxP=0; cBufSz=chnInfo->probe_eeg_msecs*10;
    e.cBuf.resize(cBufSz); e.cBufF.resize(cBufSz); e.imps.resize(chnInfo->physChnCount);
    ee.push_back(e);
   }

   // ----- List unsorted vs. sorted
   //for (unsigned int i=0;i<ee.size();i++) qDebug() << stoi(ee[i].amp->getSerialNumber());

#else
   // Construct main EE structure
   for (int i=0;i<EE_AMPCOUNT;i++) { eemulti e;
    e.cBufIdx=convN; e.cBufIdxP=0; cBufSz=chnInfo->probe_eeg_msecs*10;
    e.cBuf.resize(cBufSz); e.cBufF.resize(cBufSz); e.imps.resize(chnInfo->physChnCount);
    e.dc=0; e.ampl=0.000100; e.t=0.0;
    ee.push_back(e);
   }
#endif
   switchToEEGMode();

   // Main Loop

   while (*daemonRunning) {
    if (*eegImpedanceMode) {

     getImpedanceData();
#ifdef EEMAGINE
     for (eemulti& e:ee) for (unsigned int j=0;j<e.chnList.size();j++) e.imps[j]=e.buffer.getSample(j,0);
#endif
     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_impedance_msecs));

    } else {

     getEegData();
    
// ---------------------------------------------------------------------------------------------------
#ifdef EEMAGINE
     for (eemulti& e:ee) for (unsigned int j=0;j<e.smpCount;j++) {
      for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=e.buffer.getSample(k,j);
      smp.trigger=e.buffer.getSample(chnCount-2,j);
      smp.offset=e.absSmpIdx=e.buffer.getSample(chnCount-1,j);
      e.cBuf[(e.cBufIdx+j)%cBufSz]=smp;
      //if (j==e.smpCount-1) e.absSmpIdx=smp.offset;
     }
#else
     for (eemulti& e:ee) for (unsigned int j=0;j<e.smpCount;j++) {
      for (unsigned int k=0;k<chnCount-2;k++)
       smp.data[k]=dc+ampl*cos(2.0*M_PI*frqA*e.t)+(ampl/1.0)*sin(2.0*M_PI*frqB*e.t);
      smp.trigger=0; smp.offset=e.absSmpIdx; e.t+=dt; e.absSmpIdx++;
      e.cBuf[(e.cBufIdx+j)%cBufSz]=smp;
     }
#endif
     // Filtering
     for (eemulti& e:ee) for (unsigned int j=0;j<chnCount-2;j++)
      for (int k=-convN2;k<(int)(e.smpCount)-convN2;k++) {
       sum=0.; for (int m=0;m<convN;m++) sum+=e.cBuf[(e.cBufIdx+k+m)%cBufSz].data[j];
       e.cBuf[(e.cBufIdx+k+convN2)%cBufSz].dataF[j]=sum/convN;
      }

     for (eemulti& e:ee) e.cBufIdx+=e.smpCount;

#ifdef EEMAGINE
     qDebug() << "Sample count vs. Checkpoint: SC0:" << ee[0].smpCount << " CHK0:" << ee[0].absSmpIdx;
     qDebug() << "                             SC1:" << ee[1].smpCount << " CHK1:" << ee[1].absSmpIdx;
#endif

     if (ee[0].cBufIdxP < ee[1].cBufIdxP) pivot0=ee[0].cBufIdxP; else pivot0=ee[1].cBufIdxP;
     if (ee[0].cBufIdx < ee[1].cBufIdx)   pivot1=ee[0].cBufIdx;  else pivot1=ee[1].cBufIdx;

#ifdef DEBUG
     qDebug() << "Indices (idx, Gidx Previous vs. actual)";
     qDebug() << "Buf0Idx P,N (Amp1): " << ee[0].cBufIdxP << " " << ee[0].cBufIdx << " -- (Amp 2):" << ee[1].cBufIdxP << " " << ee[1].cBufIdx;
     qDebug() << "Pivot: " << pivot0 << " " << pivot1;
     qDebug() << "---------";
#endif
// ---------------------------------------------------------------------------------------------------

     mutex->lock();
      quint64 tcpDataSize=pivot1-pivot0;
      for (quint64 i=0;i<tcpDataSize;i++) {
       tcpS.amp1=ee[0].cBuf[(pivot0+i-convN2)%cBufSz]; tcpS.amp2=ee[1].cBuf[(pivot0+i-convN2)%cBufSz];
       (*tcpBuffer)[(*tcpBufPIdx+i)%tcpBufSize]=tcpS;
      }
      (*tcpBufPIdx)+=tcpDataSize; // Update producer index
     mutex->unlock();
     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_eeg_msecs));
     //qDebug("octopus_acqd: <acqthread> Data validated to buffer..");
    } // eegImpedanceMode
   } // daemonRunning

   // Stop EEG Stream
   //mutex->lock(); mutex->unlock();
#ifdef EEMAGINE
   for (eemulti& e:ee) { delete e.stream; delete e.amp; }
#endif
   qDebug("octopus_acqd: <acqthread> Exiting thread..");
  }

 private:
  bool *daemonRunning,*eegImpedanceMode; QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPIdx,*tcpBufCIdx;
  chninfo *chnInfo; QMutex *mutex;
  
  sample smp; tcpsample tcpS;

  float sampleData,sum;
  quint64 pivot0,pivot1;

  int convN2,convN,cBufSz;

  std::vector<eemulti> ee;

  unsigned int smpCount,chnCount;

#ifdef EEMAGINE
  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
  int eeBuf0Chk,eeBuf1Chk;
#else

   quint64 amp0offset,amp1offset; float dc,ampl,frqA,frqB,dt;
#endif

};

#endif
