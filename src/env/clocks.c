// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#define __MCFCRT_CLOCKS_INLINE_OR_EXTERN     extern inline
#include "clocks.h"
#include "mcfwin.h"
#include "bail.h"
#include "once_flag.h"
#include "xassert.h"

static _MCFCRT_OnceFlag g_once;
static uint64_t g_tz_bias;
static double g_pc_freq_recip;

static void FetchParametersOnce(void)
  {
    const _MCFCRT_OnceResult result = _MCFCRT_WaitForOnceFlagForever(&g_once);
    if(result == _MCFCRT_kOnceResultFinished) {
      return;
    }
    _MCFCRT_ASSERT(result == _MCFCRT_kOnceResultInitial);

    TIME_ZONE_INFORMATION tz_info;
    if(GetTimeZoneInformation(&tz_info) == TIME_ZONE_ID_INVALID) {
      _MCFCRT_Bail(L"GetTimeZoneInformation() failed.");
    }
    g_tz_bias = (uint64_t)tz_info.Bias * 60000;

    LARGE_INTEGER pc_freq;
    if(!QueryPerformanceFrequency(&pc_freq)) {
      _MCFCRT_Bail(L"QueryPerformanceFrequency() failed.");
    }
    g_pc_freq_recip = 1000 / (double)pc_freq.QuadPart;

    _MCFCRT_SignalOnceFlagAsFinished(&g_once);
  }

uint64_t _MCFCRT_GetUtcClock(void)
  {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    LARGE_INTEGER li;
    __builtin_memcpy(&li, &ft, sizeof(li));
    // 0x019DB1DED53E8000 = duration since 1601-01-01 until 1970-01-01 in nanoseconds.
    return (uint64_t)(int64_t)((double)(li.QuadPart - 0x019DB1DED53E8000) / 10000);
  }

uint64_t _MCFCRT_GetLocalClock(void)
  {
    const uint64_t utc = _MCFCRT_GetUtcClock();
    return _MCFCRT_GetLocalClockFromUtc(utc);
  }

uint64_t _MCFCRT_GetUtcClockFromLocal(uint64_t local)
  {
    FetchParametersOnce();
    return local + g_tz_bias;
  }

uint64_t _MCFCRT_GetLocalClockFromUtc(uint64_t utc)
  {
    FetchParametersOnce();
    return utc - g_tz_bias;
  }

#ifdef NDEBUG
#  define MONO_CLOCK_OFFSET   0
#else
#  define MONO_CLOCK_OFFSET   0x100000000
#endif

uint64_t _MCFCRT_GetFastMonoClock(void)
  {
    return GetTickCount64() + MONO_CLOCK_OFFSET * 3;
  }

double _MCFCRT_GetHiResMonoClock(void)
  {
    LARGE_INTEGER pc_cntr;
    if(!QueryPerformanceCounter(&pc_cntr)) {
      _MCFCRT_Bail(L"QueryPerformanceCounter() failed.");
    }
    FetchParametersOnce();
    return ((double)pc_cntr.QuadPart + MONO_CLOCK_OFFSET * 5) * g_pc_freq_recip;
  }
