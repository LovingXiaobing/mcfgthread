// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#define __MCFCRT_ONCE_FLAG_INLINE_OR_EXTERN     extern inline
#include "once_flag.h"
#include "_nt_timeout.h"
#include "xassert.h"
#include "expect.h"
#include <ntdef.h>

__attribute__((__dllimport__, __stdcall__)) extern NTSTATUS NtWaitForKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout);
__attribute__((__dllimport__, __stdcall__)) extern NTSTATUS NtReleaseKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout);

__attribute__((__dllimport__, __stdcall__, __const__)) extern BOOLEAN RtlDllShutdownInProgress(void);

#ifndef __BYTE_ORDER__
#  error Byte order is unknown.
#endif
// The first byte is reserved by Itanium ABI to indicate whether the initialization has succeeded.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define BSUSR(v_)             ((uintptr_t)(v_) << CHAR_BIT)
#  define BSFB(v_)              ((uintptr_t)(v_)            )
#else
#  define BSUSR(v_)             ((uintptr_t)(v_)                                        )
#  define BSFB(v_)              ((uintptr_t)(v_) << ((sizeof(uintptr_t) - 1) * CHAR_BIT))
#endif

#define MASK_LOCKED             ((uintptr_t)( BSUSR(0x01)              ))
#define MASK_FINISHED           ((uintptr_t)(                BSFB(0x01)))
#define MASK_THREADS_TRAPPED    ((uintptr_t)(~BSUSR(0x01) & ~BSFB(0xFF)))

#define THREADS_TRAPPED_ONE     ((uintptr_t)(MASK_THREADS_TRAPPED & -MASK_THREADS_TRAPPED))
#define THREADS_TRAPPED_MAX     ((uintptr_t)(MASK_THREADS_TRAPPED / THREADS_TRAPPED_ONE))

__attribute__((__always_inline__)) static inline _MCFCRT_OnceResult ReallyWaitForOnceFlag(volatile uintptr_t *puControl, bool bMayTimeOut, uint64_t u64UntilFastMonoClock){
	for(;;){
		bool bFinished, bTaken = false;
		{
			uintptr_t uOld, uNew;
			uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
			do {
				bFinished = !!(uOld & MASK_FINISHED);
				if(bFinished){
					break;
				}
				bTaken = !(uOld & MASK_LOCKED);
				if(!bTaken){
					uNew = uOld + THREADS_TRAPPED_ONE;
				} else {
					uNew = uOld + MASK_LOCKED;
				}
			} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
		}
		if(_MCFCRT_EXPECT(bFinished)){
			return _MCFCRT_kOnceResultFinished;
		}
		if(_MCFCRT_EXPECT(bTaken)){
			return _MCFCRT_kOnceResultInitial;
		}
		if(bMayTimeOut){
			LARGE_INTEGER liTimeout;
			__MCFCRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
			NTSTATUS lStatus = NtWaitForKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, &liTimeout);
			_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() failed.");
			while(_MCFCRT_EXPECT(lStatus == STATUS_TIMEOUT)){
				bool bDecremented;
				{
					uintptr_t uOld, uNew;
					uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
					do {
						const size_t uThreadsTrapped = (uOld & MASK_THREADS_TRAPPED) / THREADS_TRAPPED_ONE;
						bDecremented = uThreadsTrapped != 0;
						if(!bDecremented){
							break;
						}
						uNew = uOld - THREADS_TRAPPED_ONE;
					} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
				}
				if(bDecremented){
					return _MCFCRT_kOnceResultTimedOut;
				}
				liTimeout.QuadPart = 0;
				lStatus = NtWaitForKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, &liTimeout);
				_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() failed.");
			}
		} else {
			NTSTATUS lStatus = NtWaitForKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, _MCFCRT_NULLPTR);
			_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() failed.");
			_MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
		}
	}
}
__attribute__((__always_inline__)) static inline void ReallySignalOnceFlag(volatile uintptr_t *puControl, bool bFinished){
	uintptr_t uCountToSignal;
	{
		uintptr_t uOld, uNew;
		uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
		do {
			_MCFCRT_ASSERT_MSG(uOld & MASK_LOCKED, L"This once flag isn't locked by any thread.");
			_MCFCRT_ASSERT_MSG(!(uOld & MASK_FINISHED), L"This once flag has been disposed.");
			const size_t uThreadsTrapped = (uOld & MASK_THREADS_TRAPPED) / THREADS_TRAPPED_ONE;
			const uintptr_t uMaxCountToSignal = (uintptr_t)(1 - bFinished * 2);
			uCountToSignal = (uThreadsTrapped <= uMaxCountToSignal) ? uThreadsTrapped : uMaxCountToSignal;
			uNew = (uOld & ~(MASK_LOCKED | MASK_FINISHED)) + bFinished * MASK_FINISHED - uCountToSignal * THREADS_TRAPPED_ONE;
		} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)));
	}
	// If `RtlDllShutdownInProgress()` is `true`, other threads will have been terminated.
	// Calling `NtReleaseKeyedEvent()` when no thread is waiting results in deadlocks. Don't do that.
	if(_MCFCRT_EXPECT_NOT((uCountToSignal > 0) && !RtlDllShutdownInProgress())){
		for(size_t uIndex = 0; uIndex < uCountToSignal; ++uIndex){
			NTSTATUS lStatus = NtReleaseKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, _MCFCRT_NULLPTR);
			_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtReleaseKeyedEvent() failed.");
			_MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
		}
	}
}

_MCFCRT_OnceResult __MCFCRT_ReallyWaitForOnceFlag(_MCFCRT_OnceFlag *pOnceFlag, uint64_t u64UntilFastMonoClock){
	const _MCFCRT_OnceResult eResult = ReallyWaitForOnceFlag(&(pOnceFlag->__u), true, u64UntilFastMonoClock);
	return eResult;
}
_MCFCRT_OnceResult __MCFCRT_ReallyWaitForOnceFlagForever(_MCFCRT_OnceFlag *pOnceFlag){
	const _MCFCRT_OnceResult eResult = ReallyWaitForOnceFlag(&(pOnceFlag->__u), false, UINT64_MAX);
	_MCFCRT_ASSERT(eResult != _MCFCRT_kOnceResultTimedOut);
	return eResult;
}
void __MCFCRT_ReallySignalOnceFlagAsFinished(_MCFCRT_OnceFlag *pOnceFlag){
	ReallySignalOnceFlag(&(pOnceFlag->__u), true);
}
void __MCFCRT_ReallySignalOnceFlagAsAborted(_MCFCRT_OnceFlag *pOnceFlag){
	ReallySignalOnceFlag(&(pOnceFlag->__u), false);
}
