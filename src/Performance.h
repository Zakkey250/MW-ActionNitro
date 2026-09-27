#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
namespace modperf {
inline uint64_t now(){LARGE_INTEGER t;QueryPerformanceCounter(&t);return uint64_t(t.QuadPart);}
inline double micros(uint64_t ticks){static const double scale=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return 1e6/double(f.QuadPart);}();return double(ticks)*scale;}
inline constexpr unsigned limits[]={10,25,50,100,250,500,1000,2000,4000,8000,16000,33000,100000};
struct Sample {uint64_t count=0,total=0,maximum=0;unsigned bins[14]{};};
class Stats {
 SRWLOCK lock=SRWLOCK_INIT;Sample data;
public:
 void add(uint64_t duration){
  const double us=micros(duration);unsigned bin=0;while(bin<13&&us>limits[bin])++bin;
  AcquireSRWLockExclusive(&lock);++data.count;data.total+=duration;data.maximum=std::max(data.maximum,duration);++data.bins[bin];ReleaseSRWLockExclusive(&lock);
 }
 void report(const char* label,void (*log)(const char*,...)){
  AcquireSRWLockExclusive(&lock);const auto s=data;data={};ReleaseSRWLockExclusive(&lock);if(!s.count)return;
  uint64_t cumulative=0;unsigned percentile=0;for(unsigned i=0;i<14;++i){cumulative+=s.bins[i];if(cumulative*100>=s.count*95){percentile=i<13?limits[i]:0;break;}}
  log("PERF scope=%s samples=%llu mean_us=%.2f p95_upper_us=%u max_us=%.2f wall_total_ms=%.3f",label,s.count,micros(s.total)/s.count,percentile,micros(s.maximum),micros(s.total)/1000);
 }
};
class Scope {
 Stats& stats;uint64_t start;
public:
 explicit Scope(Stats& s):stats(s),start(now()){}
 ~Scope(){stats.add(now()-start);}
};
}
