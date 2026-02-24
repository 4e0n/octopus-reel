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

#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
#include <chrono>

#include "timespec.h" // ts::now(), ts::add_ns(), ts::sleep_until_abs(), ts::timespec_to_ns()

namespace sendloop {

using ShouldStopFn=std::function<bool(void)>;
using AvailFn=std::function<uint64_t(void)>;

// Return true if it serialized and advanced tail by N; false if not enough data after lock/recheck
using SerializeAndAdvanceFn=std::function<bool(char* payloadDst)>;

using SendFn=std::function<void(const char* outData,int outLen)>;

// Optional debug callback (Qt-free types)
using DebugFn=std::function<void(uint64_t avail,uint64_t target,int64_t err,int64_t corrNs,
                                 int64_t periodNs,int64_t baseNs)>;
struct Params {
 int N=0;
 int64_t basePeriod_ns=0;
 uint64_t target=0;         // e.g. 2*N
 uint64_t minAvailToSend=0; // e.g. N+N/2

 int64_t Kp_ns_per_sample=0;
 int64_t Ki_ns_per_sample=0;
 int64_t corrClamp_ns=0;
 int64_t integClamp=0;

 int64_t lateResyncThresholdPeriods=5; // resync if now > next + 5*basePeriod
 int64_t startDelay_ns=5*1000*1000; // 5ms
 int64_t logEvery_ms=1000;          // debug rate
};

inline int64_t clamp_i64(int64_t v,int64_t lo,int64_t hi) {
 return (v<lo)?lo: (v>hi)?hi:v;
}

inline void run_paced_sender(const Params& p,int payloadBytesPerSample,ShouldStopFn shouldStop,
                             AvailFn availFn,SerializeAndAdvanceFn serializeAndAdvanceFn,
                             SendFn sendFn,DebugFn dbgFn=nullptr) {
 // local state
 ts::deadline_t nextDeadline=ts::add_ns(ts::now(),p.startDelay_ns);
 int64_t integ=0;

 const int payloadLen=p.N*payloadBytesPerSample;
 const uint32_t Lo=uint32_t(payloadLen);

 std::vector<char> payload(payloadLen);
 std::vector<char> out(4+payloadLen);

 // fixed header
 out[0]=char((Lo    )&0xff);
 out[1]=char((Lo>> 8)&0xff);
 out[2]=char((Lo>>16)&0xff);
 out[3]=char((Lo>>24)&0xff);

 int64_t lastLogMs=0;

 while (!shouldStop()) {
  ts::sleep_until_abs(nextDeadline);

  const auto nowts=ts::now();
  const int64_t nowNs=ts::timespec_to_ns(nowts);
  const int64_t nextNs=ts::timespec_to_ns(nextDeadline);

  if (nowNs>nextNs+p.lateResyncThresholdPeriods*p.basePeriod_ns) {
   nextDeadline=nowts;
   integ=0;
  }

  const uint64_t avail=availFn();

  // STARVED path: keep cadence stable, don't PI-steer time
  if (avail<p.minAvailToSend) {
   integ=0;
   nextDeadline=ts::add_ns(nextDeadline,p.basePeriod_ns);
   continue;
  }

  // PI path
  const int64_t err=int64_t(avail)-int64_t(p.target);
  integ=clamp_i64(integ+err,-p.integClamp,p.integClamp);

  int64_t corrNs=p.Kp_ns_per_sample*err+p.Ki_ns_per_sample*integ;
  corrNs=clamp_i64(corrNs,-p.corrClamp_ns,p.corrClamp_ns);

  int64_t periodNs=p.basePeriod_ns-corrNs;
  periodNs=clamp_i64(periodNs,p.basePeriod_ns/2,p.basePeriod_ns*2);

  nextDeadline=ts::add_ns(nextDeadline,periodNs);

  // serialize+advance tail under caller's mutex
  if (!serializeAndAdvanceFn(payload.data())) {
   // Not enough data after re-check; keep cadence stable, don't spam PI
   integ=0;
   nextDeadline=ts::add_ns(nextDeadline,p.basePeriod_ns);
   continue;
  }

  // copy payload into out and send
  std::memcpy(out.data()+4,payload.data(),payloadLen);
  sendFn(out.data(),int(out.size()));

  // debug @ ~1Hz using monotonic-ish wall ms from ns
  // inside run_paced_sender(...) while loop, after computing err/corr/period
  if (dbgFn) {
   using clock=std::chrono::steady_clock;
   static auto last=clock::now();
   auto now=clock::now();
   if (dbgFn && (now-last)>=std::chrono::seconds(1)) {
    dbgFn(avail,p.target,err,corrNs,periodNs,p.basePeriod_ns);
    last=now;
   }
  }
 }
}

} // namespace sendloop
