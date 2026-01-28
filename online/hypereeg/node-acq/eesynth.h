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

/* As the Eego amplifiers are not with me all the time but I can code **anywhere**, I translated
   the class hierarchy and namespace of Eemagine to a simulated environment called "Eesynth"
   which transparently and synthetically generated synthetic EEG data over the same system
   calls and functions. One can enable it by commenting out the EEMAGINE line definition
   in ../common/globals.h (which is the main header with global definitions; either simulated or real).
   
   Due to copyright reasons, the respective Eemagine library isn't included in that project.

   This is the file of eesynth namespace and all classes that mimicks the overall system.
*/

#pragma once

#include <string>
#include <cstring>
#include <vector>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <random>

#include "../common/globals.h"

// Very small inline feeder that reads /opt/octopus/data/heeg.raw on demand.
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

namespace eesynth {

// internal amp-id assignment for stream objects (no public API change)
// auto amp-id (0,1,2,...) for stream instances (no API change)
inline unsigned& _amp_id_counter() { static unsigned c=0; return c; }
inline unsigned alloc_amp_id() { return _amp_id_counter()++; }
inline void reset_amp_ids() { _amp_id_counter()=0; }

// ========== Master playback ==========
// Reads ONE shared file, prepares the next 100-sample batch once,
// and lets two per-amp streams copy their 66 channels from that batch.

// These arbitrary offsets mimic the randomly arriving physical SYNC triggers on a real amp.
const unsigned int trig_offset[8]={1057,1163,1217,1349,1427,1503,1687,1734};

namespace exceptions {
 class internalError : public std::runtime_error {
  public: explicit internalError(const std::string &msg) : std::runtime_error(msg) {}
 };
 class unknown : public std::runtime_error { public: explicit unknown(const std::string &msg) : std::runtime_error(msg) {} };
}

class buffer {
 public:
  buffer(unsigned int channel_count=0,unsigned int sample_count=0) :
	    _data(channel_count*sample_count),_channel_count(channel_count),_sample_count(sample_count) {}
  void setCounts(unsigned int channel_count,unsigned int sample_count) {
   _data.resize(channel_count*sample_count); _channel_count=channel_count; _sample_count=sample_count;
  }
  const unsigned int& getChannelCount() const { return _channel_count; }
  const unsigned int& getSampleCount() const { return _sample_count; }
  const double& getSample(unsigned int channel,unsigned int sample) const { 
  //if (channel==0)
  // qDebug() << "eesynth: smpIdx=" << sample << " chnIdx=" << channel << " value=" << _data[channel+sample*_channel_count];

  return _data[channel+sample*_channel_count]; }
  void setSample(unsigned int channel,unsigned int sample,double value) { _data[channel+sample*_channel_count]=value; }
  size_t size() const { return _data.size(); }
  double* data() { return _data.data(); }
 private:
  std::vector<double> _data;
  unsigned int _channel_count,_sample_count;
};

class channel {
 public:
  enum channel_type {none,reference,bipolar,trigger,sample_counter,impedance_reference,impedance_ground };
  channel():_index(0),_type(none) {}
  channel(unsigned int index,channel_type type):_index(index),_type(type) {}
  unsigned int getIndex() const { return _index; }
  channel_type getType() const { return _type; }
  bool operator==(const channel& other) const { return _type==other._type; }
  bool operator!=(const channel& other) const { return !operator==(other); }
 protected:
  unsigned int _index;
  channel_type _type;
};

inline std::ostream& operator<<(std::ostream &out,const channel &c) {
 out << "channel("; out << c.getIndex(); out << ", ";
 switch (c.getType()) {
  case channel::reference:out << "reference"; break;
  case channel::bipolar:out << "bipolar"; break;
  case channel::trigger: out << "trigger"; break;
  case channel::sample_counter: out << "sample_counter"; break;
  case channel::impedance_reference: out << "impedance_reference"; break;
  case channel::impedance_ground: out << "impedance_ground"; break;
  default: out << "?"; break;
 }
 out << ")";
 return out;
}

class stream {
 public:
  stream(unsigned int c,unsigned int smpRate) {
   chnCount=c;
   if (smpRate==0) { impMode=true; smpCount=1; }
   else { impMode=false; smpCount=100;
    t=0.0f; dt=1./(double)(smpRate);
    trigger=counter=0.;
   }
   ampId_=eesynth::alloc_amp_id();
   // Open file
  }
  ~stream() {
   // Close file
  }
  //~stream() = default;

  buffer getData() {
   buffer b;
   if (impMode) {
    b.setCounts(chnCount-2,1); // bipolar chns aren't counted for during imp mode?? Not handled currently!!!
    for (unsigned int cc=0;cc<chnCount;++cc) b.setSample(0,cc,2.71);
    return b;
   }

   Feeder& F=feeder(); // Embedded file playback
   const unsigned fs=(dt>0.0) ? unsigned(1.0/dt+0.5):1000u; // derive fs from dt if possible; fallback to 1000 Hz

   if (!F.openOnce("/opt/octopus/data/raweeg/synth-eeg.raw",fs)) { // If no file, output zeros
    const unsigned N=smpCount;
    const unsigned eegCh=(chnCount>=2)? chnCount-2 : chnCount;
    b.setCounts(chnCount,N);

    const double counter0=counter;
    PARFOR(sc,0,int(N)) {
    //for (unsigned sc=0;sc<N;++sc) {
     for (unsigned cc=0;cc<eegCh;++cc) b.setSample(cc,sc,0.0);
     b.setSample(chnCount-2,sc,0.0);
     b.setSample(chnCount-1,sc,counter0+double(sc));
    }
    counter+=N; t+=dt*N; trigger=0.0;
    return b;
   }
 
   if (F.left==0) { // First stream call of the cycle reads the next block
    F.readBlock(); F.left=F.numAmps; // serve this many streams before next read
   }

   const unsigned eegCh=(chnCount>=2) ? chnCount-2 : chnCount;
   const unsigned N=F.N;
   b.setCounts(chnCount,N);

   // bounds check (defensive)
   if (F.chansPerAmp < eegCh || (ampId_+1)*F.chansPerAmp>F.totalCh) {
    const double counter0=counter;
    PARFOR(sc,0,int(N)) {
    //for (unsigned sc=0;sc<N;++sc) {
     for (unsigned cc=0;cc<eegCh;++cc) b.setSample(cc,sc,0.0);
     b.setSample(chnCount-2,sc,0.0);
     b.setSample(chnCount-1,sc,counter0+double(sc));
    }
   } else {
    const unsigned base=ampId_*F.chansPerAmp;
    double trigOnce=trigger;   // capture one-shot; we’ll consume it on the first sample
    PARFOR(sc,0,int(N)) {
    //for (unsigned sc=0;sc<N;++sc) {
     const float* src=&F.batch[size_t(sc)*F.totalCh+base];
     // contiguous copy of EEG channels for sample sc
     for (unsigned cc=0;cc<eegCh;++cc) b.setSample(cc,sc,double(src[cc]));
     const double trigFile=double(F.trig[sc]);
     const double trigOut=trigFile+((sc==0) ? trigOnce:0.0); // inject runtime one-shot
     b.setSample(chnCount-2,sc,trigOut);
     const double cntVal = double(F.offs[sc]); // sample index/offset from file
     b.setSample(chnCount-1, sc, cntVal);
    }
   }

   counter+=N; t+=dt*N; trigger=0.0; // Local time/counter continue to advance (optional)

   if (F.left>0) --F.left; // mark this stream as served in this cycle

   return b;
  }

  void setTrigger(unsigned int t) { trigger=(double)t; };

  bool impMode; double trigger,counter;
  unsigned int chnCount,smpCount; double t=0.0,dt=0.0;
 private:
  unsigned int ampId_=0; // Which amplifier this stream represents
};

class amplifier {
 public:
  amplifier(std::string s,unsigned int sto) {
   serialNumber=s; syncTrigOffset=sto; // following a sync event via parallel port, trigger sent after this many iterations.
   synthTrigger=0; eegStreamOpen=false;
  }
  ~amplifier() {}

  std::vector<channel> getChannelList(quint64 ref_mask,quint64 bip_mask) {
   channel c; quint64 m; chnList.resize(0); unsigned int idx=0;
   m=ref_mask; while (m!=0) { if (m&0x1) chnList.push_back(channel(idx,channel::reference)); m>>=1; idx++; }
   m=bip_mask; while (m!=0) { if (m&0x1) chnList.push_back(channel(idx,channel::bipolar)); m>>=1; idx++; }
   chnList.push_back(channel(idx,channel::trigger)); idx++;
   chnList.push_back(channel(idx,channel::sample_counter));
   return chnList;
  }

  std::string getSerialNumber() { return serialNumber; }

  // -- These may be implemented in the future if needed.
  //std::vector<int> getSamplingRatesAvailable() const=0;
  //std::vector<double> getReferenceRangesAvailable() const=0;
  //std::vector<double> getBipolarRangesAvailable() const=0;

  void setEEGFeed(std::string fn) {
   eegFeedName=fn;
  }

  stream* OpenEegStream(int sampling_rate,double reference_range,double bipolar_range,const std::vector<channel>& channel_list) {
   impedanceMode=false; smpRate=sampling_rate; refRange=reference_range; bipRange=bipolar_range; chnList=channel_list;
   str=new stream(chnList.size(),smpRate); eegStreamOpen=true;
   return str;
  }

  stream* OpenImpedanceStream(const std::vector<channel>& channel_list) {
   impedanceMode=true; chnList=channel_list; eegStreamOpen=false;
   str=new stream(chnList.size(),0);
   return str;
  }

  void setTrigger(unsigned int t) { if (eegStreamOpen) str->setTrigger(t); };

 private:
  std::string serialNumber; bool impedanceMode,eegStreamOpen; unsigned int syncTrigOffset;
  unsigned int smpRate; float refRange,bipRange; std::vector<channel> chnList;
  stream *str; unsigned int synthTrigger;
  std::string eegFeedName;
};

class factory { // Creates any number of virtual amplifiers identical to EE.
 public:
  factory(unsigned int ampCount) {
   // Create serial pool with shuffled order
   unsigned int i,no; amplifier *a;
   for (i=0,no=100071;i<EE_MAX_AMPCOUNT;i++) { sNos.push_back(std::to_string(no)); no++; }
   std::random_device rd;
   std::mt19937 rng(rd()); // Mersenne Twister engine
   std::shuffle(sNos.begin(),sNos.end(),rng);
   for (std::string s:sNos) std::cout << s;
   
   // Create desired number of amplifier
   for (i=0;i<ampCount;i++) { a=new amplifier(sNos[i],trig_offset[i]); amps.push_back(a); }
  }
  std::vector<amplifier*> getAmplifiers() {
   return amps;
  }
 private:
  std::vector<amplifier*> amps;
  std::vector<std::string> sNos;
};

} // eesynth namespace
