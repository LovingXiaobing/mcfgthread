// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#define __MCFCRT_C11THREAD_INLINE_OR_EXTERN     extern inline
#include "c11thread.h"

void __MCFCRT_c11thread_tls_destructor(intptr_t context, void *storage){
	void (*const destructor)(void *) = (void (*)(void *))context;

	void *const value = *(void **)storage;
	if(!value){
		return;
	}
	*(void **)storage = _MCFCRT_NULLPTR;
	(*destructor)(value);
}

intptr_t __MCFCRT_c11thread_unlock_callback_mutex(intptr_t context){
	mtx_t *const mutex = (mtx_t *)context;

	if(mutex->__mask & mtx_recursive){
		_MCFCRT_ASSERT(_MCFCRT_GetCurrentThreadId() == __atomic_load_n(&(mutex->__owner), __ATOMIC_RELAXED));

		const size_t old_count = mutex->__count;
		mutex->__count = 0;
		__atomic_store_n(&(mutex->__owner), 0, __ATOMIC_RELAXED);

		_MCFCRT_SignalMutex(&(mutex->__mutex));
		return (intptr_t)old_count;
	} else {
		_MCFCRT_SignalMutex(&(mutex->__mutex));
		return 1;
	}
}
void __MCFCRT_c11thread_relock_callback_mutex(intptr_t context, intptr_t unlocked){
	mtx_t *const mutex = (mtx_t *)context;

	if(mutex->__mask & mtx_recursive){
		_MCFCRT_ASSERT((size_t)unlocked >= 1);
		_MCFCRT_WaitForMutexForever(&(mutex->__mutex), _MCFCRT_MUTEX_SUGGESTED_SPIN_COUNT);

		const uintptr_t self = _MCFCRT_GetCurrentThreadId();
		__atomic_store_n(&(mutex->__owner), self, __ATOMIC_RELAXED);
		mutex->__count = (size_t)unlocked;
	} else {
		_MCFCRT_ASSERT((size_t)unlocked == 1);
		_MCFCRT_WaitForMutexForever(&(mutex->__mutex), _MCFCRT_MUTEX_SUGGESTED_SPIN_COUNT);
	}
}

void __MCFCRT_c11thread_mopthread_wrapper(void *params){
	__MCFCRT_c11thread_control_t *const control = params;

	const int exit_code = (*(control->__proc))(control->__param);
	control->__exit_code = exit_code;
}
void __MCFCRT_c11thread_mopthread_exit_modifier(void *params, size_t size_of_params, intptr_t context){
	_MCFCRT_ASSERT(size_of_params >= sizeof(__MCFCRT_c11thread_control_t));

	__MCFCRT_c11thread_control_t *const control = params;

	control->__exit_code = (int)context;
}
