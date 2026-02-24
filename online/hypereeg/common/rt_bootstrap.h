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

#ifdef __linux__
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static inline void warn_errno(const char* what) {
 fprintf(stderr,"[RT] %s failed: %s (errno=%d)\n",what,strerror(errno),errno);
}

static inline bool set_process_nice(int nice_val) {
 errno=0;
 if (setpriority(PRIO_PROCESS,0,nice_val)!=0) { warn_errno("setpriority"); return false; }
 return true;
}

//static inline void prefault_stack() {
// // Touch stack pages to force mapping now (avoid page faults later)
// constexpr size_t kStack=8*1024*1024; // 8MB
// volatile char dummy[kStack];
// for (size_t i=0;i<kStack;i+=4096) dummy[i]=0;
//}

//static inline void prefault_stack() {
// // keep this small (see section C)
// volatile char dummy[64*1024];
// memset((void*)dummy,0,sizeof(dummy));
//}

static inline void prefault_stack() {
 // 256 KiB is plenty; touching 8-32 MiB will easily blow a QThread stack.
 constexpr size_t N=256*1024;
 volatile char *p=(volatile char*)alloca(N);
 for (size_t i=0;i<N;i+=4096) p[i]=0;
}

static inline void dump_memlock_limits() {
 rlimit rl{};
 if (getrlimit(RLIMIT_MEMLOCK,&rl)==0) {
  qWarning() << "[RT] RLIMIT_MEMLOCK cur=" << (qulonglong)rl.rlim_cur
             << " max=" << (qulonglong)rl.rlim_max;
 }
}

static inline void lock_memory_or_warn() {
// dump_memlock_limits();
 if (mlockall(MCL_CURRENT|MCL_FUTURE)!=0) {
  qWarning() << "[RT] mlockall failed:" << strerror(errno);
 } else {
  qInfo() << "[RT] mlockall OK";
 }
 prefault_stack();
}

//static bool lock_memory() {
// if (mlockall(MCL_CURRENT|MCL_FUTURE)!=0) { warn_errno("mlockall"); return false; }
// return true;
//}

static inline bool set_thread_rt(pthread_t thr,int policy,int prio) {
 sched_param sp{};
 sp.sched_priority=prio;
 int rc=pthread_setschedparam(thr,policy,&sp);
 if (rc!=0) { errno=rc; warn_errno("pthread_setschedparam"); return false; }
 return true;
}

static inline bool pin_thread_to_cpu(pthread_t thr,int cpu) {
 cpu_set_t set;
 CPU_ZERO(&set);
 CPU_SET(cpu,&set);
 int rc=pthread_setaffinity_np(thr,sizeof(set),&set);
 if (rc!=0) { errno=rc; warn_errno("pthread_setaffinity_np"); return false; }
 return true;
}
#endif
