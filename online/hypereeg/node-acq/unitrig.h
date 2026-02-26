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

#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>

struct UniTrig {
 // State
 std::atomic_bool syncOngoing{false};
 std::atomic_bool syncPerformed{false};

 // Per-amp bookkeeping (valid only during sync window)
 std::vector<bool> syncSeenAtAmp; uint32_t syncSeenAtAmpCount=0;
 std::vector<uint64_t> syncSeenAtAmpLocalIdx; // "local timebase" index (baseIdx+smpIdx)

 // Output: how much each amp lags behind the earliest amp during sync
 std::vector<uint32_t> ampAlignOffset; // size=ampCount

 // stats
 uint64_t trigMismatchCounter=0; // >=2 non-zero triggers but different values
 uint64_t trigMissingCounter=0;  // some non-zero but not all amps non-zero
 uint64_t trigProduced=0;        // samples processed (merge evaluated)
 uint64_t trigNonZeroProduced=0; // merged!=0

 uint64_t syncBeginLocalIdx=0;    // localIdx at beginSync (AcqThread supplies)
 uint64_t syncDeadlineLocalIdx=0; // begin + timeoutSamples

 void init(size_t ampCount) {
  syncSeenAtAmp.assign(ampCount,false);
  syncSeenAtAmpLocalIdx.assign(ampCount,0);
  ampAlignOffset.assign(ampCount,0);
  syncSeenAtAmpCount=0;
//  syncOngoing.store(false,std::memory_order_release);
//  syncPerformed.store(false,std::memory_order_release);
 }

 void beginSync(uint64_t nowLocalIdx,uint64_t timeoutSamples) {
  syncPerformed.store(false,std::memory_order_release);
  syncOngoing.store(true,std::memory_order_release);
  std::fill(syncSeenAtAmp.begin(),syncSeenAtAmp.end(),false);
  std::fill(syncSeenAtAmpLocalIdx.begin(),syncSeenAtAmpLocalIdx.end(),0);
  syncSeenAtAmpCount=0;
  std::fill(ampAlignOffset.begin(),ampAlignOffset.end(),0);
  syncBeginLocalIdx=nowLocalIdx;
  syncDeadlineLocalIdx=nowLocalIdx+timeoutSamples;
 }

 // Call from fetch loop when TRIG_AMPSYNC from ampIdx is seen
 inline bool noteSyncSeen(size_t ampIdx,uint64_t localIdx) {
  if (!syncOngoing.load(std::memory_order_relaxed)) return false;
  if (syncSeenAtAmp[ampIdx]) return false;
  syncSeenAtAmp[ampIdx]=true;
  syncSeenAtAmpLocalIdx[ampIdx]=localIdx;
  ++syncSeenAtAmpCount;
  return true;
 }

 // Returns true exactly once when all amps have seen sync and offsets computed
 bool tryFinalizeIfReady() {
  if (!syncOngoing.load(std::memory_order_acquire)) return false;
  if (syncSeenAtAmpCount!=syncSeenAtAmp.size()) return false;

  const uint64_t minIdx=*std::min_element(syncSeenAtAmpLocalIdx.begin(),syncSeenAtAmpLocalIdx.end());
  for (size_t a=0;a<syncSeenAtAmp.size();++a) {
   ampAlignOffset[a]=uint32_t(syncSeenAtAmpLocalIdx[a]-minIdx);
  }
  syncOngoing.store(false,std::memory_order_release);
  syncPerformed.store(true,std::memory_order_release);
  syncSeenAtAmpCount=0;
  return true;
 }

 // Merge per-amp triggers into a single trigger code.
 // Returns merged trigger (0 if none).
 inline unsigned mergeTriggers(const unsigned* t,unsigned ampCount) {
  unsigned hwTrig=0,nonZero=0;
  for (unsigned a=0;a<ampCount;++a) {
   const unsigned ta=t[a];
   if (!ta) continue;
   ++nonZero;
   if (!hwTrig) hwTrig=ta;
   else if (hwTrig!=ta) ++trigMismatchCounter;
  }
  if (hwTrig && nonZero!=ampCount) ++trigMissingCounter;
  ++trigProduced;
  if (hwTrig) ++trigNonZeroProduced;
  return hwTrig;
 }
};
