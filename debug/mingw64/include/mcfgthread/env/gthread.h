// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_GTHREAD_H_
#define __MCFCRT_ENV_GTHREAD_H_

// Compatibility layer for GCC.

#include "_crtdef.h"
#include "_mopthread.h"
#include "tls.h"
#include "once_flag.h"
#include "mutex.h"
#include "condition_variable.h"
#include "clocks.h"
#include "crt_assert.h"
#include "expect.h"
#include <time.h> // struct timespec
#include <errno.h>

#ifndef __MCFCRT_GTHREAD_INLINE_OR_EXTERN
#	define __MCFCRT_GTHREAD_INLINE_OR_EXTERN     __attribute__((__gnu_inline__)) extern inline
#endif

#ifdef __GTHREADS
#	error __GTHREADS is already defined. (Thread model confliction detected?)
#endif

#define __GTHREADS 1

_MCFCRT_EXTERN_C_BEGIN

_MCFCRT_CONSTEXPR int __gthread_active_p(void) _MCFCRT_NOEXCEPT {
	return 1;
}

//-----------------------------------------------------------------------------
// Thread local storage
//-----------------------------------------------------------------------------
typedef _MCFCRT_TlsKeyHandle __gthread_key_t;

extern void __MCFCRT_GthreadTlsDestructor(_MCFCRT_STD intptr_t __context, void *__storage) _MCFCRT_NOEXCEPT;

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_key_create(__gthread_key_t *__key_ret, void (*__destructor)(void *)) _MCFCRT_NOEXCEPT {
	const __gthread_key_t __key = _MCFCRT_TlsAllocKey(sizeof(void *), _MCFCRT_NULLPTR, __destructor ? &__MCFCRT_GthreadTlsDestructor : _MCFCRT_NULLPTR, (_MCFCRT_STD intptr_t)__destructor);
	if(!__key){
		return ENOMEM;
	}
	*__key_ret = __key;
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_key_delete(__gthread_key_t __key) _MCFCRT_NOEXCEPT {
	_MCFCRT_TlsFreeKey(__key);
	return 0;
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN void *__MCFCRT_gthread_getspecific(__gthread_key_t __key) _MCFCRT_NOEXCEPT {
	void *__storage;
	const bool __success = _MCFCRT_TlsGet(__key, &__storage);
	if(!__success){
		return _MCFCRT_NULLPTR;
	}
	if(!__storage){
		return _MCFCRT_NULLPTR;
	}
	return *(void **)__storage;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_setspecific(__gthread_key_t __key, const void *__value) _MCFCRT_NOEXCEPT {
	void *__storage;
	const bool __success = _MCFCRT_TlsRequire(__key, &__storage);
	if(!__success){
		return ENOMEM;
	}
	_MCFCRT_ASSERT(__storage);
	*(void **)__storage = (void *)__value;
	return 0;
}

#define __gthread_key_create   __MCFCRT_gthread_key_create
#define __gthread_key_delete   __MCFCRT_gthread_key_delete

#define __gthread_getspecific  __MCFCRT_gthread_getspecific
#define __gthread_setspecific  __MCFCRT_gthread_setspecific

//-----------------------------------------------------------------------------
// Once
//-----------------------------------------------------------------------------
typedef _MCFCRT_OnceFlag __gthread_once_t;

#define __GTHREAD_ONCE_INIT    { 0 }

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_once(__gthread_once_t *__flag, void (*__func)(void)) _MCFCRT_NOEXCEPT {
	const _MCFCRT_OnceResult __result = _MCFCRT_WaitForOnceFlagForever(__flag);
	if(_MCFCRT_EXPECT(__result == _MCFCRT_kOnceResultFinished)){
		return 0;
	}
	_MCFCRT_ASSERT(__result == _MCFCRT_kOnceResultInitial);
	(*__func)();
	_MCFCRT_SignalOnceFlagAsFinished(__flag);
	return 0;
}

#define __gthread_once         __MCFCRT_gthread_once

//-----------------------------------------------------------------------------
// Mutex
//-----------------------------------------------------------------------------
typedef _MCFCRT_Mutex __gthread_mutex_t;

#define __GTHREAD_MUTEX_INIT            { 0 }
#define __GTHREAD_MUTEX_INIT_FUNCTION   __gthread_mutex_init_function

__MCFCRT_GTHREAD_INLINE_OR_EXTERN void __MCFCRT_gthread_mutex_init_function(__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_InitializeMutex(__mutex);
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_mutex_destroy(__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	(void)__mutex;
	return 0;
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_mutex_trylock(__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	if(_MCFCRT_EXPECT_NOT(!_MCFCRT_WaitForMutex(__mutex, _MCFCRT_MUTEX_SUGGESTED_SPIN_COUNT, 0))){
		return EBUSY;
	}
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_mutex_lock(__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_WaitForMutexForever(__mutex, _MCFCRT_MUTEX_SUGGESTED_SPIN_COUNT);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_mutex_unlock(__gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_SignalMutex(__mutex);
	return 0;
}

#define __gthread_mutex_init_function  __MCFCRT_gthread_mutex_init_function
#define __gthread_mutex_destroy        __MCFCRT_gthread_mutex_destroy

#define __gthread_mutex_trylock        __MCFCRT_gthread_mutex_trylock
#define __gthread_mutex_lock           __MCFCRT_gthread_mutex_lock
#define __gthread_mutex_unlock         __MCFCRT_gthread_mutex_unlock

//-----------------------------------------------------------------------------
// Recursive mutex
//-----------------------------------------------------------------------------
typedef struct __MCFCRT_tagGthreadRecursiveMutex {
	volatile _MCFCRT_STD uintptr_t __owner;
	_MCFCRT_STD size_t __count;
	__gthread_mutex_t __mutex;
} __gthread_recursive_mutex_t;

#define __GTHREAD_RECURSIVE_MUTEX_INIT            { 0, 0, __GTHREAD_MUTEX_INIT }
#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION   __gthread_recursive_mutex_init_function

__MCFCRT_GTHREAD_INLINE_OR_EXTERN void __MCFCRT_gthread_recursive_mutex_init_function(__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	__recur_mutex->__owner = 0;
	__recur_mutex->__count = 0;
	__gthread_mutex_init_function(&(__recur_mutex->__mutex)); // This guarantees release semantics so we need no atomic operations above.
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_recursive_mutex_destroy(__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	__gthread_mutex_destroy(&(__recur_mutex->__mutex));
	return 0;
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_recursive_mutex_trylock(__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	const _MCFCRT_STD uintptr_t __self = _MCFCRT_GetCurrentThreadId();
	const _MCFCRT_STD uintptr_t __old_owner = __atomic_load_n(&(__recur_mutex->__owner), __ATOMIC_RELAXED);
	if(_MCFCRT_EXPECT_NOT(__old_owner != __self)){
		const int __error = __gthread_mutex_trylock(&(__recur_mutex->__mutex));
		if(__error != 0){
			return __error;
		}
		__atomic_store_n(&(__recur_mutex->__owner), __self, __ATOMIC_RELAXED);
	}
	const _MCFCRT_STD size_t __new_count = ++__recur_mutex->__count;
	_MCFCRT_ASSERT(__new_count != 0);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_recursive_mutex_lock(__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	const _MCFCRT_STD uintptr_t __self = _MCFCRT_GetCurrentThreadId();
	const _MCFCRT_STD uintptr_t __old_owner = __atomic_load_n(&(__recur_mutex->__owner), __ATOMIC_RELAXED);
	if(_MCFCRT_EXPECT_NOT(__old_owner != __self)){
		__gthread_mutex_lock(&(__recur_mutex->__mutex));
		__atomic_store_n(&(__recur_mutex->__owner), __self, __ATOMIC_RELAXED);
	}
	const _MCFCRT_STD size_t __new_count = ++__recur_mutex->__count;
	_MCFCRT_ASSERT(__new_count != 0);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_recursive_mutex_unlock(__gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_DEBUG_CHECK(_MCFCRT_GetCurrentThreadId() == __atomic_load_n(&(__recur_mutex->__owner), __ATOMIC_RELAXED));
	const _MCFCRT_STD size_t __new_count = --__recur_mutex->__count;
	if(_MCFCRT_EXPECT_NOT(__new_count == 0)){
		__atomic_store_n(&(__recur_mutex->__owner), 0, __ATOMIC_RELAXED);
		__gthread_mutex_unlock(&(__recur_mutex->__mutex));
	}
	return 0;
}

#define __gthread_recursive_mutex_init_function   __MCFCRT_gthread_recursive_mutex_init_function
#define __gthread_recursive_mutex_destroy         __MCFCRT_gthread_recursive_mutex_destroy

#define __gthread_recursive_mutex_trylock         __MCFCRT_gthread_recursive_mutex_trylock
#define __gthread_recursive_mutex_lock            __MCFCRT_gthread_recursive_mutex_lock
#define __gthread_recursive_mutex_unlock          __MCFCRT_gthread_recursive_mutex_unlock

//-----------------------------------------------------------------------------
// Condition variable
//-----------------------------------------------------------------------------
#define __GTHREAD_HAS_COND 1

typedef _MCFCRT_ConditionVariable __gthread_cond_t;

#define __GTHREAD_COND_INIT             { 0 }
#define __GTHREAD_COND_INIT_FUNCTION    __gthread_cond_init_function

extern _MCFCRT_STD intptr_t __MCFCRT_GthreadUnlockCallbackMutex(_MCFCRT_STD intptr_t __context) _MCFCRT_NOEXCEPT;
extern void __MCFCRT_GthreadRelockCallbackMutex(_MCFCRT_STD intptr_t __context, _MCFCRT_STD intptr_t __unlocked) _MCFCRT_NOEXCEPT;

extern _MCFCRT_STD intptr_t __MCFCRT_GthreadUnlockCallbackRecursiveMutex(_MCFCRT_STD intptr_t __context) _MCFCRT_NOEXCEPT;
extern void __MCFCRT_GthreadRelockCallbackRecursiveMutex(_MCFCRT_STD intptr_t __context, _MCFCRT_STD intptr_t __unlocked) _MCFCRT_NOEXCEPT;

__MCFCRT_GTHREAD_INLINE_OR_EXTERN void __MCFCRT_gthread_cond_init_function(__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT {
	_MCFCRT_InitializeConditionVariable(__cond);
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_destroy(__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT {
	(void)__cond;
	return 0;
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_wait(__gthread_cond_t *__cond, __gthread_mutex_t *__mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_WaitForConditionVariableForever(__cond, &__MCFCRT_GthreadUnlockCallbackMutex, &__MCFCRT_GthreadRelockCallbackMutex, (_MCFCRT_STD intptr_t)__mutex, _MCFCRT_CONDITION_VARIABLE_SUGGESTED_SPIN_COUNT);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_wait_recursive(__gthread_cond_t *__cond, __gthread_recursive_mutex_t *__recur_mutex) _MCFCRT_NOEXCEPT {
	_MCFCRT_WaitForConditionVariableForever(__cond, &__MCFCRT_GthreadUnlockCallbackRecursiveMutex, &__MCFCRT_GthreadRelockCallbackRecursiveMutex, (_MCFCRT_STD intptr_t)__recur_mutex, _MCFCRT_CONDITION_VARIABLE_SUGGESTED_SPIN_COUNT);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_signal(__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT {
	_MCFCRT_SignalConditionVariable(__cond, 1);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_broadcast(__gthread_cond_t *__cond) _MCFCRT_NOEXCEPT {
	_MCFCRT_BroadcastConditionVariable(__cond);
	return 0;
}

#define __gthread_cond_init_function   __MCFCRT_gthread_cond_init_function
#define __gthread_cond_destroy         __MCFCRT_gthread_cond_destroy

#define __gthread_cond_wait            __MCFCRT_gthread_cond_wait
#define __gthread_cond_wait_recursive  __MCFCRT_gthread_cond_wait_recursive
#define __gthread_cond_signal          __MCFCRT_gthread_cond_signal
#define __gthread_cond_broadcast       __MCFCRT_gthread_cond_broadcast

//-----------------------------------------------------------------------------
// Thread
//-----------------------------------------------------------------------------
#define __GTHREADS_CXX0X 1

typedef _MCFCRT_STD uintptr_t __gthread_t;

typedef struct __MCFCRT_tagGthreadControlBlock {
	void *(*__proc)(void *);
	void *__param;
	void *__exit_code;
} __MCFCRT_GthreadControlBlock;

extern void __MCFCRT_GthreadMopWrapper(void *__params) _MCFCRT_NOEXCEPT;

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_create(__gthread_t *__tid_ret, void *(*__proc)(void *), void *__param) _MCFCRT_NOEXCEPT {
	__MCFCRT_GthreadControlBlock __control = { __proc, __param, (void *)0xDEADBEEF };
	const _MCFCRT_STD uintptr_t __tid = __MCFCRT_MopthreadCreate(&__MCFCRT_GthreadMopWrapper, &__control, sizeof(__control));
	if(__tid == 0){
		return EAGAIN;
	}
	*__tid_ret = __tid;
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_join(__gthread_t __tid, void **_MCFCRT_RESTRICT __exit_code_ret) _MCFCRT_NOEXCEPT {
	if(__tid == _MCFCRT_GetCurrentThreadId()){
		return EDEADLK;
	}
	if(__exit_code_ret){
		__MCFCRT_GthreadControlBlock __control;
		if(!__MCFCRT_MopthreadJoin(__tid, &__control)){
			return ESRCH;
		}
		*__exit_code_ret = __control.__exit_code;
	} else {
		if(!__MCFCRT_MopthreadJoin(__tid, _MCFCRT_NULLPTR)){
			return ESRCH;
		}
	}
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_detach(__gthread_t __tid) _MCFCRT_NOEXCEPT {
	if(__tid == _MCFCRT_GetCurrentThreadId()){
		return EDEADLK;
	}
	if(!__MCFCRT_MopthreadDetach(__tid)){
		return ESRCH;
	}
	return 0;
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_equal(__gthread_t __tid1, __gthread_t __tid2) _MCFCRT_NOEXCEPT {
	return __tid1 == __tid2;
}
__attribute__((__const__))
__MCFCRT_GTHREAD_INLINE_OR_EXTERN __gthread_t __MCFCRT_gthread_self(void) _MCFCRT_NOEXCEPT {
	return _MCFCRT_GetCurrentThreadId();
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_yield(void) _MCFCRT_NOEXCEPT {
	_MCFCRT_YieldThread();
	return 0;
}

#define __gthread_create   __MCFCRT_gthread_create
#define __gthread_join     __MCFCRT_gthread_join
#define __gthread_detach   __MCFCRT_gthread_detach

#define __gthread_equal    __MCFCRT_gthread_equal
#define __gthread_self     __MCFCRT_gthread_self
#define __gthread_yield    __MCFCRT_gthread_yield

//-----------------------------------------------------------------------------
// Timed functions
//-----------------------------------------------------------------------------
typedef struct timespec __gthread_time_t;

__MCFCRT_GTHREAD_INLINE_OR_EXTERN _MCFCRT_STD uint64_t __MCFCRT_GthreadTranslateTimeout(const __gthread_time_t *_MCFCRT_RESTRICT __utc_timeout) _MCFCRT_NOEXCEPT {
	const double __utc_timeout_ms = (double)__utc_timeout->tv_sec * 1.0e3 + (double)__utc_timeout->tv_nsec / 1.0e6;
	const double __utc_now_ms = (double)_MCFCRT_GetUtcClock();
	const double __delta_ms = __utc_timeout_ms - __utc_now_ms;
	if(__delta_ms <= 0){
		return 0;
	}
	const _MCFCRT_STD uint64_t __mono_now_ms = _MCFCRT_GetFastMonoClock();
	// XXX: We assume the value returned by _MCFCRT_GetFastMonoClock() is the number of milliseconds since the system starts, which can hardly be greater than 0x1p48.
	//      All current implementations, especially clock_gettime() on Linux, are specified as that. This isn't guaranteed behavior nevertheless.
	const _MCFCRT_STD uint64_t __complement_ms = (1ull << 48) - 1 - __mono_now_ms;
	// Cast the uint64_t to int64_t for performance reasons. The compiler would otherwise produce more code since unsigned types can't be processed by x87 directly.
	if(__delta_ms >= (double)(_MCFCRT_STD int64_t)__complement_ms){
		return (_MCFCRT_STD uint64_t)-1;
	}
	return __mono_now_ms + (_MCFCRT_STD uint64_t)(_MCFCRT_STD int64_t)(__delta_ms + 0.999999);
}

__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_mutex_timedlock(__gthread_mutex_t *_MCFCRT_RESTRICT __mutex, const __gthread_time_t *_MCFCRT_RESTRICT __timeout) _MCFCRT_NOEXCEPT {
	const _MCFCRT_STD uint64_t __mono_timeout_ms = __MCFCRT_GthreadTranslateTimeout(__timeout);
	if(!_MCFCRT_WaitForMutex(__mutex, _MCFCRT_MUTEX_SUGGESTED_SPIN_COUNT, __mono_timeout_ms)){
		return ETIMEDOUT;
	}
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_recursive_mutex_timedlock(__gthread_recursive_mutex_t *_MCFCRT_RESTRICT __recur_mutex, const __gthread_time_t *_MCFCRT_RESTRICT __timeout) _MCFCRT_NOEXCEPT {
	const _MCFCRT_STD uintptr_t __self = _MCFCRT_GetCurrentThreadId();
	const _MCFCRT_STD uintptr_t __old_owner = __atomic_load_n(&(__recur_mutex->__owner), __ATOMIC_RELAXED);
	if(_MCFCRT_EXPECT_NOT(__old_owner != __self)){
		const int __error = __MCFCRT_gthread_mutex_timedlock(&(__recur_mutex->__mutex), __timeout);
		if(__error != 0){
			return __error;
		}
		__atomic_store_n(&(__recur_mutex->__owner), __self, __ATOMIC_RELAXED);
	}
	const _MCFCRT_STD size_t __new_count = ++__recur_mutex->__count;
	_MCFCRT_ASSERT(__new_count != 0);
	return 0;
}
__MCFCRT_GTHREAD_INLINE_OR_EXTERN int __MCFCRT_gthread_cond_timedwait(__gthread_cond_t *_MCFCRT_RESTRICT __cond, __gthread_mutex_t *_MCFCRT_RESTRICT __mutex, const __gthread_time_t *_MCFCRT_RESTRICT __timeout) _MCFCRT_NOEXCEPT {
	const _MCFCRT_STD uint64_t __mono_timeout_ms = __MCFCRT_GthreadTranslateTimeout(__timeout);
	if(!_MCFCRT_WaitForConditionVariable(__cond, &__MCFCRT_GthreadUnlockCallbackMutex, &__MCFCRT_GthreadRelockCallbackMutex, (_MCFCRT_STD intptr_t)__mutex, _MCFCRT_CONDITION_VARIABLE_SUGGESTED_SPIN_COUNT, __mono_timeout_ms)){
		return ETIMEDOUT;
	}
	return 0;
}

#define __gthread_mutex_timedlock            __MCFCRT_gthread_mutex_timedlock
#define __gthread_recursive_mutex_timedlock  __MCFCRT_gthread_recursive_mutex_timedlock
#define __gthread_cond_timedwait             __MCFCRT_gthread_cond_timedwait

_MCFCRT_EXTERN_C_END

#endif
