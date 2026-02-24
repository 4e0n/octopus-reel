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

#include <QThread>
#include <QMutexLocker>
#include <QByteArray>
#include <QDebug>

#ifdef __linux__
#include <pthread.h>
#endif

#include "confparam.h"
#include "../common/timespec.h"
#include "../common/tcp_sendloop.h"
#include "../common/logring.h"

#ifdef EEMAGINE
#include "../common/rt_bootstrap.h"
#endif

#include "../common/tcpsample_pp.h"   // <-- adjust include name to whatever you use (TcpSamplePP)

//
// #define LOG_PLL   // enable PLL logs if desired
//

class TcpThread : public QThread {
  Q_OBJECT
public:
  explicit TcpThread(ConfParam *c, QObject *parent=nullptr)
    : QThread(parent), conf(c)
  {
    tcpBufSize = conf->tcpBufSize;
    tcpBuffer  = &conf->tcpBuffer;     // must be QVector<TcpSamplePP>
    conf->tcpBufTail = 0;
  }

protected:
  void run() override {
#ifdef __linux__
#ifdef EEMAGINE
    lock_memory_or_warn();
    pin_thread_to_cpu(pthread_self(), 3);  // different core than compute if possible
    set_thread_rt(pthread_self(), SCHED_FIFO, 55);
#endif
#endif

    const int N = conf->eegSamplesInTick;
    const int64_t basePeriod_ns =
        (int64_t(N) * 1000000000LL) / int64_t(conf->eegRate);

    // In PP, your serialized sample size is known/configured
    const int sz = conf->frameBytesOut;
    if (sz <= 0) {
      qWarning() << "[PP:SEND] frameBytesOut not set";
      return;
    }

    const int expectSz = TcpSamplePP::serializedSizeFor(conf->ampCount, conf->physChnCount);
    if (sz != expectSz) {
      qWarning() << "[PP:SEND] frameBytesOut mismatch sz=" << sz << " expect=" << expectSz;
      return; // recommended while stabilizing
    }

    sendloop::Params p;
    p.N = N;
    p.basePeriod_ns = basePeriod_ns;
    p.target = uint64_t(2 * N);

    // PP is typically “more conservative”: require at least target available
    // (You can change this to N+N/2 if you want it to behave like node-acq)
    p.minAvailToSend = uint64_t(2 * N);

    p.Kp_ns_per_sample = std::max<int64_t>(2000, 1000000LL / int64_t(p.target) / 2);
    p.Ki_ns_per_sample = std::max<int64_t>(1, p.Kp_ns_per_sample / 500); // your PP used /500
    p.corrClamp_ns = basePeriod_ns / 4;
    p.integClamp   = 10 * int64_t(p.target);

    // Wait for at least one sample
    while (!isInterruptionRequested()) {
      QMutexLocker lk(&conf->mutex);
      if (conf->tcpBufHead - conf->tcpBufTail) break;
      lk.unlock();
      msleep(1);
    }

    auto shouldStop = [this]() -> bool {
      return isInterruptionRequested();
    };

    auto availFn = [this]() -> uint64_t {
      QMutexLocker lk(&conf->mutex);
      return uint64_t(conf->tcpBufHead - conf->tcpBufTail);
    };

    // Used only for 1Hz log_ring_1hz (like before)
    quint64 headSnap = 0;
    quint64 tailAfter = 0;

    auto serializeAndAdvance = [this, N, sz, &headSnap, &tailAfter](char* dst) -> bool {
      quint64 tail0 = 0;

      {
        QMutexLocker lk(&conf->mutex);

        const quint64 avail = conf->tcpBufHead - conf->tcpBufTail;
        if (avail < quint64(N)) return false;

        tail0     = conf->tcpBufTail;
        headSnap  = conf->tcpBufHead;
        tailAfter = tail0 + quint64(N);

        conf->tcpBufTail = tailAfter;

        // If you have a producer waiting on space, keep this
        conf->spaceReady.wakeOne();
      }

      // Serialize OUTSIDE mutex (faster, like your old PP thread)
      for (int i = 0; i < N; ++i) {
        const TcpSamplePP &s = (*tcpBuffer)[(tail0 + quint64(i)) % tcpBufSize];
        const int wrote = s.serializeTo(dst + i * sz, sz);
        if (wrote != sz) {
          qWarning() << "[PP:SEND] serializeTo failed wrote=" << wrote << " expected=" << sz;
        }
      }

      // 1Hz ring log (no mutex required; uses snapped head/tail)
      static quint64 lastH = 0, lastT = 0;
      static qint64  lastMs = 0;
      log_ring_1hz("PP:SEND", headSnap, tailAfter, lastH, lastT, lastMs);

      return true;
    };

    auto sendFn = [this](const char* outData, int outLen) {
      emit sendPacketSignal(QByteArray(outData, outLen));
    };

    sendloop::DebugFn dbgFn = nullptr;

#ifdef LOG_PLL
    dbgFn = [](uint64_t avail, uint64_t target, int64_t err,
               int64_t corrNs, int64_t periodNs, int64_t baseNs)
    {
      const double fill = (target > 0) ? double(avail) / double(target) : 0.0;
      qInfo().noquote()
        << QString("<TcpPLL-PP> avail=%1 target=%2 fill=%3 err=%4 corrNs=%5 periodNs=%6 baseNs=%7")
             .arg(qulonglong(avail))
             .arg(qulonglong(target))
             .arg(fill, 0, 'f', 2)
             .arg(qlonglong(err))
             .arg(qlonglong(corrNs))
             .arg(qlonglong(periodNs))
             .arg(qlonglong(baseNs));
    };
#endif

    sendloop::run_paced_sender(p, sz, shouldStop, availFn, serializeAndAdvance, sendFn, dbgFn);
  }

signals:
  void sendPacketSignal(const QByteArray &packet);

private:
  ConfParam *conf = nullptr;
  QVector<TcpSamplePP> *tcpBuffer = nullptr;
  unsigned int tcpBufSize = 0;
};
