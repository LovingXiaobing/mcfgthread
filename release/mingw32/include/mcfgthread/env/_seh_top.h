// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_SEH_TOP_H_
#define __MCFCRT_ENV_SEH_TOP_H_

#include "_crtdef.h"
#include "mcfwin.h"

_MCFCRT_EXTERN_C_BEGIN

__MCFCRT_C_CDECL
extern EXCEPTION_DISPOSITION __MCFCRT_SehTopDispatcher(EXCEPTION_RECORD *__pRecord, void *__pEstablisherFrame, CONTEXT *__pContext, void *__pDispatcherContext) _MCFCRT_NOEXCEPT;

__attribute__((__unused__, __always_inline__))
static inline void __MCFCRT_SehTopInstaller_x86(void *__seh_node) _MCFCRT_NOEXCEPT {
	void *__seh_unused;
	__asm__ volatile (
		"mov %0, dword ptr fs:[0] \n"
		"mov dword ptr[%1], %0 \n"
		"mov dword ptr[%1 + 4], offset ___MCFCRT_SehTopDispatcher \n"
		"mov dword ptr fs:[0], %1 \n"
		: "=&r"(__seh_unused) : "r"(__seh_node)
	);
}
__attribute__((__unused__, __always_inline__))
static inline void __MCFCRT_SehTopUninstaller_x86(void *__seh_node) _MCFCRT_NOEXCEPT {
	void *__seh_unused;
	__asm__ volatile (
		"mov %0, dword ptr[%1] \n"
		"mov dword ptr fs:[0], %0 \n"
		: "=&r"(__seh_unused) : "r"(__seh_node)
	);
}

_MCFCRT_EXTERN_C_END

#ifdef _WIN64
#  define __MCFCRT_SEH_TOP_BEGIN        { __asm__ volatile (	\
	                                        ".seh_handler __MCFCRT_SehTopDispatcher, @except \n");	\
	                                      {
#  define __MCFCRT_SEH_TOP_END            }	\
	                                    }
#else
#  define __MCFCRT_SEH_TOP_BEGIN        { void *__MCFCRT_seh_node[2]	\
	                                        __attribute__((__cleanup__(__MCFCRT_SehTopUninstaller_x86)));	\
	                                      __MCFCRT_SehTopInstaller_x86(&__MCFCRT_seh_node);	\
	                                      {
#  define __MCFCRT_SEH_TOP_END            }	\
	                                    }
#endif

#endif
