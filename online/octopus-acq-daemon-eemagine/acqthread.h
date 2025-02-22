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
#include "../acqglobals.h"

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

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(QObject* parent,chninfo *ci,QVector<tcpsample> *tb,quint64 *pidx,quint64 *cidx,
            bool *r,bool *im,QMutex *m) : QThread(parent) {
   mutex=m; chnInfo=ci; tcpBuffer=tb; tcpBufPIdx=pidx; tcpBufCIdx=cidx; daemonRunning=r; eegImpedanceMode=im;
   tcpS.trigger=0;
   convN=chnInfo->sampleRate/50; convN2=convN/2;
  }

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

  void fetchEegData() {
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   //for (eex& e:ee) {
   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData();
     ee[i].smpCount=ee[i].buf.getSampleCount(); chnCount=ee[i].buf.getChannelCount();
     //qDebug() << e.smpCount << " " << chnCount;
     if (chnCount!=chnInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!";
     //qDebug() << "Amp " << i+1 << ": " << smpCount << " samples," << chnCount << " channels.";
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    ee[i].cBufIdxP=ee[i].cBufIdx;
    //if (i==0) {
    // qDebug() << "Sample count: " << ee[i].smpCount << "Channel count: " << chnCount;
    // for (unsigned int j=0;j<ee[i].smpCount;j++) { qDebug() << ee[i].buf.getSample(0,j); }
    //}
   }
  }

// ------------------------------------------------

  virtual void run() {
   float sum; int tcpBufSize=tcpBuffer->size();
#ifdef EEMAGINE
   using namespace eemagine::sdk; factory eeFact("libeego-SDK.so");
#else
   using namespace eesynth; factory eeFact;
#endif

   // --- Initial setup ---

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
   for (amplifier* eeAmp:eeAmps) { eex e; e.amp=eeAmp;
    e.chnList=e.amp->getChannelList(rMask,bMask);
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    //e.chnList=e.amp->getChannelList(0xffffffffffffffff,0x0000000000000003);
    //e.chnList=e.amp->getChannelList(0x0000000000000000,0x0000000000000001);
    cBufSz=chnInfo->sampleRate*CBUF_SIZE_IN_SECS; e.cBufIdx=cBufSz/2+convN; e.cBufIdxP=0;
    e.cBuf.resize(cBufSz); e.cBufF.resize(cBufSz); e.imps.resize(chnInfo->physChnCount);
    ee.push_back(e);
   }

   // ----- List unsorted vs. sorted
   for (unsigned int i=0;i<ee.size();i++) qDebug() << stoi(ee[i].amp->getSerialNumber());

   switchToEEGMode();

   // Main Loop

   while (*daemonRunning) {
    if (*eegImpedanceMode) {
     fetchImpedanceData();
     for (eex& e:ee) for (unsigned int j=0;j<e.chnList.size();j++) e.imps[j]=e.buf.getSample(j,0);
     std::this_thread::sleep_for(std::chrono::milliseconds(chnInfo->probe_impedance_msecs));
    } else {
     fetchEegData();
// ---------------------------------------------------------------------------------------------------
     for (eex& e:ee) for (unsigned int j=0;j<e.smpCount;j++) {
      smp.marker=M_PI;
      for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=e.buf.getSample(k,j);
      smp.trigger=e.buf.getSample(chnCount-2,j);
      smp.offset=e.absSmpIdx=e.buf.getSample(chnCount-1,j);
      e.cBuf[(e.cBufIdx+j)%cBufSz]=smp;
      //if (j==e.smpCount-1) e.absSmpIdx=smp.offset;
     }
     // Filtering
     for (eex& e:ee) for (unsigned int j=0;j<chnCount-2;j++)
      for (int k=-convN2;k<(int)(e.smpCount)-convN2;k++) {
       sum=0.; for (int m=0;m<convN;m++) sum+=e.cBuf[(e.cBufIdx+k+m)%cBufSz].data[j];
       e.cBuf[(e.cBufIdx+k+convN2)%cBufSz].dataF[j]=sum/convN;
      }

     for (eex& e:ee) e.cBufIdx+=e.smpCount;
//     qDebug() << "Sample count vs. Checkpoint: SC0:" << ee[0].smpCount << " CHK0:" << ee[0].absSmpIdx;
//     qDebug() << "                             SC1:" << ee[1].smpCount << " CHK1:" << ee[1].absSmpIdx;
     if (ee[0].cBufIdxP < ee[1].cBufIdxP) pivot0=ee[0].cBufIdxP; else pivot0=ee[1].cBufIdxP;
     if (ee[0].cBufIdx < ee[1].cBufIdx)   pivot1=ee[0].cBufIdx;  else pivot1=ee[1].cBufIdx;

//     qDebug() << "Indices (idx, Gidx Previous vs. actual)";
//     qDebug() << "Buf0Idx P,N (Amp1): " << ee[0].cBufIdxP << " " << ee[0].cBufIdx << " -- (Amp 2):" << ee[1].cBufIdxP << " " << ee[1].cBufIdx;
//     qDebug() << "Pivot: " << pivot0 << " " << pivot1;
//     qDebug() << "---------";

// ---------------------------------------------------------------------------------------------------

     mutex->lock();
      quint64 tcpDataSize=pivot1-pivot0;
      for (quint64 i=0;i<tcpDataSize;i++) {
       tcpS.amp[0]=ee[0].cBuf[(pivot0+i-convN2)%cBufSz];
       tcpS.amp[1]=ee[1].cBuf[(pivot0+i-convN2)%cBufSz];
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

   for (eex& e:ee) { delete e.str; delete e.amp; }

   qDebug("octopus_acqd: <acqthread> Exiting thread..");
  }

 private:
#ifdef EEMAGINE
  std::vector<eemagine::sdk::amplifier*> eeAmpsU;
#else
  std::vector<eesynth::amplifier*> eeAmpsU;
#endif
  QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPIdx,*tcpBufCIdx; tcpsample tcpS; sample smp;
  std::vector<eex> ee; chninfo *chnInfo; unsigned int cBufSz,smpCount,chnCount;

  int convN2,convN; quint64 pivot0,pivot1;

  QMutex *mutex; bool *daemonRunning,*eegImpedanceMode;
};

#endif
