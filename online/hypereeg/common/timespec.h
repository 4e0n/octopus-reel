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

#ifdef __linux__
#include <time.h>
#include <errno.h>
#else
#include <chrono>
#include <thread>
#endif

namespace ts {

#ifdef __linux__
using deadline_t=::timespec;

static inline int64_t timespec_to_ns(const ::timespec& ts) {
 return int64_t(ts.tv_sec)*1000000000LL+int64_t(ts.tv_nsec);
}

static inline ::timespec ns_to_timespec(int64_t ns) {
 ::timespec ts{};
 ts.tv_sec=time_t(ns/1000000000LL);
 ts.tv_nsec=long(ns%1000000000LL);
 if (ts.tv_nsec<0) { ts.tv_nsec+=1000000000L; ts.tv_sec-=1; }
 return ts;
}

// Add ns (can be negative). Normalizes tv_nsec.
static inline ::timespec add_ns(::timespec ts,int64_t add) {
 int64_t ns=timespec_to_ns(ts)+add;
 return ns_to_timespec(ns);
}

static inline ::timespec now() {
 ::timespec ts{};
 ::clock_gettime(CLOCK_MONOTONIC,&ts);
 return ts;
}

// Absolute sleep until deadline (CLOCK_MONOTONIC). Handles EINTR.
static inline void sleep_until_abs(const ::timespec& dl) {
 for (;;) {
  const int rc=::clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&dl,nullptr);
  if (rc==0) return;
  if (rc==EINTR) continue;
  // Any other error: just return (don’t spin forever)
  return;
 }
}

#else // !__linux__

using clock_t=std::chrono::steady_clock;
using deadline_t=clock_t::time_point;

static inline deadline_t now() { return clock_t::now(); }

static inline deadline_t add_ns(deadline_t dl,int64_t add) {
 return dl+std::chrono::nanoseconds(add);
}

static inline void sleep_until_abs(deadline_t dl) {
 std::this_thread::sleep_until(dl);
}

static inline int64_t to_ns_since_epoch(deadline_t tp) {
 // steady_clock has no real epoch guarantee; used only for deltas/logging if needed
 return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

#endif

} // namespace timespec
