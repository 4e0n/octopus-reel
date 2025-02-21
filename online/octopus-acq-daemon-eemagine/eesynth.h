#ifndef EESYNTH
#define EESYNTH

#include <string>
#include <vector>
#include <ostream>
#include <stdexcept>

namespace eesynth {
 namespace exceptions {
  class internalError : public std::runtime_error { public: explicit internalError(const std::string &msg) : std::runtime_error(msg) {} };
  class unknown : public std::runtime_error { public: explicit unknown(const std::string &msg) : std::runtime_error(msg) {} };
 }

class buffer {
 public:
  buffer(unsigned int channel_count=0,unsigned int sample_count=0):_data(channel_count*sample_count),_channel_count(channel_count),_sample_count(sample_count) {}
  const unsigned int& getChannelCount() const { return _channel_count; }
  const unsigned int& getSampleCount() const { return _sample_count; }
  const double& getSample(unsigned int channel,unsigned int sample) const { return _data[channel+sample*_channel_count]; }
  void setSample(unsigned int channel,unsigned int sample,double value) { _data[channel+sample*_channel_count]=value; }
  size_t size() const { return _data.size(); }
  double* data() { return _data.data(); }
 protected:
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
  stream(unsigned int c,unsigned int s,std::vector<channel>& cl,unsigned int smpRate) {
   chnCount=c; smpCount=s; chnList=cl; t=0.0;
   dc=0.0; // to simulate High-Pass
   ampl=0.000100; // 100uV mimicks EEG
   dt=1.0/(double)(smpRate); frqA=10.0; frqB=48.0;
  }
  ~stream() {}
  std::vector<channel> getChannelList() { return chnList; }
  buffer getData() { buffer b(chnCount,smpCount);
   for (unsigned int i=0;i<smpCount;i++) {
    for (unsigned int j=0;j<chnCount;j++) {
     b.setSample(i,j,dc+ampl*cos(2.0*M_PI*frqA*t)+(ampl/1.0)*sin(2.0*M_PI*frqB*t));
    }
    t+=dt;
   }
   return b;
  }
  std::vector<channel> chnList;
  unsigned int chnCount,smpCount; double dc,ampl,frqA,frqB,t,dt;
};

class amplifier {
 public:
  amplifier(std::string s) { serialNumber=s; sampleCount=100; }
  ~amplifier() {}
  std::vector<channel> getChannelList(quint64 ref_mask,quint64 bip_mask) {
   channelList.resize(0); unsigned int idx=0;
   channel c; quint64 m;
   m=ref_mask; while (m!=0) { if (m&0x1) channelList.push_back(channel(idx,channel::reference)); m>>=1; idx++; }
   m=bip_mask; while (m!=0) { if (m&0x1) channelList.push_back(channel(idx,channel::bipolar)); m>>=1; idx++; }
   channelList.push_back(channel(idx,channel::trigger)); idx++;
   channelList.push_back(channel(idx,channel::sample_counter)); idx++;
   return channelList;
  }
  std::string getSerialNumber() { return serialNumber; }
  //std::vector<int> getSamplingRatesAvailable() const=0;
  //std::vector<double> getReferenceRangesAvailable() const=0;
  //std::vector<double> getBipolarRangesAvailable() const=0;
  stream* OpenEegStream(int sampling_rate,double reference_range,double bipolar_range,const std::vector<channel>& channel_list) {
   smpRate=sampling_rate; refRange=reference_range; bipRange=bipolar_range; channelList=channel_list;
   eegStream=new stream(channelList.size(),sampleCount,channelList,smpRate);
   return eegStream;
  }
  stream* OpenImpedanceStream(const std::vector<channel>& channel_list) {
   channelList=channel_list;
   impedanceStream=new stream(channelList.size(),1,channelList,smpRate);
   return impedanceStream;
  }
  std::string serialNumber;
  std::vector<channel> channelList;
  stream* eegStream;
  stream* impedanceStream;
  unsigned int sampleCount;
  unsigned int smpRate; float refRange,bipRange;
};

class factory {
 public:
  factory() {
   amplifier *a;
   a=new amplifier(std::string("0077")); amps.push_back(a);
   a=new amplifier(std::string("0075")); amps.push_back(a);
  }
  std::vector<amplifier*> getAmplifiers() {
   return amps;
  }
  //virtual std::string getSerialNumber() const=0;
  std::vector<amplifier*> amps;
};

}

#endif
