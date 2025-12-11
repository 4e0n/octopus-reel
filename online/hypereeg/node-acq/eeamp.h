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

#include "../common/globals.h"
#include "iirfilter.h"

#ifdef EEMAGINE
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
using namespace eemagine::sdk;
#else
using namespace eesynth;
#endif

// [0.1-100Hz] Band-pass
const std::vector<double> b_01_100={0.06734199, 0.         ,-0.13468398, 0.        ,0.06734199};
const std::vector<double> a_01_100={1.        ,-3.14315078, 3.69970174,-1.96971135,0.41316049};

// [1-40Hz] Band-pass
const std::vector<double> b_1_40={0.01274959, 0.       ,-0.02549918, 0.        ,0.01274959};
const std::vector<double> a_1_40={1.        ,-3.6532502, 5.01411673,-3.06801391,0.7071495 };

// [2-40Hz] Band-pass
const std::vector<double> b_2_40={0.01215196, 0.        ,-0.02430392, 0.        ,0.01215196};
const std::vector<double> a_2_40={1.        ,-3.65903703, 5.03244992,-3.08686268,0.71345829};

// Notch@50Hz (Q=30)
const std::vector<double> b_notch={0.99479124,-1.89220538,0.99479124};
const std::vector<double> a_notch={1.        ,-1.89220538,0.98958248};

// Delta (2-4)
const std::vector<double> b_delta={3.91302054e-05, 0.00000000e+00,-7.82604108e-05, 0.00000000e+00,3.91302054e-05};
const std::vector<double> a_delta={1.            ,-3.98160009    , 5.94559129    ,-3.94637655    ,0.98238545};

// Theta (4-8)
const std::vector<double> b_theta={0.00015515, 0.        ,-0.0003103 , 0.      ,0.00015515};
const std::vector<double> a_theta={1.        ,-3.96195654, 5.88903994,-3.892163,0.96508117};

// Alpha (8-14)
const std::vector<double> b_alpha={0.00034604, 0.       ,-0.00069208, 0.        ,0.00034604};
const std::vector<double> a_alpha={1.        ,-3.9379744, 5.82427903,-3.83436731,0.94808171};

// Beta (14-28)
const std::vector<double> b_beta={0.00182013, 0.        ,-0.00364026, 0.        ,0.00182013};
const std::vector<double> a_beta={1.        ,-3.84577528, 5.57661047,-3.61363652,0.88302609};

// Gamma (28-40)
const std::vector<double> b_gamma={0.00134871, 0.        ,-0.00269742, 0.        ,0.00134871};
const std::vector<double> a_gamma={1.        ,-3.80766385, 5.52048605,-3.60983953,0.89885899};

struct EEAmp {
 unsigned int id=0;
 amplifier *amp=nullptr;
 std::vector<channel> chnList;
 // IIR filter states and others.. for each channel of the amplifier
 std::vector<IIRFilter> filterListBP,filterListN,filterListD,filterListT,filterListA,filterListB,filterListG;
 buffer buf; stream *str=nullptr; // EEbuf and EEStream
 unsigned int t=0,smpCount=0;     // data count int buffer
 unsigned int baseSmpIdx=0;       // Base Sample Index: is subtracted from all upcoming smpIdx's
 unsigned int smpIdx=0;           // Absolute Sample Index: sent from the amplifier
 std::vector<Sample> cBuf; quint64 cBufIdx; // Abstract (our) buffer

 EEAmp(unsigned int idx=0,amplifier *ampx=nullptr,
       quint64 rMask=0,quint64 bMask=0,size_t cbs=0,size_t ci=0,size_t chnCount=0) { init(idx,ampx,rMask,bMask,cbs,ci,chnCount); }

 void init(unsigned int idx,amplifier *ampx,quint64 rMask,quint64 bMask,size_t cbs,size_t ci,size_t chnCount) {
  id=idx; amp=ampx;
  chnList=amp->getChannelList(rMask,bMask);
  cBuf.assign(cbs,Sample(chnCount)); cBufIdx=ci;
  str=nullptr;
  t=0;
  smpCount=0;
  baseSmpIdx=0;
  smpIdx=0;
  cBufIdx=0;

  // Initialize filters
  filterListBP.reserve(chnCount); filterListN.reserve(chnCount);
  filterListD.reserve(chnCount); filterListT.reserve(chnCount);
  filterListA.reserve(chnCount); filterListB.reserve(chnCount); filterListG.reserve(chnCount);

  for (size_t chnIdx=0;chnIdx<chnCount;chnIdx++) {
   filterListBP.emplace_back(b_2_40,a_2_40); filterListN.emplace_back(b_notch,a_notch);
   filterListD.emplace_back(b_delta,a_delta); filterListT.emplace_back(b_theta,a_theta);
   filterListA.emplace_back(b_alpha,a_alpha); filterListB.emplace_back(b_beta,a_beta); filterListG.emplace_back(b_gamma,a_gamma);
  }
 }

 stream* OpenEegStream(int sampling_rate,double reference_range,double bipolar_range,const std::vector<channel>& channel_list) {
#ifndef EEMAGINE
 //qDebug() << "/opt/eeg"+QString::number(id+1)+".raw";
 amp->setEEGFeed(("/opt/eeg"+QString::number(id+1)).toStdString());
#endif
  return amp->OpenEegStream(sampling_rate,reference_range,bipolar_range,channel_list);
 }
};
