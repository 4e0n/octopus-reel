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
    for (unsigned int i=0;i<e.fY[0].size();i++) printf("%f ",e.fY[chn][i]); printf("\n"); }
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
  AcqThread(QObject* parent,chninfo *ci,QVector<tcpsample> *tb,quint64 *pidx,
            bool *r,bool *im,QMutex *m) : QThread(parent) {
   mutex=m; chnInfo=ci; tcpBuffer=tb; tcpBufPivot=pidx; daemonRunning=r; eegImpedanceMode=im;
   tcpS.trigger=0;
   convN=chnInfo->sampleRate/50; convN2=convN/2;
   convL=4*chnInfo->sampleRate; convL2=convN/2; // 4 seconds MA for high pass

   filterMA=true; filterIIR_1_40=false;
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

  void fetchEegData() { float sum0,sum1;
#ifdef EEMAGINE
   using namespace eemagine::sdk;
#else
   using namespace eesynth;
#endif
   std::vector<double> v;
   //for (eex& e:ee) {
   for (unsigned int i=0;i<ee.size();i++) {
    try {
     ee[i].buf=ee[i].str->getData(); chnCount=ee[i].buf.getChannelCount();
     ee[i].smpCount=ee[i].buf.getSampleCount();
     //qDebug() << e.smpCount << " " << chnCount;
     if (chnCount!=chnInfo->totalChnCount) qDebug() << "octopus_acqd: <fetchEegData> Channel count mismatch!!!";
     //qDebug() << "Amp " << i+1 << ": " << smpCount << " samples," << chnCount << " channels.";
    } catch (const exceptions::internalError& ex) {
     std::cout << "Exception" << ex.what() << std::endl;
    }
    // returns the minimum index for the number of samples fetched via any amp, to use later in circular buffer updates.
    cBufIdxList[i]=ee[i].cBufIdxP=ee[i].cBufIdx;
    //if (i==0) {
    // qDebug() << "Sample count: " << ee[i].smpCount << "Channel count: " << chnCount;
    // for (unsigned int j=0;j<ee[i].smpCount;j++) { qDebug() << ee[i].buf.getSample(0,j); }
    //}
    for (unsigned int j=0;j<ee[i].smpCount;j++) { smp.marker=M_PI;
     for (unsigned int k=0;k<chnCount-2;k++) smp.data[k]=ee[i].buf.getSample(k,j);
     smp.trigger=ee[i].buf.getSample(chnCount-2,j);
     smp.offset=ee[i].absSmpIdx=ee[i].buf.getSample(chnCount-1,j);
     ee[i].cBuf[(ee[i].cBufIdx+j)%cBufSz]=smp;
     //if (j==e.smpCount-1) e.absSmpIdx=smp.offset;
    }

    // ----- Filtering -----

    // Past average subtraction for High Pass + Moving Average for 50Hz and harmonics
    if (filterMA)
     for (unsigned int j=0;j<chnCount-2;j++) {
      for (int k=-convN2;k<(int)(ee[i].smpCount)-convN2;k++) {
       sum0=0.; for (int m=k-convL;m<k;m++)      sum0+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].data[j];
       sum1=0.; for (int m=-convN2;m<convN2;m++) sum1+=ee[i].cBuf[(ee[i].cBufIdx+k+m)%cBufSz].data[j];
       ee[i].cBuf[(ee[i].cBufIdx+k+convN2)%cBufSz].dataF[j]=sum1/convN-sum0/convL;
      }
     }

    if (filterIIR_1_40) // Cascade to MA50Hz
     for (unsigned int j=0;j<chnCount-2;j++) {
      v.resize(0);
      for (int k=0;k<(int)(ee[i].smpCount);k++) v.push_back(ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].data[j]);
      bpf.processFiltFilt(v,ee[i],j);
      for (int k=0;k<(int)(ee[i].smpCount);k++) ee[i].cBuf[(ee[i].cBufIdx+k)%cBufSz].dataF[j]=v[k];
     }

    // ---------------------

    ee[i].cBufIdx+=ee[i].smpCount;
   }
   cBufPivotP=cBufPivot; cBufPivot=*std::min_element(cBufIdxList.begin(),cBufIdxList.end());
  }

// ------------------------------------------------

  virtual void run() {
   int tcpBufSize=tcpBuffer->size();
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
   for (unsigned int i=0;i<eeAmps.size();i++) { eex e; e.idx=i; e.amp=eeAmps[i];
    e.chnList=e.amp->getChannelList(rMask,bMask);
    // Select same channels for all eeAmps (i.e. All 64/64 referentials and 2/24 of bipolars)
    //e.chnList=e.amp->getChannelList(0xffffffffffffffff,0x0000000000000003);
    //e.chnList=e.amp->getChannelList(0x0000000000000000,0x0000000000000001);
    cBufSz=chnInfo->sampleRate*CBUF_SIZE_IN_SECS; e.cBufIdxP=0; e.cBufIdx=cBufSz/2; //+convN;
    e.cBuf.resize(cBufSz); e.cBufF.resize(cBufSz); e.imps.resize(chnInfo->physChnCount);
    e.fX.resize(chnInfo->physChnCount); for (int i=0;i<e.fX.size();i++) for (int j=0;j<e.fX[i].size();j++) e.fX[i][j]=0.;
    e.fY.resize(chnInfo->physChnCount); for (int i=0;i<e.fY.size();i++) for (int j=0;j<e.fY[i].size();j++) e.fY[i][j]=0.;
    ee.push_back(e);
   }
   cBufIdxList.resize(EE_AMPCOUNT);

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

     mutex->lock();
      quint64 tcpDataSize=cBufPivot-cBufPivotP;
      //qDebug() << cBufPivotP << " " << cBufPivot;
      for (quint64 i=0;i<tcpDataSize;i++) {
       tcpS.amp[0]=ee[0].cBuf[(cBufPivotP+i-convN2)%cBufSz];
       tcpS.amp[1]=ee[1].cBuf[(cBufPivotP+i-convN2)%cBufSz];
       (*tcpBuffer)[(*tcpBufPivot+i)%tcpBufSize]=tcpS;
      }
      (*tcpBufPivot)+=tcpDataSize; // Update producer index
     mutex->unlock();

     cBufPivotP=cBufPivot;
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
  QVector<tcpsample> *tcpBuffer; quint64 *tcpBufPivot,*tcpBufCIdx; tcpsample tcpS; sample smp;
  std::vector<eex> ee; chninfo *chnInfo; unsigned int cBufSz,smpCount,chnCount;
  std::vector<unsigned int> cBufIdxList;

  int convN2,convN,convL2,convL; quint64 cBufPivot,cBufPivotP;

  QMutex *mutex; bool *daemonRunning,*eegImpedanceMode;

  bool filterMA,filterIIR_1_40;

  // Butterworth coefficients (replace with MATLAB-generated values)
  HighPassFilter hpf;
  BandPassFilter bpf;
};

#endif
