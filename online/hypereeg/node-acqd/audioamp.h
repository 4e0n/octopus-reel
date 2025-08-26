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

#ifndef _AUDIOAMP_H
#define _AUDIOAMP_H

#include <cstring>
#include <iostream>
#include <deque>
#include <alsa/asoundlib.h>
#include "confparam.h"

const unsigned int AUDIO_SAMPLE_RATE=48000;
const unsigned int AUDIO_NUM_CHANNELS=2;
const unsigned int AUDIO_CBUF_SIZE=10; // 10 seconds -- 4800 to read correct it

struct AudioTrig { uint64_t frame; };

struct AudioAmp {
 std::vector<int16_t> audioRing; // interleaved L,R
 unsigned bufSize; quint64 bufTail,bufHead; bool audioOK;

 // Alsa Audio
 snd_pcm_t* audioPCMHandle; snd_pcm_hw_params_t* audioParams; std::vector<int16_t> audioBuffer;
 uint64_t audioFrameCounter=0; // total frames captured (for sync math)

 ConfParam *conf; unsigned int audioIntBufSize; int ii;

 float trigHigh=8000.0f;   // ~25% FS for int16, tune
 float trigLow=4000.0f;    // hysteresis
 unsigned trigMinGap=2400; // 50 ms @ 48 kHz
 bool trigState=false;
 uint64_t lastTrigFrame=0; // absolute frame index (audioFrameCounter-based)

 std::deque<AudioTrig> trigQueue; // Optional queue to pass triggers upstream:

 // --- Envelope ring (preallocated, no per-tick allocs) ---
 static constexpr size_t ENV_QMAX=8;      // keep last 8 ticks (~0.8 s @ 100 ms)
 std::vector<std::vector<float>> envRing; // [ENV_QMAX][N]
 size_t envHead=0;                        // next slot to write
 size_t envCount=0;                       // valid slots (<= ENV_QMAX)

 // Scratch buffer reused for 48k->EEG downsample (avoid allocs)
 std::vector<int16_t> scratchInter;       // size = framesNeeded * AUDIO_NUM_CHANNELS

 AudioAmp() {};
 AudioAmp(ConfParam *c) { init(c); }

 void init(ConfParam *c) { conf=c;
  audioIntBufSize=static_cast<unsigned>( // frames we want to read per EEG tick (e.g., 100ms)
   std::lround(AUDIO_SAMPLE_RATE*(conf->eegProbeMsecs/float(conf->eegRate)))
  ); // e.g., 48000 * 0.1 = 4800

  // Preallocate envelope ring slots: N: EEG samples per tick
  const unsigned N=c->eegSamplesInTick; // compute once before calling init()
  envRing.assign(ENV_QMAX,std::vector<float>(N,0.f));
  envHead=0; envCount=0;

  audioBuffer.resize(audioIntBufSize*AUDIO_NUM_CHANNELS); // working read buffer (interleaved stereo)
  // ring buffer: N seconds of audio (frames*channels)
  bufSize=AUDIO_CBUF_SIZE*AUDIO_SAMPLE_RATE; // frames
  audioRing.resize(bufSize*AUDIO_NUM_CHANNELS); // samples
  bufHead=bufTail=0; audioOK=false; ii=0;
 }

 void initAlsa() {
  int rc=snd_pcm_open(&audioPCMHandle,"default",SND_PCM_STREAM_CAPTURE,0); // Open ALSA capture device
  if (rc<0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> ERROR: open failed:" << snd_strerror(rc); return; }

  snd_pcm_hw_params_t* params=nullptr;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(audioPCMHandle,params);

  snd_pcm_hw_params_set_access(audioPCMHandle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(audioPCMHandle,params,SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(audioPCMHandle,params,AUDIO_NUM_CHANNELS);

  unsigned rate=AUDIO_SAMPLE_RATE;
  rc=snd_pcm_hw_params_set_rate_near(audioPCMHandle,params,&rate,nullptr);
  if (rc<0 || rate!=AUDIO_SAMPLE_RATE) {
   qWarning() << "octopus_hacqd: <ALSAAudioInit> ERROR: rate not set to 48000:" << rate << snd_strerror(rc);
  }

  // period = frames per EEG tick (e.g., 4800), buffer = 4 periods
  snd_pcm_uframes_t period=audioIntBufSize; snd_pcm_uframes_t bufsize=period*4;

  snd_pcm_hw_params_set_period_size_near(audioPCMHandle,params,&period,nullptr);
  snd_pcm_hw_params_set_buffer_size_near(audioPCMHandle,params,&bufsize);

  rc=snd_pcm_hw_params(audioPCMHandle,params);
  if (rc<0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> ERROR: hw_params fail:" << snd_strerror(rc); return; }

  // Start clean
  snd_pcm_prepare(audioPCMHandle); audioOK=true;
  qInfo() << "octopus_hacqd: <ALSAAudioInit> capture ready. period=" << (unsigned)period
          << " frames, buffer=" << (unsigned)bufsize << " frames";
 }

 void instreamAudio() { // Call regularly within main loopthread
  if (!audioOK) return;

  const int framesWanted=int(audioIntBufSize); int framesRead=0;

  while (framesRead<framesWanted) {
   int rc=snd_pcm_readi(audioPCMHandle,audioBuffer.data()+(framesRead*AUDIO_NUM_CHANNELS),framesWanted-framesRead);
   if (rc==-EPIPE) {
    qWarning() << "octopus_hacqd: <ALSAAudioStream> ERROR: ALSA overrun; recovering.";
    snd_pcm_prepare(audioPCMHandle);
    continue; // retry
   } else if (rc < 0) {
     qWarning() << "octopus_hacqd: <ALSAAudioStream> ERROR: ALSA read error:" << snd_strerror(rc);
     return;
   }
   framesRead+=rc;
  }

  // 1) write to ring (existing code, but into audioRing)
  // write to ring buffer (interleaved) - buffer holds bufSize*AUDIO_NUM_CHANNELS of int16 samples
  unsigned headFrames=bufHead%bufSize;
  unsigned spaceToEnd=bufSize-headFrames;
  unsigned firstFrames=std::min<unsigned>(framesRead,spaceToEnd);
  unsigned remFrames=framesRead-firstFrames;
  // first chunk
  std::memcpy(&audioRing[size_t(headFrames)*AUDIO_NUM_CHANNELS],audioBuffer.data(),size_t(firstFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
  // wrap
  if (remFrames) std::memcpy(&audioRing[0],audioBuffer.data()+size_t(firstFrames)*AUDIO_NUM_CHANNELS,size_t(remFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));

  // 2) trigger decode on RIGHT channel of this block
  //    (index 1 in interleaved LRLR...)
  for (int i=0;i<framesRead;++i) {
   int16_t r=audioBuffer[size_t(i)*AUDIO_NUM_CHANNELS+1];
   float v=float(r); bool high=(v>=trigHigh); bool low=(v<=trigLow);
   if (!trigState && high) {
    uint64_t absFrame=audioFrameCounter+uint64_t(i);
    if (absFrame-lastTrigFrame >= trigMinGap) {
     trigQueue.push_back({absFrame});
     lastTrigFrame=absFrame;
    }
    trigState=true;
   } else if (trigState&&low) {
    trigState=false;
   }
  }

  // 3) advance counters/head
  audioFrameCounter+=uint64_t(framesRead);

  bufHead=(bufHead+framesRead)%bufSize;
 }

 void stopAudio() {
  if (!audioOK) return;
  snd_pcm_drain(audioPCMHandle); snd_pcm_close(audioPCMHandle);
  audioOK=false;
 }

// bufSize = frames in ring (not samples); bufHead = write index in frames

 // Copy the latest `frames` interleaved samples into `dst` (size = frames*channels)
 bool readLatestFrames(unsigned frames, std::vector<int16_t>& dst) const {
  if (!audioOK || frames > bufSize) return false;
  dst.resize(size_t(frames)*AUDIO_NUM_CHANNELS);
  unsigned head=bufHead; // current head in frames
  unsigned start=(head+bufSize-frames)%bufSize;

  unsigned toEnd=bufSize-start;
  unsigned firstFrames=std::min(frames,toEnd);
  unsigned remFrames=frames-firstFrames;

  std::memcpy(dst.data(),&audioRing[size_t(start)*AUDIO_NUM_CHANNELS],size_t(firstFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
  if (remFrames) {
   std::memcpy(dst.data()+size_t(firstFrames)*AUDIO_NUM_CHANNELS,&audioRing[0],size_t(remFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
  }
  return true;
 }

 // Fills 'dst' (already sized to frames*AUDIO_NUM_CHANNELS) with the newest frames
 bool readLatestFramesInto(unsigned frames,std::vector<int16_t>& dst) const {
  if (!audioOK || frames>bufSize) return false;
  if (dst.size()<size_t(frames)*AUDIO_NUM_CHANNELS) return false;

  const unsigned head=bufHead; // frames
  const unsigned start=(head+bufSize-frames)%bufSize;
  const unsigned toEnd=bufSize-start;
  const unsigned firstFrames=std::min(frames,toEnd);
  const unsigned remFrames=frames-firstFrames;

  std::memcpy(dst.data(),&audioRing[size_t(start)*AUDIO_NUM_CHANNELS],size_t(firstFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));

  if (remFrames) {
   std::memcpy(dst.data()+size_t(firstFrames)*AUDIO_NUM_CHANNELS,&audioRing[0],size_t(remFrames)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
  }
  return true;
 }

 // Helper: compute envelope for the *latest* EEG block (N eeg samples)
 bool buildAudioEnvelopeForEEGTick(unsigned eegRate,unsigned eegSamplesInTick,std::vector<float>& outEnv /* size N */) {
  if (!audioOK) return false;
  const unsigned framesPerEEG=(AUDIO_SAMPLE_RATE+eegRate/2)/eegRate; // round
  const unsigned needFrames=framesPerEEG*eegSamplesInTick;

  std::vector<int16_t> inter; if (!readLatestFrames(needFrames,inter)) return false;

  outEnv.assign(eegSamplesInTick,0.f);

  // walk newest chunk in time order
  for (unsigned n=0;n<eegSamplesInTick;++n) {
   uint64_t base=uint64_t(n)*framesPerEEG;
   double acc=0.0;
   for (unsigned k=0;k<framesPerEEG;++k) {
    int16_t L=inter[size_t(base+k)*AUDIO_NUM_CHANNELS+0];
    float x=float(L)/32768.0f; // normalize to [-1,1]
    acc+=double(x)*double(x);
   }
   outEnv[n]=float(std::sqrt(acc/double(framesPerEEG))); // RMS
  }
  return true;
 }

 // Fill the next envelope slot from the *latest* audio frames.
 // Returns true if a full N-sample envelope was written.
 bool pushEnvelopeTick(unsigned eegRate,unsigned N) {
  if (!audioOK) return false;

  // frames per EEG sample (rounded) and total frames needed this tick
  const unsigned framesPerEEG=(AUDIO_SAMPLE_RATE+eegRate/2)/eegRate; const unsigned needFrames=framesPerEEG*N;

  // Make sure scratch buffer is large enough (no alloc if already big enough)
  if (scratchInter.size()<size_t(needFrames)*AUDIO_NUM_CHANNELS) scratchInter.resize(size_t(needFrames)*AUDIO_NUM_CHANNELS);

  // Pull the newest 'needFrames' from the audio ring into scratchInter
  if (!readLatestFramesInto(needFrames,scratchInter)) return false;

  // Write directly into preallocated ring slot
  std::vector<float>& outEnv=envRing[envHead];
  if (outEnv.size()!=N) outEnv.assign(N,0.f); // one-time correction if N changed

  // Build RMS per EEG sample from LEFT channel
  for (unsigned n=0;n<N;++n) {
   const size_t base=size_t(n)*framesPerEEG;
   double acc=0.0;
   for (unsigned k=0;k<framesPerEEG;++k) {
    const int16_t L=scratchInter[(base+k)*AUDIO_NUM_CHANNELS+0];
    const float x=float(L)/32768.0f;
    acc+=double(x)*double(x);
   }
   outEnv[n]=float(std::sqrt(acc/double(framesPerEEG)));
  }

  // Advance ring
  envHead=(envHead+1)%ENV_QMAX;
  if (envCount<ENV_QMAX) ++envCount;
  return true;
 }

 // Access the latest (k=0) or k-th previous envelope. Returns nullptr if unavailable.
 const std::vector<float>* getEnvelopeBack(size_t k=0) const {
  if (envCount==0) return nullptr;
  if (k>=envCount) k=envCount-1;
  const size_t idx=(envHead+ENV_QMAX-1-k)%ENV_QMAX;
  return &envRing[idx];
 }
};

#endif
