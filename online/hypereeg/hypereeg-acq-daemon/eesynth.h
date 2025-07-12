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

/* As the Eego amplifiers are not with me all the time but I can code **anywhere**, I translated
   the class hierarchy and namespace of Eemagine to a simulated environment called "Eesynth"
   which transparently and synthetically generated synthetic EEG data over the same system
   calls and functions. One can enable it by commenting out the EEMAGINE line definition
   in ../acqglobals.h (which is the main header with global definitions; either simulated or real).
   
   Due to copyright reasons, the respective Eemagine library isn't included in that project.

   This is the file of eesynth namespace and all classes that mimicks the overall system.
*/

#ifndef EESYNTH
#define EESYNTH

#include <string>
#include <vector>
#include <ostream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <algorithm>
#include <random>

#include "../hacqglobals.h"

namespace eesynth {
 namespace exceptions {
  class internalError : public std::runtime_error { public: explicit internalError(const std::string &msg) : std::runtime_error(msg) {} };
  class unknown : public std::runtime_error { public: explicit unknown(const std::string &msg) : std::runtime_error(msg) {} };
 }

class buffer {
 public:
  buffer(unsigned int channel_count=0,unsigned int sample_count=0):_data(channel_count*sample_count),_channel_count(channel_count),_sample_count(sample_count) {}
  void setCounts(unsigned int channel_count,unsigned int sample_count) {
   _data.resize(channel_count*sample_count); _channel_count=channel_count; _sample_count=sample_count;
  }
  const unsigned int& getChannelCount() const { return _channel_count; }
  const unsigned int& getSampleCount() const { return _sample_count; }
  const double& getSample(unsigned int channel,unsigned int sample) const { 
  //if (channel==0)
  //qDebug() << "eesynth: smpIdx=" << sample << " chnIdx=" << channel << " value=" << _data[channel+sample*_channel_count];

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
 out << "channel(";
 out << c.getIndex();
 out << ", ";
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
    t=0.; // chnList=cl;
    dc=0.001000; // to simulate High-Pass - e.g. DC=1mV
    a0=0.000050; // 50uVp (100uVpp) mimicks EEG
    dt=1./(double)(smpRate); frqA=1.; frqB=48.; // 1Hz
    trigger=counter=0.;
   }
  }
  ~stream() {}

  buffer getData() {
   buffer b;
   if (impMode) {
    b.setCounts(chnCount-2,1); // bipolars aren't counted for during imp mode?? Not handled currently!!!
    for (unsigned int cc=0;cc<chnCount;cc++) b.setSample(0,cc,2.71);
   } else {
    b.setCounts(chnCount,smpCount);
    for (unsigned int sc=0;sc<smpCount;sc++,t+=dt,counter+=1.) {
     for (unsigned int cc=0;cc<chnCount-2;cc++) {
      b.setSample(cc,sc,dc+a0*cos(2.0*M_PI*frqA*t) /*+(a0*1.0)*sin(2.0*M_PI*frqB*t)*/ );
     }
     b.setSample(chnCount-2,sc,trigger); trigger=0.; b.setSample(chnCount-1,sc,counter);
    }
   }
   return b;
  }

  void setTrigger(unsigned int t) { trigger=(double)t; };

  bool impMode; double trigger,counter;
  unsigned int chnCount,smpCount; double dc,a0,frqA,frqB,t,dt;
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
  //std::vector<int> getSamplingRatesAvailable() const=0;
  //std::vector<double> getReferenceRangesAvailable() const=0;
  //std::vector<double> getBipolarRangesAvailable() const=0;

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
  stream *str;
  unsigned int synthTrigger;
};

const unsigned int trig_offset[8]={1057,1163,1217,1349,1427,1503,1687,1734};

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

}

#endif
