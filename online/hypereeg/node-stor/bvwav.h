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

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QByteArray>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>  // lrintf

struct BrainVisionMeta {
 QStringList channelNames;     // size=nChBV (EEG+optional derived channels)
 double sampleRateHz=1000.0;   // e.g. 1000
 QString unit="uV";
 double channelResolution=1.0; // if values already uV -> 1.0; if volts -> 1e6
};

struct WavMeta {
 int sampleRate=48000;
 int numChannels=1;    // mono audio
 // PCM 16-bit LE
};

class BVWav {
 public:
  BVWav()=default;
  ~BVWav() { stop(nullptr); }

  bool isOpen() const { return m_open; }
  uint64_t bvFramesWritten() const { return m_bvFramesWritten; }
  uint64_t wavFramesWritten() const { return m_wavFramesWritten; } // audioframes (at 48k)

  // filenameBase: full path without extension (base.vhdr, base.vmrk, base.eeg, base.wav)
  bool setup(const QString& filenameBase,const BrainVisionMeta& bvMeta,
             const WavMeta& wavMeta,QString* err=nullptr) {
   reset();
   if (bvMeta.channelNames.isEmpty()) return setErr(err,"BV: channelNames empty.");
   if (bvMeta.sampleRateHz<=0.0) return setErr(err,"BV: Requirement not satisfied: sampleRateHz>0.");
   if (wavMeta.sampleRate<=0) return setErr(err,"WAV: Requirement not satisfied: sampleRate>0.");
   if (wavMeta.numChannels<=0) return setErr(err,"WAV: Requirement not satisfied: numChannels>0.");
   m_bvMeta=bvMeta; m_nChBV=bvMeta.channelNames.size(); m_wavMeta=wavMeta;
   m_base=filenameBase;
   m_vhdrPath=m_base+".vhdr"; m_eegPath=m_base+".eeg"; m_vmrkPath=m_base+".vmrk"; m_wavPath=m_base+".wav";

   // Ensure directory exists
   {
     QFileInfo fi(m_base); QDir dir=fi.dir();
     if (!dir.exists()) {
      if (!dir.mkpath(".")) return setErr(err,"Cannot create directory: "+dir.absolutePath());
     }
   }

   // Open EEG (binary)
   m_fdEeg=::open(m_eegPath.toUtf8().constData(),O_CREAT|O_TRUNC|O_WRONLY,0644);
   if (m_fdEeg<0) return setErrno(err,"open .eeg failed");

   // Open VMRK (text)
   m_fdVmrk=::open(m_vmrkPath.toUtf8().constData(),O_CREAT|O_TRUNC|O_WRONLY,0644);
   if (m_fdVmrk<0) return failSetup(err,"open .vmrk failed");

   // Open VHDR (text)
   m_fdVhdr=::open(m_vhdrPath.toUtf8().constData(),O_CREAT|O_TRUNC|O_WRONLY,0644);
   if (m_fdVhdr<0) return failSetup(err,"open .vhdr failed");

   // Open WAV (binary)
   m_fdWav=::open(m_wavPath.toUtf8().constData(),O_CREAT|O_TRUNC|O_WRONLY,0644);
   if (m_fdWav<0) return failSetup(err,"open .wav failed");

   // Write headers
   if (!writeVmrkHeader(err)) return failSetup(err,"write vmrk header failed");
   if (!writeVhdr(err)) return failSetup(err,"write vhdr failed");
   if (!writeWavHeaderPlaceholder(err))return failSetup(err,"write wav header failed");

   m_open=true; m_bvFramesWritten=m_markerCount=m_wavFramesWritten=m_wavDataBytes=0;
   return true;
  }

  // -------- per-tick API (kept) --------

  // One 1kHz "tick": writes one BrainVision frame + exactly 48 audio samples to WAV.
  // - bvFrameInterleaved: float[nChBV] for this tick (one frame)
  // - audio48_i16: int16[48*numChannels] interleaved by channel if numChannels>1
  bool writeTick_BVFloat32_WavInt16(const float* bvFrameInterleaved,const int16_t* audio48_i16,QString* err=nullptr) {
   if (!m_open) return setErr(err,"writeTick called while not open.");
   if (!bvFrameInterleaved) return setErr(err,"BV frame pointer null.");
   if (!audio48_i16) return setErr(err,"audio48 pointer null.");

   // 1) BrainVision: write one frame=nChBVxfloat32
   {
     const int64_t nBytes=int64_t(m_nChBV)*int64_t(sizeof(float));
     if (!writeAll(m_fdEeg,bvFrameInterleaved,size_t(nBytes),err,"write .eeg failed")) return false;
     m_bvFramesWritten++;
   }

   // 2) WAV: write exactly 48 frames at 48k (per channel interleaved)
   {
     const int framesPerTick=48;
     const int ch=m_wavMeta.numChannels;
     const int samples=framesPerTick*ch;
     const int64_t nBytes=int64_t(samples)*int64_t(sizeof(int16_t));
     if (!writeAll(m_fdWav,audio48_i16,size_t(nBytes),err,"write .wav failed")) return false;
     m_wavFramesWritten+=uint64_t(framesPerTick);
     m_wavDataBytes+=uint64_t(nBytes);
   }
   return true;
  }

  // Variant: audio comes as float[-1..+1] (or arbitrary), we convert to PCM16.
  // Provide float audio48_f32 with length = 48*numChannels, interleaved.
  bool writeTick_BVFloat32_WavFloatToInt16(const float* bvFrameInterleaved,const float* audio48_f32,QString* err=nullptr) {
   if (!m_open) return setErr(err,"writeTick called while not open.");
   if (!bvFrameInterleaved) return setErr(err,"BV frame pointer null.");
   if (!audio48_f32) return setErr(err,"audio48 pointer null.");
   // BrainVision frame
   {
     const int64_t nBytes=int64_t(m_nChBV)*int64_t(sizeof(float));
     if (!writeAll(m_fdEeg,bvFrameInterleaved,size_t(nBytes),err,"write .eeg failed")) return false;
     m_bvFramesWritten++;
   }
   // Convert 48*ch floats -> int16 buffer (stack-friendly small)
   const int framesPerTick=48;
   const int ch=m_wavMeta.numChannels;
   const int samples=framesPerTick*ch;

   int16_t tmp[48*8]; // supports up to 8 channels without heap
   if (samples>int(sizeof(tmp)/sizeof(tmp[0])))
    return setErr(err,"WAV: too many channels for stack buffer; increase tmp size.");

   for (int i=0;i<samples;++i) {
    float x=audio48_f32[i];
    // Assuming normalized [-1..+1]:
    if (x>1.0f) x=1.0f;
    if (x<-1.0f) x=-1.0f;
    const int v=int(lrintf(x*32767.0f));
    tmp[i]=(v<-32768) ? -32768:(v>32767 ? 32767:int16_t(v));
   }

   const int64_t nBytes=int64_t(samples)*int64_t(sizeof(int16_t));
   if (!writeAll(m_fdWav,tmp,size_t(nBytes),err,"write .wav failed")) return false;

   m_wavFramesWritten+=uint64_t(framesPerTick);
   m_wavDataBytes+=uint64_t(nBytes);
   return true;
  }

  // Preferred API for RecThread (N=100)
  // Write N TcpSamples in two big writes:
  //  .eeg: N frames of float32 multiplexed (amp-major, channel-minor) using Sample::data
  //  .wav: N*48 frames of PCM16, currently mono only (numChannels must be 1)
  bool writeChunk_FromTcpSamples(const std::vector<TcpSample>& chunk,size_t ampCount,size_t chnCount,QString* err=nullptr) {
   if (!m_open) return setErr(err, "writeChunk called while not open.");
   if (chunk.empty()) return true;

   //   const size_t nChBV_expected = ampCount * chnCount;
   const size_t nEEG=ampCount*chnCount;
   const size_t nAUD=48;
   const size_t nChBV_expected=nEEG+nAUD;

   if ((size_t)m_nChBV!=nChBV_expected) {
    return setErr(err,QString("BV: channel count mismatch: header says %1, but expected=%2 (EEG=%3 + AUD=%4)")
                      .arg(m_nChBV).arg((qulonglong)nChBV_expected)
                      .arg((qulonglong)nEEG).arg((qulonglong)nAUD));
   }

   const size_t N=chunk.size();
   std::vector<float> bvBuf;
   bvBuf.resize(N*nChBV_expected);

   size_t k=0;
   for (size_t i=0;i<N;++i) {
    const TcpSample& t=chunk[i];
    // EEG part (132)
    for (size_t a=0;a<ampCount;++a) {
     const Sample& s=t.amp[a];
     for (size_t c=0;c<chnCount;++c) {
      bvBuf[k++]=s.data[c];
     }
    }
    // AUD part (48)
    for (size_t j=0;j<48;++j) {
     bvBuf[k++]=t.audioN[j];  // AUD0..AUD47
    }
   }
   const int64_t bvBytes=int64_t(bvBuf.size())*int64_t(sizeof(float));
   if (!writeAll(m_fdEeg,bvBuf.data(),size_t(bvBytes),err,"write .eeg failed")) return false;
   m_bvFramesWritten+=uint64_t(N);

   // ---- build WAV buffer ----
   const int framesPerTick=48;
   const int ch=m_wavMeta.numChannels;
   if (ch!=1) {
    return setErr(err,"WAV: writeChunk_FromTcpSamples currently supports numChannels=1 only.");
   }

   const size_t wavFrames=N*(size_t)framesPerTick;
   std::vector<int16_t> wavBuf;
   wavBuf.resize(wavFrames*(size_t)ch);

   size_t w=0;
   for (size_t i=0;i<N;++i) {
    const TcpSample& t=chunk[i];
    for (int j=0;j<framesPerTick;++j) {
     float x=t.audioN[j];
     if (x> 1.0f) x= 1.0f;
     if (x<-1.0f) x=-1.0f;
     const int v=int(lrintf(x*32767.0f));
     wavBuf[w++]=(v<-32768) ? -32768 : (v>32767 ? 32767:int16_t(v));
    }
   }

   const int64_t wavBytes=int64_t(wavBuf.size())*int64_t(sizeof(int16_t));
   if (!writeAll(m_fdWav,wavBuf.data(),size_t(wavBytes),err,"write .wav failed")) return false;

   m_wavFramesWritten+=uint64_t(wavFrames);
   m_wavDataBytes+=uint64_t(wavBytes);

   return true;
  }

  // Marker position in BV samples (1 kHz frames), 1-based recommended.
  bool addMarker(const QString& type,const QString& description,uint64_t positionSamples,QString* err=nullptr) {
   if (!m_open) return setErr(err,"addMarker called while not open.");

   const uint64_t pos=(positionSamples<1) ? 1:positionSamples;
   const uint64_t mkN=++m_markerCount;

   const QString line=QString("Mk%1=%2,%3,%4,1,0\r\n")
                       .arg(mkN)
                       .arg(sanitize(type))
                       .arg(sanitize(description))
                       .arg(QString::number(pos));

   const QByteArray utf8=line.toUtf8();
   return writeAll(m_fdVmrk,utf8.constData(),size_t(utf8.size()),err,"write marker failed");
  }

  bool stop(QString* err=nullptr) {
   if (!m_open) return true;

   // Finalize WAV header (must happen before fsync/close)
   (void)finalizeWavHeader(err);

   bool ok=true;
   ok &=fsyncFd(m_fdEeg,err,".eeg");
   ok &=fsyncFd(m_fdVmrk,err,".vmrk");
   ok &=fsyncFd(m_fdVhdr,err,".vhdr");
   ok &=fsyncFd(m_fdWav,err,".wav");

   closeFd(m_fdEeg); closeFd(m_fdVmrk); closeFd(m_fdVhdr); closeFd(m_fdWav);

   m_open=false;
   return ok;
  }

  void reset() {
   closeFd(m_fdEeg); closeFd(m_fdVmrk); closeFd(m_fdVhdr); closeFd(m_fdWav);

   m_fdEeg=m_fdVmrk=m_fdVhdr=m_fdWav=-1;
   m_open=false;

   m_bvFramesWritten=m_markerCount=m_wavFramesWritten=m_wavDataBytes=m_nChBV=0;
   m_base.clear(); m_vhdrPath.clear(); m_eegPath.clear(); m_vmrkPath.clear(); m_wavPath.clear();

   m_bvMeta=BrainVisionMeta{}; m_wavMeta=WavMeta{};
  }

 private:
  bool failSetup(QString* err,const QString& msg) {
   const QString full=msg+QString(" (errno=%1: %2)").arg(errno).arg(QString::fromLocal8Bit(::strerror(errno)));
   if (err) *err=full;
   reset();
   return false;
  }

  static bool setErr(QString* err,const QString& msg) {
   if (err) *err=msg;
   return false;
  }

  static bool setErrno(QString* err,const QString& msg) {
   const QString full=msg+QString(" (errno=%1: %2)").arg(errno).arg(QString::fromLocal8Bit(::strerror(errno)));
   if (err) *err=full;
   return false;
  }

  static QString sanitize(QString s) {
   s.replace("\r"," ");
   s.replace("\n"," ");
   s.replace(","," ");
   return s.trimmed();
  }

  static bool writeAll(int fd,const void* data,size_t n,QString* err,const char* what) {
   const uint8_t* p=static_cast<const uint8_t*>(data);
   size_t left=n;
   while (left) {
    const ssize_t w=::write(fd,p,left);
    if (w<0) {
     if (errno==EINTR) continue;
     if (err) {
      *err=QString("%1 (errno=%2: %3)").arg(what).arg(errno).arg(QString::fromLocal8Bit(::strerror(errno)));
     }
     return false;
    }
    p+=size_t(w);
    left-=size_t(w);
   }
   return true;
  }

  static bool fsyncFd(int fd,QString* err,const char* tag) {
   if (fd<0) return true;
   if (::fsync(fd)!=0) {
    if (err) {
     *err=QString("fsync %1 failed (errno=%2: %3)")
           .arg(tag).arg(errno).arg(QString::fromLocal8Bit(::strerror(errno)));
    }
    return false;
   }
   return true;
  }

  static void closeFd(int& fd) {
   if (fd>=0) {
    for (;;) {
     const int r=::close(fd);
     if (r==0) break;
     if (errno==EINTR) continue;
     break;
    }
    fd=-1;
   }
  }

  // ---------------- BrainVision headers ----------------
  bool writeVmrkHeader(QString* err) {
   const QString eegName=QFileInfo(m_eegPath).fileName();
   const QString hdr="Brain Vision Data Exchange Marker File, Version 1.0\r\n"
                      "\r\n"
                      "[Common Infos]\r\n"
                      "DataFile="+eegName+"\r\n"
                      "\r\n"
                      "[Marker Infos]\r\n"
                      "Mk1=New Segment,,1,1,0\r\n";
   m_markerCount=1;
   const QByteArray utf8=hdr.toUtf8();
   return writeAll(m_fdVmrk,utf8.constData(),size_t(utf8.size()),err,"write .vmrk header failed");
  }

  bool writeVhdr(QString* err) {
   const QString eegName=QFileInfo(m_eegPath).fileName();
   const QString vmrkName=QFileInfo(m_vmrkPath).fileName();

   const double samplingIntervalUs=1e6/m_bvMeta.sampleRateHz;

   QString out;
   out+="Brain Vision Data Exchange Header File Version 1.0\r\n";
   out+="\r\n";
   out+="[Common Infos]\r\n";
   out+="DataFile="+eegName+"\r\n";
   out+="MarkerFile="+vmrkName+"\r\n";
   out+="DataFormat=BINARY\r\n";
   out+="DataOrientation=MULTIPLEXED\r\n";
   out+="NumberOfChannels="+QString::number(m_nChBV)+"\r\n";
   out+="SamplingInterval="+QString::number(samplingIntervalUs,'f',6)+"\r\n";
   out+="\r\n";
   out+="[Binary Infos]\r\n";
   out+="BinaryFormat=IEEE_FLOAT_32\r\n";
   out+="UseBigEndianOrder=NO\r\n";
   out+="\r\n";
   out+="[Channel Infos]\r\n";

   for (int i=0;i<m_nChBV;++i) {
    const int chNum=i+1;
    const QString chName=sanitize(m_bvMeta.channelNames.at(i));
    out+=QString("Ch%1=%2,,%3,%4\r\n")
          .arg(chNum)
          .arg(chName)
          .arg(QString::number(m_bvMeta.channelResolution,'g',12))
          .arg(sanitize(m_bvMeta.unit));
   }

   const QByteArray utf8=out.toUtf8();
   return writeAll(m_fdVhdr,utf8.constData(),size_t(utf8.size()),err,"write .vhdr failed");
  }

  // ---------------- WAV header + finalize ----------------
  // Standard 44-byte PCM header, written once at start then patched on stop.
  bool writeWavHeaderPlaceholder(QString* err) {
   // We write a placeholder header with 0 sizes, then patch.
   const uint16_t audioFormat=1; // PCM
   const uint16_t numCh=uint16_t(m_wavMeta.numChannels);
   const uint32_t sampleRate=uint32_t(m_wavMeta.sampleRate);
   const uint16_t bitsPerSample=16;
   const uint16_t blockAlign=uint16_t(numCh*(bitsPerSample/8));
   const uint32_t byteRate=sampleRate*uint32_t(blockAlign);

   uint8_t h[44];
   std::memset(h,0,sizeof(h));

   auto put4=[&](int off,const char* s) { std::memcpy(h+off,s,4); };
   auto put32=[&](int off,uint32_t v) { std::memcpy(h+off,&v,4); };
   auto put16=[&](int off,uint16_t v) { std::memcpy(h+off,&v,2); };

   put4(0,"RIFF");
   put32(4,36);     // placeholder: 36+dataBytes
   put4(8,"WAVE");

   put4(12,"fmt ");
   put32(16,16);    // PCM fmt chunk size
   put16(20,audioFormat);
   put16(22,numCh);
   put32(24,sampleRate);
   put32(28,byteRate);
   put16(32,blockAlign);
   put16(34,bitsPerSample);

   put4(36,"data");
   put32(40,0);     // placeholder dataBytes

   return writeAll(m_fdWav,h,sizeof(h),err,"write wav header failed");
  }

  bool finalizeWavHeader(QString* err) {
   if (m_fdWav<0) return true;

   const uint32_t dataBytes=(m_wavDataBytes>0xFFFFFFFFull) ? 0xFFFFFFFFu:uint32_t(m_wavDataBytes);
   const uint32_t riffSize=(dataBytes>0xFFFFFFFFu-36u) ? 0xFFFFFFFFu:(36u+dataBytes);

   // Patch RIFF chunk size at 4, data chunk size at 40
   if (::lseek(m_fdWav,4,SEEK_SET)<0) return setErrno(err,"lseek wav(4) failed");
   if (!writeAll(m_fdWav,&riffSize,4,err,"patch RIFF size failed")) return false;

   if (::lseek(m_fdWav,40,SEEK_SET)<0) return setErrno(err,"lseek wav(40) failed");
   if (!writeAll(m_fdWav,&dataBytes,4,err,"patch data size failed")) return false;

   ::lseek(m_fdWav,0,SEEK_END);
   return true;
  }

 private:
  BrainVisionMeta m_bvMeta; WavMeta m_wavMeta;
  int m_nChBV=0; QString m_base,m_vhdrPath,m_eegPath,m_vmrkPath,m_wavPath;
  int m_fdEeg=-1; int m_fdVhdr=-1; int m_fdVmrk=-1; int m_fdWav=-1;
  bool m_open=false;
  uint64_t m_bvFramesWritten=0; uint64_t m_markerCount=0;
  uint64_t m_wavFramesWritten=0; uint64_t m_wavDataBytes=0;
};
