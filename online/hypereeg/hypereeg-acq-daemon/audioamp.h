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

#include <iostream>
#include <alsa/asoundlib.h>
#include "audiosample.h"
#include "confparam.h"
//#include "iirfilter.h"

// [0.1-100Hz] Band-pass
//const std::vector<double> b={0.06734199,0.          ,-0.13468398,0.         ,0.06734199};
//const std::vector<double> a={1.        , -3.14315078, 3.69970174,-1.96971135,0.41316049};

// Notch@50Hz (Q=30)
//const std::vector<double> nb={0.99479124,-1.89220538,0.99479124};
//const std::vector<double> na={1.        ,-1.89220538,0.98958248};

const unsigned int AUDIO_SAMPLE_RATE=48000;
const unsigned int AUDIO_NUM_CHANNELS=2;
const unsigned int AUDIO_CBUF_SIZE=10; // 10 seconds -- 4800 to read correct it
//const int AUDIO_INTERNAL_BUF_SIZE=AUDIO_SAMPLE_RATE/10; // Is this the internal kernel buffer?
							  // Does it effect latency? (shorter the better, etc.?
							  // Is it channel# inclusive?

struct AudioAmp {
 std::vector<AudioSample> buffer; unsigned bufSize; quint64 bufTail,bufHead;

 AudioAmp() {};
 AudioAmp(ConfParam *c) { init(c); }

 void init(ConfParam *c) { conf=c;
  audioIntBufSize=AUDIO_SAMPLE_RATE/(conf->eegRate/conf->eegProbeMsecs);
  qDebug() << AUDIO_SAMPLE_RATE << conf->eegRate << audioIntBufSize;
  audioBuffer.resize(audioIntBufSize);

  buffer.resize(AUDIO_CBUF_SIZE*AUDIO_NUM_CHANNELS*AUDIO_SAMPLE_RATE); audioOK=false;
  for (auto& b:buffer) { AudioSample s; b=s; }
  ii=0;
 }

 void initAlsa() {
  if (snd_pcm_open(&audioPCMHandle,"default",SND_PCM_STREAM_CAPTURE,0)==0) { // Open ALSA capture device
   // Configure hardware parameters
   snd_pcm_hw_params_alloca(&audioParams);
   snd_pcm_hw_params_any(audioPCMHandle,audioParams);
   snd_pcm_hw_params_set_access(audioPCMHandle,audioParams,SND_PCM_ACCESS_RW_INTERLEAVED);
   snd_pcm_hw_params_set_format(audioPCMHandle,audioParams,SND_PCM_FORMAT_S16_LE);
   snd_pcm_hw_params_set_channels(audioPCMHandle,audioParams,AUDIO_NUM_CHANNELS);
   snd_pcm_hw_params_set_rate(audioPCMHandle,audioParams,AUDIO_SAMPLE_RATE,0);
   //snd_pcm_hw_params_set_buffer_size(audioPCMHandle,audioParams,audioIntBufSize); // by default set to 0.1 second.
   if (snd_pcm_hw_params(audioPCMHandle,audioParams)==0) audioOK=true;
   else qDebug() << "octopus_hacqd: ERROR!!! <AlsaAudioInit> while setting PCM parameters.";
  } else qDebug() << "octopus_hacqd: ERROR!!! <AlsaAudioInit> while opening PCM device.";
  if (audioOK) qInfo() << "octopus_hacqd: <AlsaAudioInit> Alsa Audio Input successfully activated, audio streaming started.";
 }

 void instreamAudio() { // Call regularly within main loopthread
//  int err=snd_pcm_readi(audioPCMHandle,audioBuffer.data(),audioIntBufSize);
//  if (err==-EPIPE) {
//   qWarning() << "octopus_hacqd: WARNING!!! <AlsaAudioStream> BUFFER OVERRUN!!! Recovering...";
//   snd_pcm_prepare(audioPCMHandle);
//  } else if (err<0) {
//   qWarning() << "octopus_hacqd: WARNING!!! <AlsaAudioStream> ERROR READING AUDIO! Err.No:" << snd_strerror(err);
//  } else { ;
   //qInfo() << "octopus_hacqd: <AlsaAudioStream> Instreamed" << audioIntBufSize << "frames (48kHz, Stereo)" << ii++;
   //for (unsigned int bufIdx=0;bufIdx<audioIntBufSize;bufIdx++) {
   // for (unsigned int sIdx=0;sIdx<conf->setCount;sIdx++) {
   //  buffer[(bufHead+bufIdx)%bufSize].data[0]=audioBuffer[bufIdx];
   // }
   //}
//  }
 }

 void stopAudio() {
  snd_pcm_drain(audioPCMHandle); // Clean up
  snd_pcm_close(audioPCMHandle);
  std::cout << "octopus_hacqd: <AlsaAudioStream> Instreaming stopped.\n";
 }

 bool audioOK;

 // Alsa Audio
 snd_pcm_t* audioPCMHandle; snd_pcm_hw_params_t* audioParams; std::vector<int16_t> audioBuffer;

 ConfParam *conf; unsigned int audioIntBufSize;
 int ii;
};

#endif
