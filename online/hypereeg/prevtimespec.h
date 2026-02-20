#ifdef __linux__
#include <time.h>
#include <errno.h>

static inline int64_t timespec_to_ns(const timespec& ts) {
 return int64_t(ts.tv_sec)*1000000000LL+int64_t(ts.tv_nsec);
}

static inline timespec ns_to_timespec(int64_t ns) {
 timespec ts;
 ts.tv_sec=time_t(ns/1000000000LL);
 ts.tv_nsec=long(ns%1000000000LL);
 return ts;
}

static inline timespec ts_add_ns(timespec ts,int64_t add_ns) {
 int64_t ns=timespec_to_ns(ts)+add_ns;
 return ns_to_timespec(ns);
}

// absolute sleep until deadline (monotonic), resilient to EINTR
static inline void sleep_until_abs(const timespec& deadline) {
 while (true) {
  const int rc=::clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&deadline,nullptr);
  if (rc==0) return;
  if (rc==EINTR) continue;
  // unexpected: just bail out of sleep
  return;
 }
}
#endif
