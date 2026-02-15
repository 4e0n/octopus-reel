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

struct EEAmp {
 unsigned int id=0;
 amplifier *amp=nullptr;
 std::vector<channel> chnList;
 // for each channel of the amplifier
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
 }

 stream* OpenEegStream(int sampling_rate,double reference_range,double bipolar_range,const std::vector<channel>& channel_list) {
  return amp->OpenEegStream(sampling_rate,reference_range,bipolar_range,channel_list);
 }
};
