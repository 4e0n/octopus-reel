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

//#include <string>
//#include <cstring>
//#include <vector>
//#include <ostream>
//#include <stdexcept>
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <algorithm>
//#include <random>

//#include "../common/globals.h"

// Very small inline feeder that reads /opt/octopus/heeg.raw on demand.
// File format per sample: u64 offset, float32[ch= numAmps*chansPerAmp], float32 trigger.
struct Feeder {
 std::ifstream f;
 std::streampos dataStart;
 unsigned numAmps=0,chansPerAmp=0,totalCh=0,N=100;
 std::vector<float> batch,trig;        // [N][totalCh],[N]
 std::vector<unsigned long long> offs; // [N]
 unsigned left=0;                      // streams left to serve in this cycle

 bool openOnce(const char* path="/opt/octopus/data/raweeg/synth-eeg.raw",unsigned fs=1000) {
  if (f.is_open()) return true;
  f.open(path, std::ios::binary); if (!f) return false;
  // Mandatory signature
  static constexpr char kSig[] = "OCTOPUS_HEEG"; // Mandatory signature (fixed 12 bytes, no \0)
  static constexpr std::size_t kSigLen=sizeof(kSig)-1; // 12
  char sigBuf[kSigLen]={}; if (!f.read(sigBuf,std::streamsize(kSigLen))) return false;

  // TEMPORARY: peek an extra byte to swallow the stray '\0' from today's files
//  char maybeNul=0; f.read(&maybeNul,1); if (maybeNul!='\0') { // not a stray NUL, rewind by one
//                                                                f.seekg(-1, std::ios::cur); }
  // END TEMPORARY (remove after re-dumping files without \0)

  if (std::memcmp(sigBuf,kSig,kSigLen)!=0) return false; // Not our file format
  // Header: uint32_t numAmps, uint32_t chansPerAmp (little-endian on x86)
  uint32_t amps=0,chn=0;
  if (!f.read(reinterpret_cast<char*>(&amps),4)) return false;
  if (!f.read(reinterpret_cast<char*>(&chn),4)) return false;
  if (amps==0 || chn==0 || amps>8 || chn>256) return false; // Basic sanity checks
  numAmps=amps; chansPerAmp=chn; totalCh=numAmps*chansPerAmp;
  N=fs/10; if (N==0) N=100; // 100 ms worth
  batch.resize(static_cast<std::size_t>(N)*totalCh);
  trig.resize(N); offs.resize(N);
  dataStart=f.tellg(); // start of sample payload after header
  left=0;
  return true;
 }

 void readBlock() {
  const size_t frameBytes=size_t(totalCh)*sizeof(float);
  for (unsigned sc=0;sc<N;++sc) {
   if (!f.read((char*)&offs[sc],sizeof(unsigned long long))) rewindToStart();
   if (!f.read((char*)&batch[size_t(sc)*totalCh],frameBytes)) rewindToStart();
   if (!f.read((char*)&trig[sc],sizeof(float))) rewindToStart();
  }
 }

 void rewindToStart() {
  f.clear(); f.seekg(dataStart); // after rewinding, retry the read at caller's next attempt
 }
};

inline Feeder& feeder(){ static Feeder F; return F; }
