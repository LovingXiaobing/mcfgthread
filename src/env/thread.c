// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "thread.h"
#include "_nt_timeout.h"
#include "crtassert.h"
#include "mcfwin.h"
#include <winternl.h>
#include <ntdef.h>

#undef GetCurrentProcess
#define GetCurrentProcess()  ((HANDLE)-1)

#undef GetCurrentThread
#define GetCurrentThread()   ((HANDLE)-2)

__attribute__((__dllimport__, __stdcall__))
NTSTATUS RtlCreateUserThread(HANDLE hProcess, const SECURITY_DESCRIPTOR *pSecurityDescriptor, BOOLEAN bSuspended,
	ULONG ulStackZeroBits, ULONG *pulStackReserved, ULONG *pulStackCommitted, PTHREAD_START_ROUTINE pfnThreadProc, void *pParam, HANDLE *pHandle, CLIENT_ID *pClientId);

__attribute__((__dllimport__, __stdcall__))
extern NTSTATUS NtDelayExecution(BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout);
__attribute__((__dllimport__, __stdcall__))
extern NTSTATUS NtYieldExecution(void);

__attribute__((__dllimport__, __stdcall__))
extern NTSTATUS NtSuspendThread(HANDLE hThread, LONG *plPrevCount);
__attribute__((__dllimport__, __stdcall__))
extern NTSTATUS NtResumeThread(HANDLE hThread, LONG *plPrevCount);

_MCFCRT_ThreadHandle _MCFCRT_CreateNativeThread(_MCFCRT_NativeThreadProc pfnThreadProc, void *pParam, bool bSuspended, uintptr_t *restrict puThreadId){
	HANDLE hThread;
	CLIENT_ID vClientId;
	const NTSTATUS lStatus = RtlCreateUserThread(GetCurrentProcess(), _MCFCRT_NULLPTR, bSuspended, 0, _MCFCRT_NULLPTR, _MCFCRT_NULLPTR, pfnThreadProc, pParam, &hThread, &vClientId);
	if(!NT_SUCCESS(lStatus)){
		SetLastError(RtlNtStatusToDosError(lStatus));
		return _MCFCRT_NULLPTR;
	}
	if(puThreadId){
		*puThreadId = (uintptr_t)vClientId.UniqueThread;
	}
	return (_MCFCRT_ThreadHandle)hThread;
}
void _MCFCRT_CloseThread(_MCFCRT_ThreadHandle hThread){
	const NTSTATUS lStatus = NtClose((HANDLE)hThread);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtClose() failed.");
}

void _MCFCRT_Sleep(uint64_t u64UntilFastMonoClock){
	LARGE_INTEGER liTimeout;
	__MCFCRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
	const NTSTATUS lStatus = NtDelayExecution(false, &liTimeout);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtDelayExecution() failed.");
}
bool _MCFCRT_AlertableSleep(uint64_t u64UntilFastMonoClock){
	LARGE_INTEGER liTimeout;
	__MCFCRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
	const NTSTATUS lStatus = NtDelayExecution(true, &liTimeout);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtDelayExecution() failed.");
	if(lStatus == STATUS_TIMEOUT){
		return false;
	}
	return true;
}
void _MCFCRT_AlertableSleepForever(void){
	LARGE_INTEGER liTimeout;
	liTimeout.QuadPart = INT64_MAX;
	const NTSTATUS lStatus = NtDelayExecution(true, &liTimeout);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtDelayExecution() failed.");
}
void _MCFCRT_YieldThread(void){
	const NTSTATUS lStatus = NtYieldExecution();
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtYieldExecution() failed.");
}

long _MCFCRT_SuspendThread(_MCFCRT_ThreadHandle hThread){
	LONG lPrevCount;
	const NTSTATUS lStatus = NtSuspendThread((HANDLE)hThread, &lPrevCount);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtSuspendThread() failed.");
	return lPrevCount;
}
long _MCFCRT_ResumeThread(_MCFCRT_ThreadHandle hThread){
	LONG lPrevCount;
	const NTSTATUS lStatus = NtResumeThread((HANDLE)hThread, &lPrevCount);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtResumeThread() failed.");
	return lPrevCount;
}

bool _MCFCRT_WaitForThread(_MCFCRT_ThreadHandle hThread, uint64_t u64UntilFastMonoClock){
	LARGE_INTEGER liTimeout;
	__MCFCRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
	const NTSTATUS lStatus = NtWaitForSingleObject((HANDLE)hThread, false, &liTimeout);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForSingleObject() failed.");
	if(lStatus == STATUS_TIMEOUT){
		return false;
	}
	return true;
}
void _MCFCRT_WaitForThreadForever(_MCFCRT_ThreadHandle hThread){
	const NTSTATUS lStatus = NtWaitForSingleObject((HANDLE)hThread, false, _MCFCRT_NULLPTR);
	_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForSingleObject() failed.");
}

uintptr_t _MCFCRT_GetCurrentThreadId(void){
	return GetCurrentThreadId();
}
