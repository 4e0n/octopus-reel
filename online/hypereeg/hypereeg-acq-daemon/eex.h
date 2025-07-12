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

#ifndef _EEX_H
#define _EEX_H

#include "../hacqglobals.h"
#include "iirfilter.h"

#ifdef EEMAGINE
using namespace eemagine::sdk;
#else
using namespace eesynth;
#endif

typedef struct _EEx {
 unsigned int idx=0;
 amplifier *amp=nullptr;
 std::vector<channel> chnList;
 std::vector<IIRFilter> filterList;
 stream *str=nullptr;
 buffer buf;
 unsigned int t=0;
 unsigned int smpCount=0; // data count int buffer
 //unsigned int chnCount; // channel count int buffer -- redundant, to be asserted
 unsigned int baseSmpIdx=0; // Base Sample Index, to subtract from all upcoming basSmpIdx's
 unsigned int smpIdx=0; // Absolute Sample Index, as sent from the amplifier
 //std::vector<float> imps;
 quint64 cBufIdx=0;
 std::vector<Sample> cBuf;
 //std::vector<Sample> cBufF;
 //std::vector<std::array<double,IIR_HIST_SIZE> > fX,fY; // IIR filter History

 //_EEx(unsigned int id=0,size_t sample_count=0,size_t chn_count=0) : idx(id),cBuf(sample_count,Sample(chn_count)) {}
 _EEx(unsigned int id=0,amplifier *ampx=nullptr,quint64 rMask=0,quint64 bMask=0,size_t cbs=0,size_t ci=0,size_t chnCount=0,
      std::vector<float> b={},std::vector<float> a={}) {
  init(id,ampx,rMask,bMask,cbs,ci,chnCount,b,a);
 }

 void init(unsigned int id,amplifier *ampx,quint64 rMask,quint64 bMask,size_t cbs,size_t ci,size_t chnCount,
           std::vector<float> b,std::vector<float> a) {
  idx=id; amp=ampx; chnList=amp->getChannelList(rMask,bMask);
  cBuf.assign(cbs,Sample(chnCount)); cBufIdx=ci;
  str=nullptr;
  t=0;
  smpCount=0;
  baseSmpIdx=0;
  smpIdx=0;
  cBufIdx=0;
  // Initialize filters
  filterList.reserve(chnCount);
  for (size_t j=0;j<chnCount;++j) filterList.emplace_back(b,a);
 }
} EEx;

#endif
