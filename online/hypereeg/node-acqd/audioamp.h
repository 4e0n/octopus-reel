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

#include <alsa/asoundlib.h>
#include <atomic>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>

const unsigned AUDIO_SAMPLE_RATE=48000;
const unsigned AUDIO_NUM_CHANNELS=2;
const unsigned AUDIO_CBUF_SECONDS=10; // ~10s ring

struct AudioAmp {
 std::atomic<uint64_t> consFrame{0};  // absolute frame index the consumer will read next

 // Synchronization for “new audio available”
 std::mutex cvMx;
 std::condition_variable cvAvail;

 // In class AudioAmp (private or public section)
 bool audioOK=false;

 // --- Ring buffer ---
 std::vector<int16_t> audioRing;   // interleaved L,R
 unsigned bufSizeFrames=0;         // total frames in ring
 std::atomic<unsigned> bufHead{0}; // producer index in frames
 std::atomic<bool> run{false};

 // --- ALSA ---
 snd_pcm_t* handle=nullptr; // audioPCMHandle
 std::thread th;

 // --- Stats ---
 std::atomic<uint64_t> audioFrameCounter{0};
 std::atomic<uint64_t> underruns{0};

 // --- Trigger detection state ---
 float trigHigh=8000.f;
 float trigLow=4000.f;
 unsigned trigMinGap=2400; // 50 ms @ 48 kHz
 bool trigState=false;
 uint64_t lastTrigFrame=0;

 // Init ring
 void init() {
  bufSizeFrames=AUDIO_CBUF_SECONDS*AUDIO_SAMPLE_RATE;
  audioRing.resize(size_t(bufSizeFrames)*AUDIO_NUM_CHANNELS);
  bufHead.store(0);
 }

bool initAlsa() {
    // Open device (blocking capture)
    int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> open failed:" << snd_strerror(rc); return false; }

    // ---- HW params ----
    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, AUDIO_NUM_CHANNELS);

    // Disable kernel resampler; try to force exact 48k
    snd_pcm_hw_params_set_rate_resample(handle, params, 0);
    unsigned rate = AUDIO_SAMPLE_RATE;
    rc = snd_pcm_hw_params_set_rate(handle, params, rate, 0);
    if (rc < 0) {
        qWarning() << "octopus_hacqd: <ALSAAudioInit> set_rate exact failed, trying near:" << snd_strerror(rc);
        rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, nullptr);
        if (rc < 0) {
            qWarning() << "octopus_hacqd: <ALSAAudioInit> set_rate_near failed:" << snd_strerror(rc);
        }
    }

    // Try HARD low-latency: 1 ms period, 4 ms buffer
    snd_pcm_uframes_t period  = 48;
    snd_pcm_uframes_t bufsize = period * 4;

    rc = snd_pcm_hw_params_set_period_size(handle, params, period, 0);
    if (rc < 0) {
        qWarning() << "octopus_hacqd: <ALSAAudioInit> set_period_size exact failed, trying near:" << snd_strerror(rc);
        rc = snd_pcm_hw_params_set_period_size_near(handle, params, &period, nullptr);
        if (rc < 0) {
            qWarning() << "octopus_hacqd: <ALSAAudioInit> set_period_size_near failed:" << snd_strerror(rc);
        }
        bufsize = period * 4; // keep 4×period relationship
    }

    rc = snd_pcm_hw_params_set_buffer_size(handle, params, bufsize);
    if (rc < 0) {
        qWarning() << "octopus_hacqd: <ALSAAudioInit> set_buffer_size exact failed, trying near:" << snd_strerror(rc);
        rc = snd_pcm_hw_params_set_buffer_size_near(handle, params, &bufsize);
        if (rc < 0) {
            qWarning() << "octopus_hacqd: <ALSAAudioInit> set_buffer_size_near failed:" << snd_strerror(rc);
        }
    }

    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> hw_params commit failed:" << snd_strerror(rc); return false; }

    // ---- SW params: start ASAP, wake each period ----
    snd_pcm_sw_params_t* sw = nullptr;
    snd_pcm_sw_params_malloc(&sw);
    snd_pcm_sw_params_current(handle, sw);

    // Wake when at least one period is available
    snd_pcm_sw_params_set_avail_min(handle, sw, period);
    // Start as soon as any data arrives (minimize initial latency)
    snd_pcm_sw_params_set_start_threshold(handle, sw, 1);

    rc = snd_pcm_sw_params(handle, sw);
    snd_pcm_sw_params_free(sw);
    if (rc < 0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> sw_params commit failed:" << snd_strerror(rc); return false; }

    // Prepare PCM
    rc = snd_pcm_prepare(handle);
    if (rc < 0) { qWarning() << "octopus_hacqd: <ALSAAudioInit> prepare failed:" << snd_strerror(rc); return false; }

    // Report negotiated actuals
    snd_pcm_hw_params_get_rate(params, &rate, 0);
    snd_pcm_hw_params_get_period_size(params, &period, 0);
    snd_pcm_hw_params_get_buffer_size(params, &bufsize);

    const double latency_ms = 1000.0 * double(bufsize) / double(rate);
    qInfo() << "octopus_hacqd: <ALSAAudioInit> ready. rate=" << rate
            << "Hz period=" << (unsigned)period
            << "frames buffer=" << (unsigned)bufsize
            << "frames (~" << latency_ms << "ms)";

    audioOK = true;
    return true;
}

/*

 bool initAlsa(const char* dev="default") {
  int rc=snd_pcm_open(&handle,dev,SND_PCM_STREAM_CAPTURE,0);
  if (rc<0) { std::fprintf(stderr,"[AudioAmp] open failed: %s\n",snd_strerror(rc)); return false; }

  snd_pcm_hw_params_t* p; snd_pcm_hw_params_alloca(&p);
  snd_pcm_hw_params_any(handle,p);
  snd_pcm_hw_params_set_access(handle,p,SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle,p,SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(handle,p,AUDIO_NUM_CHANNELS);

  unsigned rate=AUDIO_SAMPLE_RATE;
  //snd_pcm_hw_params_set_rate_near(handle,p,&rate,0);
  rc=snd_pcm_hw_params_set_rate(handle,p,rate,0);

  // Force period = 48 (1 ms), buffer=period*4=192 (4ms)
  snd_pcm_uframes_t period=48;
  //snd_pcm_uframes_t period=240;
  snd_pcm_uframes_t buf=period*4;

  rc=snd_pcm_hw_params_set_rate_resample(handle,p,0); // no resample
  //snd_pcm_hw_params_set_period_size_near(handle,p,&period,nullptr);
  //snd_pcm_hw_params_set_buffer_size_near(handle,p,&buf);
  // OR stricter:
  snd_pcm_hw_params_set_period_size(handle,p,period,0);
  snd_pcm_hw_params_set_buffer_size(handle,p,buf);

  rc=snd_pcm_hw_params(handle,p);
  if (rc<0) { std::fprintf(stderr,"[AudioAmp] hw_params: %s\n", snd_strerror(rc)); return false; }

  snd_pcm_prepare(handle);
  std::fprintf(stderr,"[AudioAmp] ready, period=%lu, buffer=%lu\n",(unsigned long)period,(unsigned long)buf);
  return true;
 }
*/
 void start() {
  if (!handle) return;
  run.store(true);
  th=std::thread([this]{ this->captureLoop(); });
 }

 void stop() {
  run.store(false);
  if (th.joinable()) th.join();
  if (handle) { snd_pcm_drop(handle); snd_pcm_close(handle); handle=nullptr; }
 }

 // Call from acqThread just before the 1 kHz loop starts
 void alignConsumerToProducerNow() {
   consFrame.store(audioFrameCounter.load(std::memory_order_acquire),std::memory_order_release);
 }

// Fills dst48 with 48 normalized LEFT samples; returns trigger offset [0..47] or UINT_MAX.
// Blocks up to wait_ms if producer hasn't delivered enough; on timeout, zero-pads and returns UINT_MAX.
// Consumer cursor NEVER advances unless 48 real frames were read.
unsigned fetch48(float* dst48, int wait_ms = 20) {
    constexpr unsigned NEED = 48;

    for (;;) {
        const uint64_t prod = audioFrameCounter.load(std::memory_order_acquire);
        const uint64_t cur  = consFrame.load(std::memory_order_relaxed);

        if (prod >= cur + NEED) {
            // Enough frames → consume exactly 48
            const unsigned start = unsigned(cur % bufSizeFrames);
            const unsigned toEnd = bufSizeFrames - start;
            const unsigned first = std::min(NEED, toEnd);

            auto i16_to_f32 = [](int16_t s)->float {
                return (s == INT16_MIN) ? -1.0f : float(s) / 32767.0f;
            };

            // LEFT → dst48
            for (unsigned i = 0; i < first; ++i)
                dst48[i] = i16_to_f32(audioRing[(size_t(start + i) * AUDIO_NUM_CHANNELS) + 0]);
            for (unsigned i = 0; i < NEED - first; ++i)
                dst48[first + i] = i16_to_f32(audioRing[(size_t(i) * AUDIO_NUM_CHANNELS) + 0]);

            // RIGHT → trigger detection (hysteresis + min-gap)
            unsigned trigOff = UINT_MAX;
            for (unsigned i = 0; i < NEED; ++i) {
                const unsigned idx = ((start + i) % bufSizeFrames) * AUDIO_NUM_CHANNELS + 1; // RIGHT
                const float v = float(audioRing[idx]); // raw int16 domain
                const bool high = (v >= trigHigh);
                const bool low  = (v <= trigLow);
                if (!trigState && high) {
                    const uint64_t absFrame = cur + i;
                    if (absFrame - lastTrigFrame >= trigMinGap) {
                        trigOff = i;
                        lastTrigFrame = absFrame;
                    }
                    trigState = true;
                } else if (trigState && low) {
                    trigState = false;
                }
            }

            // Advance only on success
            consFrame.store(cur + NEED, std::memory_order_release);
            return trigOff;
        }

        // Not enough yet → block up to wait_ms (bounded)
        if (wait_ms <= 0) {
            // No waiting requested: pad and return
            std::fill(dst48, dst48 + NEED, 0.0f);
            underruns.fetch_add(1, std::memory_order_relaxed);
            return UINT_MAX;
        }

        std::unique_lock<std::mutex> lk(cvMx);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(wait_ms);
        // Predicate re-checks prod vs cur to handle spurious wakeups
        cvAvail.wait_until(lk, deadline, [&]{
            return audioFrameCounter.load(std::memory_order_acquire) >=
                   consFrame.load(std::memory_order_relaxed) + NEED;
        });

        // loop back and try again; if still short, we’ll timeout and pad
        if (std::chrono::steady_clock::now() >= deadline) {
            std::fill(dst48, dst48 + NEED, 0.0f);
            underruns.fetch_add(1, std::memory_order_relaxed);
            return UINT_MAX; // consumer cursor NOT advanced
        }
    }
}

private:
 void captureLoop() {
  // try to raise RT priority (best effort)
#ifdef __linux__
  struct sched_param sp{30};
  pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp);
#endif
  // ALSA period detection
  snd_pcm_hw_params_t* p; snd_pcm_hw_params_alloca(&p);
  snd_pcm_hw_params_current(handle,p);
  snd_pcm_uframes_t period=0; snd_pcm_hw_params_get_period_size(p,&period,0);

  std::vector<int16_t> buf(size_t(period)*AUDIO_NUM_CHANNELS);

  while (run.load()) {
   int r=snd_pcm_readi(handle,buf.data(),period);
   if (r==-EPIPE) { snd_pcm_prepare(handle); continue; }
    if (r<0) { std::fprintf(stderr,"[AudioAmp] read: %s\n",snd_strerror(r)); continue; }
    // copy into ring
    unsigned head=bufHead.load();
    unsigned spaceToEnd=bufSizeFrames-head;
    unsigned first=std::min<unsigned>(r,spaceToEnd);
    std::memcpy(&audioRing[size_t(head)*AUDIO_NUM_CHANNELS],buf.data(),size_t(first)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
    if (r>(int)first) std::memcpy(&audioRing[0],buf.data()+size_t(first)*AUDIO_NUM_CHANNELS,
                                           size_t(r-first)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
    audioFrameCounter.fetch_add(r,std::memory_order_relaxed);
    bufHead.store((head+r)%bufSizeFrames,std::memory_order_release);
    // notify consumers waiting for more frames
    {
     std::lock_guard<std::mutex> lk(cvMx);
     cvAvail.notify_all();
    }
   }
  }
};

#endif
