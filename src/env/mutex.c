// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2019, LH_Mouse. All wrongs reserved.

#define __MCFCRT_MUTEX_INLINE_OR_EXTERN     extern inline
#include "mutex.h"
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

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define BSUSR(v_)             ((uintptr_t)(v_) << CHAR_BIT)
#  define BSFB(v_)              ((uintptr_t)(v_)            )
#else
#  define BSUSR(v_)             ((uintptr_t)(v_)                                        )
#  define BSFB(v_)              ((uintptr_t)(v_) << ((sizeof(uintptr_t) - 1) * CHAR_BIT))
#endif

#define MASK_LOCKED             ((uintptr_t)( BSFB(0x01)))
#define MASK_THREADS_SPINNING   ((uintptr_t)( BSFB(0x0E)))
#define MASK_SPIN_FAILURE_COUNT ((uintptr_t)( BSFB(0xF0)))
#define MASK_THREADS_TRAPPED    ((uintptr_t)(~BSFB(0xFF)))

#define THREADS_SPINNING_ONE    ((uintptr_t)(MASK_THREADS_SPINNING & -MASK_THREADS_SPINNING))
#define THREADS_SPINNING_MAX    ((uintptr_t)(MASK_THREADS_SPINNING / THREADS_SPINNING_ONE))

#define SPIN_FAILURE_COUNT_ONE  ((uintptr_t)(MASK_SPIN_FAILURE_COUNT & -MASK_SPIN_FAILURE_COUNT))
#define SPIN_FAILURE_COUNT_MAX  ((uintptr_t)(MASK_SPIN_FAILURE_COUNT / SPIN_FAILURE_COUNT_ONE))

#define THREADS_TRAPPED_ONE     ((uintptr_t)(MASK_THREADS_TRAPPED & -MASK_THREADS_TRAPPED))
#define THREADS_TRAPPED_MAX     ((uintptr_t)(MASK_THREADS_TRAPPED / THREADS_TRAPPED_ONE))

#define MIN_SPIN_COUNT          ((uintptr_t)16)
#define MAX_SPIN_MULTIPLIER     ((uintptr_t)32)

__attribute__((__always_inline__)) static inline bool ReallyWaitForMutex(volatile uintptr_t *puControl, size_t uMaxSpinCountInitial, bool bMayTimeOut, uint64_t u64UntilFastMonoClock)
  {
    for(;;) {
      size_t uMaxSpinCount, uSpinMultiplier;
      bool bTaken, bSpinnable;
      {
        uintptr_t uOld, uNew;
        uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
        do {
          const size_t uSpinFailureCount = (uOld & MASK_SPIN_FAILURE_COUNT) / SPIN_FAILURE_COUNT_ONE;
          if(uMaxSpinCountInitial > MIN_SPIN_COUNT) {
            uMaxSpinCount = (uMaxSpinCountInitial >> uSpinFailureCount) | MIN_SPIN_COUNT;
            uSpinMultiplier = MAX_SPIN_MULTIPLIER >> uSpinFailureCount;
          } else {
            uMaxSpinCount = uMaxSpinCountInitial;
            uSpinMultiplier = 0;
          }
          bTaken = !(uOld & MASK_LOCKED);
          bSpinnable = false;
          if(!bTaken) {
            if(uMaxSpinCount != 0) {
              const size_t uThreadsSpinning = (uOld & MASK_THREADS_SPINNING) / THREADS_SPINNING_ONE;
              bSpinnable = uThreadsSpinning < THREADS_SPINNING_MAX;
            }
            if(!bSpinnable) {
              break;
            }
            uNew = uOld + THREADS_SPINNING_ONE;
          } else {
            const bool bSpinFailureCountDecremented = uSpinFailureCount != 0;
            uNew = uOld + MASK_LOCKED - bSpinFailureCountDecremented * SPIN_FAILURE_COUNT_ONE;
          }
        } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
      }
      if(_MCFCRT_EXPECT(bTaken)) {
        return true;
      }
      if(_MCFCRT_EXPECT(bSpinnable)) {
        for(size_t uSpinIndex = 0; _MCFCRT_EXPECT(uSpinIndex < uMaxSpinCount); ++uSpinIndex) {
          register size_t uMultiplierIndex = uSpinMultiplier + 1;
          __atomic_thread_fence(__ATOMIC_SEQ_CST);
          do {
            __builtin_ia32_pause();
          } while(--uMultiplierIndex != 0);
          __atomic_thread_fence(__ATOMIC_SEQ_CST);
          {
            uintptr_t uOld, uNew;
            uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
            do {
              bTaken = !(uOld & MASK_LOCKED);
              if(!bTaken) {
                break;
              }
              const size_t uSpinFailureCount = (uOld & MASK_SPIN_FAILURE_COUNT) / SPIN_FAILURE_COUNT_ONE;
              const bool bSpinFailureCountDecremented = uSpinFailureCount != 0;
              uNew = uOld - THREADS_SPINNING_ONE + MASK_LOCKED - bSpinFailureCountDecremented * SPIN_FAILURE_COUNT_ONE;
            } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
          }
          if(_MCFCRT_EXPECT_NOT(bTaken)) {
            return true;
          }
        }
        {
          uintptr_t uOld, uNew;
          uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
          do {
            const size_t uSpinFailureCount = (uOld & MASK_SPIN_FAILURE_COUNT) / SPIN_FAILURE_COUNT_ONE;
            bTaken = !(uOld & MASK_LOCKED);
            if(!bTaken) {
              const bool bSpinFailureCountIncremented = uSpinFailureCount < SPIN_FAILURE_COUNT_MAX;
              uNew = uOld - THREADS_SPINNING_ONE + THREADS_TRAPPED_ONE + bSpinFailureCountIncremented * SPIN_FAILURE_COUNT_ONE;
            } else {
              const bool bSpinFailureCountDecremented = uSpinFailureCount != 0;
              uNew = uOld - THREADS_SPINNING_ONE + MASK_LOCKED - bSpinFailureCountDecremented * SPIN_FAILURE_COUNT_ONE;
            }
          } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
        }
        if(_MCFCRT_EXPECT(bTaken)) {
          return true;
        }
      } else {
        {
          uintptr_t uOld, uNew;
          uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
          do {
            bTaken = !(uOld & MASK_LOCKED);
            if(!bTaken) {
              uNew = uOld + THREADS_TRAPPED_ONE;
            } else {
              uNew = uOld + MASK_LOCKED;
            }
          } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
        }
        if(_MCFCRT_EXPECT(bTaken)) {
          return true;
        }
      }
      if(bMayTimeOut) {
        LARGE_INTEGER liTimeout;
        __MCFCRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
        NTSTATUS lStatus = NtWaitForKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, &liTimeout);
        _MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() failed.");
        while(_MCFCRT_EXPECT(lStatus == STATUS_TIMEOUT)) {
          bool bDecremented;
          {
            uintptr_t uOld, uNew;
            uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
            do {
              const size_t uThreadsTrapped = (uOld & MASK_THREADS_TRAPPED) / THREADS_TRAPPED_ONE;
              bDecremented = uThreadsTrapped != 0;
              if(!bDecremented) {
                break;
              }
              uNew = uOld - THREADS_TRAPPED_ONE;
            } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)));
          }
          if(bDecremented) {
            return false;
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

__attribute__((__always_inline__)) static inline void ReallySignalMutex(volatile uintptr_t *puControl)
  {
    bool bSignalOne;
    {
      uintptr_t uOld, uNew;
      uOld = __atomic_load_n(puControl, __ATOMIC_RELAXED);
      do {
        _MCFCRT_ASSERT_MSG(uOld & MASK_LOCKED, L"This mutex isn't locked by any thread.");
        bSignalOne = (uOld & MASK_THREADS_TRAPPED) != 0;
        uNew = (uOld & ~MASK_LOCKED) - bSignalOne * THREADS_TRAPPED_ONE;
      } while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(puControl, &uOld, uNew, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED)));
    }
    // If `RtlDllShutdownInProgress()` is `true`, other threads will have been terminated.
    // Calling `NtReleaseKeyedEvent()` when no thread is waiting results in deadlocks. Don't do that.
    if(_MCFCRT_EXPECT_NOT(bSignalOne && !RtlDllShutdownInProgress())) {
      NTSTATUS lStatus = NtReleaseKeyedEvent(_MCFCRT_NULLPTR, (void *)puControl, false, _MCFCRT_NULLPTR);
      _MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtReleaseKeyedEvent() failed.");
      _MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
    }
  }

bool __MCFCRT_ReallyWaitForMutex(_MCFCRT_Mutex *pMutex, size_t uMaxSpinCount, uint64_t u64UntilFastMonoClock)
  {
    const bool bLocked = ReallyWaitForMutex(&(pMutex->__u), uMaxSpinCount, true, u64UntilFastMonoClock);
    return bLocked;
  }

void __MCFCRT_ReallyWaitForMutexForever(_MCFCRT_Mutex *pMutex, size_t uMaxSpinCount)
  {
    const bool bLocked = ReallyWaitForMutex(&(pMutex->__u), uMaxSpinCount, false, UINT64_MAX);
    _MCFCRT_ASSERT(bLocked);
  }

void __MCFCRT_ReallySignalMutex(_MCFCRT_Mutex *pMutex)
  {
    ReallySignalMutex(&(pMutex->__u));
  }
