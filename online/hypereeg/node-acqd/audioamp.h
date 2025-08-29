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
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <QtGlobal>

#define DEBUG_AUDIOAMP 1

const unsigned AUDIO_SAMPLE_RATE=48000;
const unsigned AUDIO_NUM_CHANNELS=2;
const unsigned AUDIO_CBUF_SECONDS=10; // ~10s ring

inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct AudioAmp {
    // --- Ring buffer ---
    std::vector<int16_t> audioRing;
    unsigned bufSizeFrames=0;
    std::atomic<unsigned> bufHead{0};
    std::atomic<uint64_t> audioFrameCounter{0};
    std::atomic<uint64_t> consFrame{0};

    // --- Control ---
    std::atomic<bool> run{false};
    std::thread th;
    std::mutex cvMx;
    std::condition_variable cvAvail;
    bool audioOK=false;

    // --- ALSA ---
    snd_pcm_t* handle=nullptr;

    // --- Trigger detection ---
    float trigHigh=8000.f;
    float trigLow=4000.f;
    unsigned trigMinGap=2400; // 50ms @ 48kHz
    bool trigState=false;
    uint64_t lastTrigFrame=0;

    // --- Resampling PLL ---
    double   rs_srcPos   = 0.0;
    double   rs_srcStep  = 1.0;
    double   rs_srcStepLP= 1.0;
    uint64_t rs_lastProd = 0;
    uint64_t rs_last_ns  = 0;
    bool     rs_inited   = false;

    bool     rs_locked   = false;
    bool     rs_waitSync = false;
    uint64_t rs_syncTick = 0;

    // Init ring
    void init() {
        bufSizeFrames=AUDIO_CBUF_SECONDS*AUDIO_SAMPLE_RATE;
        audioRing.resize(size_t(bufSizeFrames)*AUDIO_NUM_CHANNELS);
        bufHead.store(0);
    }

    bool initAlsa() {
        int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
        if (rc < 0) {
            qWarning() << "[AudioAmp] ALSA open failed:" << snd_strerror(rc);
            return false;
        }

        snd_pcm_hw_params_t* params=nullptr;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(handle, params);
        snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(handle, params, AUDIO_NUM_CHANNELS);

        unsigned rate=AUDIO_SAMPLE_RATE;
        snd_pcm_hw_params_set_rate_resample(handle, params, 0);
        rc=snd_pcm_hw_params_set_rate(handle, params, rate, 0);
        if (rc<0) {
            qWarning() << "[AudioAmp] set_rate exact failed:" << snd_strerror(rc);
            snd_pcm_hw_params_set_rate_near(handle, params, &rate, nullptr);
        }

        snd_pcm_uframes_t period=48;
        snd_pcm_uframes_t bufsize=period*4;
        snd_pcm_hw_params_set_period_size_near(handle, params, &period, nullptr);
        snd_pcm_hw_params_set_buffer_size_near(handle, params, &bufsize);

        rc=snd_pcm_hw_params(handle, params);
        if (rc<0) {
            qWarning() << "[AudioAmp] hw_params commit failed:" << snd_strerror(rc);
            return false;
        }

        snd_pcm_prepare(handle);

        snd_pcm_hw_params_get_rate(params, &rate, 0);
        snd_pcm_hw_params_get_period_size(params, &period, 0);
        snd_pcm_hw_params_get_buffer_size(params, &bufsize);

        double latency_ms = 1000.0*double(bufsize)/double(rate);
        qInfo() << "[AudioAmp] ready. rate="<<rate<<"Hz period="<<(unsigned)period
                <<" buf="<<(unsigned)bufsize<<" (~"<<latency_ms<<"ms)";
        audioOK=true;
        return true;
    }

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

    void resamplerInit() {
        rs_lastProd=audioFrameCounter.load(std::memory_order_acquire);
        rs_last_ns =now_ns();
        rs_srcPos  =double(rs_lastProd);
        rs_srcStepLP=1.0;
        rs_inited  =true;
    }

    unsigned fetch48(float* dst48,int wait_ms=20,uint64_t eegTickNow=0) {
        constexpr unsigned NEED=48;
        if (!rs_inited) resamplerInit();

        // --- update Fs estimate every ~100ms ---
        uint64_t ns=now_ns();
        uint64_t prod=audioFrameCounter.load(std::memory_order_acquire);
        uint64_t d_ns=ns-rs_last_ns;
        if (d_ns>=100'000'000ull) {
            uint64_t d_f=prod-rs_lastProd;
            if (d_f>0) {
                double fs_meas=double(d_f)/(double(d_ns)*1e-9);
                double step_new=fs_meas/48000.0;
                double alpha=0.05;
                rs_srcStepLP=(1.0-alpha)*rs_srcStepLP+alpha*step_new;
                rs_srcStep=rs_srcStepLP;
#if DEBUG_AUDIOAMP
                qInfo() << "[PLL]"<<"fs_meas="<<fs_meas<<" step_new="<<step_new
                        <<" stepLP="<<rs_srcStepLP;
#endif
            }
            rs_last_ns=ns;
            rs_lastProd=prod;
        }

        double needSpan=rs_srcStep*(NEED-1)+1.0;
        uint64_t needUpTo=(uint64_t)std::ceil(rs_srcPos+needSpan);

        auto enough=[&]{return audioFrameCounter.load(std::memory_order_acquire)>=needUpTo;};
        if (!enough()) {
            std::unique_lock<std::mutex> lk(cvMx);
            cvAvail.wait_for(lk,std::chrono::milliseconds(wait_ms),enough);
            if (!enough()) {
                std::fill(dst48,dst48+NEED,0.0f);
                return UINT_MAX;
            }
        }

        auto sampleLR=[&](double absPos,int chan)->float {
            double ringPos=std::fmod(absPos,double(bufSizeFrames));
            int i0=int(ringPos);
            int i1=(i0+1==int(bufSizeFrames))?0:(i0+1);
            float frac=float(ringPos-double(i0));
            int idx0=i0*AUDIO_NUM_CHANNELS+chan;
            int idx1=i1*AUDIO_NUM_CHANNELS+chan;
            int16_t s0=audioRing[idx0];
            int16_t s1=audioRing[idx1];
            float v0=(s0==INT16_MIN)?-1.0f:float(s0)/32767.0f;
            float v1=(s1==INT16_MIN)?-1.0f:float(s1)/32767.0f;
            return v0+frac*(v1-v0);
        };

        unsigned trigOff=UINT_MAX;
        double pos=rs_srcPos;
        for (unsigned i=0;i<NEED;i++,pos+=rs_srcStep) {
            dst48[i]=sampleLR(pos,0);
            if (trigOff==UINT_MAX) {
                int idx=(int(std::floor(pos))%bufSizeFrames)*AUDIO_NUM_CHANNELS+1;
                int16_t r=audioRing[idx];
                if (!trigState && r>=int16_t(trigHigh)) {
                    if ((audioFrameCounter.load()-lastTrigFrame)>=trigMinGap) {
                        trigOff=i;
                        lastTrigFrame=audioFrameCounter.load();
                    }
                    trigState=true;
                } else if (trigState && r<=int16_t(trigLow)) {
                    trigState=false;
                }
            }
        }
        rs_srcPos=pos;

        // PLL nudge at sync
        if (!rs_locked && rs_waitSync && trigOff!=UINT_MAX) {
            double err_samples=double(trigOff);
            double err_frames=err_samples*rs_srcStep;
            double Kp=0.5,Ki=0.001;
            rs_srcPos-=Kp*err_frames;
            rs_srcStep*=(1.0-Ki*(err_samples/48.0));
            rs_locked=true;
            rs_waitSync=false;
#if DEBUG_AUDIOAMP
            qInfo() << "[PLL] lock. err_samples="<<err_samples<<" new_step="<<rs_srcStep;
#endif
        }
        return trigOff;
    }

private:
    void captureLoop() {
        snd_pcm_hw_params_t* p; snd_pcm_hw_params_alloca(&p);
        snd_pcm_hw_params_current(handle,p);
        snd_pcm_uframes_t period=0; snd_pcm_hw_params_get_period_size(p,&period,0);
        std::vector<int16_t> buf(size_t(period)*AUDIO_NUM_CHANNELS);

        uint64_t lastPrint=0;
        while (run.load()) {
            int r=snd_pcm_readi(handle,buf.data(),period);
            if (r==-EPIPE) { snd_pcm_prepare(handle); continue; }
            if (r<0) { continue; }

            unsigned head=bufHead.load();
            unsigned spaceToEnd=bufSizeFrames-head;
            unsigned first=std::min<unsigned>(r,spaceToEnd);
            std::memcpy(&audioRing[size_t(head)*AUDIO_NUM_CHANNELS],
                        buf.data(),size_t(first)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
            if (r>(int)first)
                std::memcpy(&audioRing[0],buf.data()+size_t(first)*AUDIO_NUM_CHANNELS,
                            size_t(r-first)*AUDIO_NUM_CHANNELS*sizeof(int16_t));
            audioFrameCounter.fetch_add(r,std::memory_order_relaxed);
            bufHead.store((head+r)%bufSizeFrames,std::memory_order_release);

            {
                std::lock_guard<std::mutex> lk(cvMx);
                cvAvail.notify_all();
            }

#if DEBUG_AUDIOAMP
            uint64_t nowTick=audioFrameCounter.load();
            if (nowTick/48000 != lastPrint/48000) { // once per ~sec
                qInfo() << "[AudioAmp]"<<"prod="<<nowTick<<"cons="<<consFrame.load();
            }
            lastPrint=nowTick;
#endif
        }
    }
};

#endif
