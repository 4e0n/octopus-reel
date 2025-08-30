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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

#include <QDebug>

static constexpr unsigned AUDIO_SAMPLE_RATE   = 48000;
static constexpr unsigned AUDIO_NUM_CHANNELS  = 2;
static constexpr unsigned AUDIO_CBUF_SECONDS  = 10;

static inline uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

struct AudioAmp {
    // ring & counters
    std::atomic<uint64_t> audioFrameCounter{0};
    std::atomic<bool>     run{false};

    std::vector<int16_t>  audioRing;
    unsigned              bufSizeFrames = 0;
    std::atomic<unsigned> bufHead{0};

    std::mutex              cvMx;
    std::condition_variable cvAvail;

    snd_pcm_t*  handle = nullptr;
    std::thread th;

    std::atomic<uint64_t> underruns{0};

    // Trigger (RIGHT channel)
    float    trigHigh   = 8000.f;
    float    trigLow    = 4000.f;
    unsigned trigMinGap = 2400; // 50 ms @ 48 kHz
    uint64_t lastTrigFrame = 0;

    // Resampler / PLL / Servo
    bool     rs_inited     = false;
    double   rs_srcPos     = 0.0;    // fractional read position (input frames)
    double   rs_stepEst    = 1.0;    // Fs_in / 48k (smoothed)
    double   rs_stepCorr   = 1.0;    // servo correction
    double   rs_stepBias   = 0.0;    // slow bias (centers stepCorr≈1)
    uint64_t rs_lastProd   = 0;

    // PI state
    double   lagInt        = 0.0;

    void init() {
        bufSizeFrames = AUDIO_CBUF_SECONDS * AUDIO_SAMPLE_RATE;
        audioRing.resize(size_t(bufSizeFrames) * AUDIO_NUM_CHANNELS);
        bufHead.store(0, std::memory_order_relaxed);
    }

    bool initAlsa() {
        int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
        if (rc < 0) { qWarning() << "[AudioAmp] ALSA open failed:" << snd_strerror(rc); return false; }

        snd_pcm_hw_params_t* hw = nullptr;
        snd_pcm_hw_params_alloca(&hw);
        snd_pcm_hw_params_any(handle, hw);

        snd_pcm_hw_params_set_access(handle, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(handle, hw, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(handle, hw, AUDIO_NUM_CHANNELS);

        snd_pcm_hw_params_set_rate_resample(handle, hw, 0);
        unsigned rate = AUDIO_SAMPLE_RATE;
        rc = snd_pcm_hw_params_set_rate(handle, hw, rate, 0);
        if (rc < 0) {
            qWarning() << "[AudioAmp] set_rate exact failed, trying near:" << snd_strerror(rc);
            rc = snd_pcm_hw_params_set_rate_near(handle, hw, &rate, nullptr);
            if (rc < 0) qWarning() << "[AudioAmp] set_rate_near failed:" << snd_strerror(rc);
        }

        snd_pcm_uframes_t period  = 48;
        snd_pcm_uframes_t bufsize = period * 4;

        rc = snd_pcm_hw_params_set_period_size(handle, hw, period, 0);
        if (rc < 0) { rc = snd_pcm_hw_params_set_period_size_near(handle, hw, &period, nullptr); bufsize = period * 4; }
        rc = snd_pcm_hw_params_set_buffer_size(handle, hw, bufsize);
        if (rc < 0) { rc = snd_pcm_hw_params_set_buffer_size_near(handle, hw, &bufsize); }

        rc = snd_pcm_hw_params(handle, hw);
        if (rc < 0) { qWarning() << "[AudioAmp] hw_params commit failed:" << snd_strerror(rc); return false; }

        snd_pcm_sw_params_t* sw = nullptr;
        snd_pcm_sw_params_malloc(&sw);
        snd_pcm_sw_params_current(handle, sw);
        snd_pcm_sw_params_set_avail_min(handle, sw, period);
        snd_pcm_sw_params_set_start_threshold(handle, sw, 1);
        rc = snd_pcm_sw_params(handle, sw);
        snd_pcm_sw_params_free(sw);
        if (rc < 0) { qWarning() << "[AudioAmp] sw_params commit failed:" << snd_strerror(rc); return false; }

        rc = snd_pcm_prepare(handle);
        if (rc < 0) { qWarning() << "[AudioAmp] prepare failed:" << snd_strerror(rc); return false; }

        return true;
    }

    void start() {
        if (!handle) return;
        run.store(true, std::memory_order_release);
        th = std::thread([this]{ captureLoop(); });
    }

    void stop() {
        run.store(false, std::memory_order_release);
        if (th.joinable()) th.join();
        if (handle) { snd_pcm_drop(handle); snd_pcm_close(handle); handle = nullptr; }
    }

    unsigned fetch48(float* dst48, int wait_ms = 10) {
        constexpr unsigned OUT            = 48;
        constexpr unsigned SAFETY_MARGIN  = 96;
        constexpr unsigned LAG_SOFT_CLAMP = OUT + 16;
        constexpr int      UNDERRUN_ARM   = 3;
        constexpr int      STALL_ARM      = 3;

        // ---- Servo tuning ----
        constexpr double TARGET_LAG_FRAMES = 3.0 * OUT;   // 144 frames cushion
        constexpr double LAG_DEADBAND      = 12.0;        // ±12 frames

        // PI (very small)
        constexpr double Kp = 2.5e-5;
        constexpr double Ki = 2.5e-7;

        // back-calculation anti-windup gain (how strongly to back off Int at clamp)
        constexpr double Kaw = 8.0e-6;

        // Integral bleed
        constexpr double I_DECAY = 0.001;

        // base correction window and slew
        constexpr double BASE_MIN   = 0.990; // ±1%
        constexpr double BASE_MAX   = 1.010;
        constexpr double SLEW_PER_FETCH = 0.0015; // ±0.15% per 48 samples

        // Init on first call
        if (!rs_inited) {
            rs_lastProd  = audioFrameCounter.load(std::memory_order_acquire);
            rs_srcPos    = double(rs_lastProd);
            rs_stepEst   = 1.0;
            rs_stepCorr  = 1.0;
            rs_stepBias  = 0.0;
            lagInt       = 0.0;
            rs_inited    = true;
        }

        // ---------- PLL + PI every ~200ms ----------
        static uint64_t pll_ns_prev   = now_ns();
        static uint64_t pll_prod_prev = 0;
        static uint64_t pll_updates   = 0;
        static uint64_t pll_lastLog   = now_ns();

        // bias loop state (slow recentring of stepCorr toward 1)
        static uint64_t bias_ns_prev  = now_ns();
        static double   clampLean     = 0.0;  // low-pass of (are we leaning into clamp?)

        {
            const uint64_t now  = now_ns();
            const uint64_t prod = audioFrameCounter.load(std::memory_order_acquire);
            const uint64_t dNs  = now - pll_ns_prev;

            if (pll_prod_prev == 0) pll_prod_prev = prod;

            if (dNs >= 200'000'000ULL) {
                const uint64_t dF = prod - pll_prod_prev;
                if (dF > 0) {
                    const double fs_meas  = double(dF) / (double(dNs) * 1e-9);
                    const double step_new = fs_meas / 48000.0;

                    // PLL smoothing (moderately slow)
                    const double alpha    = 0.05;
                    rs_stepEst = (1.0 - alpha) * rs_stepEst + alpha * step_new;

                    // Lag computation
                    const uint64_t i0  = uint64_t(std::floor(rs_srcPos));
                    const uint64_t lag = (prod > i0) ? (prod - i0) : 0ULL;
                    double lagErr = double(lag) - TARGET_LAG_FRAMES;

                    // Deadband
                    if (std::abs(lagErr) < LAG_DEADBAND) lagErr = 0.0;

                    // Adaptive correction window
                    double cmin = BASE_MIN;
                    double cmax = BASE_MAX;
                    const double absLagBlocks = std::abs(lagErr) / double(OUT);
                    if (absLagBlocks > 6.0) { cmin = 0.950; cmax = 1.050; }
                    else if (absLagBlocks > 2.0) { cmin = 0.970; cmax = 1.030; }

                    // PI raw target
                    lagInt = (1.0 - I_DECAY) * lagInt + lagErr;
                    double corrTarget = 1.0 + (Kp * lagErr + Ki * lagInt);

                    // Slew-limit corr relative to previous
                    const double prevCorr = rs_stepCorr;
                    double corrSlew = std::clamp(corrTarget,
                                                 prevCorr - SLEW_PER_FETCH,
                                                 prevCorr + SLEW_PER_FETCH);

                    // Clamp to window and apply back-calculation anti-windup
                    double corrClamped = std::clamp(corrSlew, cmin, cmax);
                    if (corrClamped != corrSlew) {
                        // Back-calculation: drive integral so that (Kp*e + Ki*I) matches clamp
                        const double awErr = (corrSlew - corrClamped); // signed excess
                        lagInt -= (Kaw / std::max(Ki,1e-12)) * awErr;  // push Int toward feasible value
                    }
                    rs_stepCorr = corrClamped;

                    // ---- Slow bias loop (recenter toward corr=1.0) ----
                    // If we're persistently at/near clamp, nudge rs_stepBias so that
                    // rs_stepEst += rs_stepBias and corr comes back to ~1.
                    {
                        const double nearMin = (rs_stepCorr < (cmin + 0.001)) ? 1.0 : 0.0;
                        const double nearMax = (rs_stepCorr > (cmax - 0.001)) ? -1.0 : 0.0;
                        const double lean    = nearMin + nearMax; // +1 means want more corr headroom upward, -1 means downward
                        const double beta    = 0.02; // LPF for lean indicator
                        clampLean = (1.0 - beta) * clampLean + beta * lean;

                        // every ~1s, apply tiny bias if leaning
                        if (now - bias_ns_prev >= 1'000'000'000ULL) {
                            // bias step ~2e-5 per second at full lean => 0.002 after 100s
                            const double biasStep = 2.0e-5 * clampLean;
                            // limit total bias to something sane (±1%)
                            rs_stepBias = std::clamp(rs_stepBias + biasStep, -0.010, 0.010);
                            bias_ns_prev = now;
                        }
                    }

                    // Debug ~1 Hz
                    ++pll_updates;
                    if (now - pll_lastLog >= 1'000'000'000ULL) {
                        const double callHz = double(pll_updates) / (double(now - pll_lastLog) * 1e-9);
                        qInfo() << "[PLL]"
                                << "fs_meas="   << QString::number(fs_meas, 'f', 1)
                                << " stepEst="  << QString::number(rs_stepEst, 'f', 6)
                                << " callHz="   << QString::number(callHz, 'f', 1)
                                << " stepBias=" << QString::number(rs_stepBias, 'f', 6)
                                << " stepCorr=" << QString::number(rs_stepCorr, 'f', 6)
                                << " stepUse="  << QString::number((rs_stepEst+rs_stepBias)*rs_stepCorr, 'f', 6);
                        pll_updates = 0;
                        pll_lastLog = now;
                    }
                }
                pll_ns_prev   = now;
                pll_prod_prev = prod;
            }
        }

        const double stepUse = (rs_stepEst + rs_stepBias) * rs_stepCorr;

        // ----- Ensure enough input for interpolation -----
        const double   needEndPos = std::floor(rs_srcPos + stepUse * (OUT - 1)) + 1.0;
        const uint64_t needAbs    = (needEndPos < 0.0) ? 0ULL : uint64_t(needEndPos);

        auto have_enough = [&]{
            return audioFrameCounter.load(std::memory_order_acquire) >= needAbs;
        };

        if (!have_enough() && wait_ms > 0) {
            std::unique_lock<std::mutex> lk(cvMx);
            cvAvail.wait_for(lk, std::chrono::milliseconds(wait_ms), have_enough);
        }

        uint64_t prodNow = audioFrameCounter.load(std::memory_order_acquire);

        // Producer stall tracking
        static uint64_t lastProdSeen = 0;
        static int      stallCount   = 0;
        if (prodNow == lastProdSeen) ++stallCount; else stallCount = 0;
        lastProdSeen = prodNow;

        // ----- Shortage handling -----
        static int      underrunsRow = 0;
        static uint64_t lastSnapNs   = 0;

        if (prodNow < needAbs) {
            std::fill(dst48, dst48 + OUT, 0.0f);
            underruns.fetch_add(1, std::memory_order_relaxed);

            if (++underrunsRow >= UNDERRUN_ARM) {
                const uint64_t snap = (prodNow > (LAG_SOFT_CLAMP+1)) ? (prodNow - (LAG_SOFT_CLAMP+1)) : 0ULL;
                rs_srcPos    = double(snap);
                underrunsRow = 0;
                lastSnapNs   = now_ns();
                qWarning() << "[AcqThread] Recovered from audio underrun (hard snap).";
            }
            return UINT_MAX;
        }

        // Soft realign if producer stalled and we’re too close to the tail
        if (stallCount >= STALL_ARM) {
            stallCount = 0;
            const uint64_t i0 = uint64_t(std::floor(rs_srcPos));
            if (prodNow <= i0 + OUT + 1) {
                const uint64_t snap = (prodNow > (LAG_SOFT_CLAMP+1)) ? (prodNow - (LAG_SOFT_CLAMP+1)) : 0ULL;
                rs_srcPos  = double(snap);
                lastSnapNs = now_ns();
                qWarning() << "[AcqThread] Producer stall detected; realigned.";
            }
        }

        // Safety clamp if far behind (approaching overwrite)
        {
            const uint64_t i0  = uint64_t(std::floor(rs_srcPos));
            const uint64_t lag = prodNow - i0;
            const uint64_t maxSafeLag = (bufSizeFrames > SAFETY_MARGIN)
                                        ? (bufSizeFrames - SAFETY_MARGIN) : 0U;
            if (lag > maxSafeLag) {
                const uint64_t snap = (prodNow > (LAG_SOFT_CLAMP+1)) ? (prodNow - (LAG_SOFT_CLAMP+1)) : 0ULL;
                rs_srcPos = double(snap);
                const uint64_t now = now_ns();
                if (now - lastSnapNs > 400'000'000ULL) {
                    lastSnapNs = now;
                    qWarning() << "[AcqThread] Reader far behind; soft-clamped to tail.";
                }
            }
        }

        // ----- Fractional read (LEFT) + trigger (RIGHT) -----
        auto i16_to_f32 = [](int16_t s)->float {
            return (s == INT16_MIN) ? -1.0f : float(s) / 32767.0f;
        };
        auto sampleChan = [&](uint64_t i, int ch)->int16_t {
            const size_t idx = (i % bufSizeFrames) * AUDIO_NUM_CHANNELS + ch;
            return audioRing[idx];
        };

        unsigned trigOff = UINT_MAX;
        bool     trigUp  = false;

        double pos = rs_srcPos;
        for (unsigned n = 0; n < OUT; ++n, pos += stepUse) {
            const uint64_t i0 = uint64_t(std::floor(pos));
            const double   f  = pos - double(i0);

            const float L0 = i16_to_f32(sampleChan(i0,   0));
            const float L1 = i16_to_f32(sampleChan(i0+1, 0));
            dst48[n] = L0 + float(f) * (L1 - L0);

            if (trigOff == UINT_MAX) {
                const int16_t r = sampleChan(i0, 1);
                if (!trigUp && r >= int16_t(trigHigh)) {
                    const uint64_t absPos = i0;
                    if (absPos - lastTrigFrame >= trigMinGap) {
                        trigOff = n;
                        lastTrigFrame = absPos;
                    }
                    trigUp = true;
                } else if (trigUp && r <= int16_t(trigLow)) {
                    trigUp = false;
                }
            }
        }

        rs_srcPos    = pos;
        underrunsRow = 0;
        return trigOff;
    }

private:
    void captureLoop() {
#ifdef __linux__
        struct sched_param sp{30};
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
#endif
        snd_pcm_hw_params_t* p; snd_pcm_hw_params_alloca(&p);
        snd_pcm_hw_params_current(handle, p);
        snd_pcm_uframes_t period = 0; snd_pcm_hw_params_get_period_size(p, &period, 0);

        std::vector<int16_t> buf(size_t(period) * AUDIO_NUM_CHANNELS);

        uint64_t lastPrint = 0;
        while (run.load(std::memory_order_acquire)) {
            int r = snd_pcm_readi(handle, buf.data(), period);
            if (r == -EPIPE) { snd_pcm_prepare(handle); continue; }
            if (r < 0) { std::fprintf(stderr, "[AudioAmp] read: %s\n", snd_strerror(r)); continue; }

            unsigned head       = bufHead.load(std::memory_order_relaxed);
            unsigned spaceToEnd = bufSizeFrames - head;
            unsigned first      = std::min<unsigned>(r, spaceToEnd);

            std::memcpy(&audioRing[size_t(head) * AUDIO_NUM_CHANNELS],
                        buf.data(),
                        size_t(first) * AUDIO_NUM_CHANNELS * sizeof(int16_t));

            if (r > (int)first) {
                std::memcpy(&audioRing[0],
                            buf.data() + size_t(first) * AUDIO_NUM_CHANNELS,
                            size_t(r - first) * AUDIO_NUM_CHANNELS * sizeof(int16_t));
            }

            audioFrameCounter.fetch_add(r, std::memory_order_relaxed);
            bufHead.store((head + r) % bufSizeFrames, std::memory_order_release);

            { std::lock_guard<std::mutex> lk(cvMx); cvAvail.notify_all(); }

            uint64_t nowTick = audioFrameCounter.load(std::memory_order_relaxed);
            if (nowTick/48000 != lastPrint/48000) {
                qInfo() << "[AudioAmp] prod=" << nowTick << "cons=0";
            }
            lastPrint = nowTick;
        }
    }
};

#endif // _AUDIOAMP_H
