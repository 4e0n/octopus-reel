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

#ifndef _EEAMP_H
#define _EEAMP_H

#include "../hacqglobals.h"
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
const std::vector<double> b={0.06734199,0.          ,-0.13468398,0.         ,0.06734199};
const std::vector<double> a={1.        , -3.14315078, 3.69970174,-1.96971135,0.41316049};

// Notch@50Hz (Q=30)
const std::vector<double> nb={0.99479124,-1.89220538,0.99479124};
const std::vector<double> na={1.        ,-1.89220538,0.98958248};

struct EEAmp {
 unsigned int idx=0;
 amplifier *amp=nullptr;
 std::vector<channel> chnList;
 std::vector<IIRFilter> bpFilterList,nFilterList; // IIR filter states and others.. for each channel of the amplifier
 buffer buf; stream *str=nullptr; // EEbuf and EEStream
 unsigned int t=0,smpCount=0; // data count int buffer
 unsigned int baseSmpIdx=0; // Base Sample Index: is subtracted from all upcoming smpIdx's
 unsigned int smpIdx=0;     // Absolute Sample Index: sent from the amplifier
 std::vector<Sample> cBuf; quint64 cBufIdx; // Abstract (our) buffer

 EEAmp(unsigned int id=0,amplifier *ampx=nullptr,
       quint64 rMask=0,quint64 bMask=0,
       size_t cbs=0,size_t ci=0,size_t chnCount=0) { init(id,ampx,rMask,bMask,cbs,ci,chnCount); }

 void init(unsigned int id,amplifier *ampx,
           quint64 rMask,quint64 bMask,
           size_t cbs,size_t ci,size_t chnCount) {
  idx=id; amp=ampx;
  chnList=amp->getChannelList(rMask,bMask);
  cBuf.assign(cbs,Sample(chnCount)); cBufIdx=ci;
  str=nullptr;
  t=0;
  smpCount=0;
  baseSmpIdx=0;
  smpIdx=0;
  cBufIdx=0;

  // Initialize filters
  bpFilterList.reserve(chnCount); nFilterList.reserve(chnCount);
  for (size_t chnIdx=0;chnIdx<chnCount;chnIdx++) { bpFilterList.emplace_back(b,a); nFilterList.emplace_back(nb,na); }
 }
};

#endif
