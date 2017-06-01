// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "clocks.h"
#include "mcfwin.h"
#include "bail.h"

static uint64_t GetTimeZoneOffsetInMillisecondsOnce(void){
	static uint64_t s_u64Value, *volatile s_pu64Inited;

	uint64_t *pu64Inited = __atomic_load_n(&s_pu64Inited, __ATOMIC_CONSUME);
	if(!pu64Inited){
		TIME_ZONE_INFORMATION vTzInfo;
		if(GetTimeZoneInformation(&vTzInfo) == TIME_ZONE_ID_INVALID){
			_MCFCRT_Bail(L"GetTimeZoneInformation() failed.");
		}
		const uint64_t u64Value = (uint64_t)(vTzInfo.Bias * -60000ll);
		pu64Inited = &s_u64Value;
		__atomic_store(pu64Inited, &u64Value, __ATOMIC_RELAXED);
		__atomic_store_n(&s_pu64Inited, pu64Inited, __ATOMIC_RELEASE);
	}
	return *pu64Inited;
}
static double QueryPerformanceFrequencyReciprocalOnce(void){
	static double s_lfValue, *volatile s_plfInited;

	double *plfInited = __atomic_load_n(&s_plfInited, __ATOMIC_CONSUME);
	if(!plfInited){
		LARGE_INTEGER liFreq;
		if(!QueryPerformanceFrequency(&liFreq)){
			_MCFCRT_Bail(L"QueryPerformanceFrequency() failed.");
		}
		const double lfValue = 1000.0 / (double)liFreq.QuadPart;
		plfInited = &s_lfValue;
		__atomic_store(plfInited, &lfValue, __ATOMIC_RELAXED);
		__atomic_store_n(&s_plfInited, plfInited, __ATOMIC_RELEASE);
	}
	return *plfInited;
}

uint64_t _MCFCRT_GetUtcClock(void){
	union {
		FILETIME ft;
		LARGE_INTEGER li;
	} unUtc;
	GetSystemTimeAsFileTime(&unUtc.ft);
	// 0x019DB1DED53E8000 = duration since 1601-01-01 until 1970-01-01 in nanoseconds.
	return (uint64_t)(((double)unUtc.li.QuadPart - 0x019DB1DED53E8000ll) / 10000.0);
}
uint64_t _MCFCRT_GetLocalClock(void){
	return _MCFCRT_GetLocalClockFromUtc(_MCFCRT_GetUtcClock());
}

uint64_t _MCFCRT_GetUtcClockFromLocal(uint64_t u64LocalClock){
	return u64LocalClock - GetTimeZoneOffsetInMillisecondsOnce();
}
uint64_t _MCFCRT_GetLocalClockFromUtc(uint64_t u64UtcClock){
	return u64UtcClock + GetTimeZoneOffsetInMillisecondsOnce();
}

#ifdef NDEBUG
#  define DEBUG_MONO_CLOCK_OFFSET   0ull
#else
#  define DEBUG_MONO_CLOCK_OFFSET   0x100000000ull
#endif

uint64_t _MCFCRT_GetFastMonoClock(void){
	return GetTickCount64() + DEBUG_MONO_CLOCK_OFFSET;
}
double _MCFCRT_GetHiResMonoClock(void){
	LARGE_INTEGER liCounter;
	if(!QueryPerformanceCounter(&liCounter)){
		_MCFCRT_Bail(L"QueryPerformanceCounter() failed.");
	}
	return ((double)liCounter.QuadPart + (double)(DEBUG_MONO_CLOCK_OFFSET * 2)) * QueryPerformanceFrequencyReciprocalOnce();
}
