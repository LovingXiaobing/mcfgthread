// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "mcfcrt.h"
#include "env/_seh_top.h"
#include "env/tls.h"

__MCFCRT_C_STDCALL
extern BOOL __MCFCRT_DllStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
	__asm__("@__MCFCRT_DllStartup");

__MCFCRT_C_STDCALL
BOOL __MCFCRT_DllStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved){
	(void)hInstance, (void)pReserved;

	bool bRet = true;

	__MCFCRT_SEH_TOP_BEGIN
	{
		switch(dwReason){
		case DLL_PROCESS_ATTACH:
			if(!__MCFCRT_InitRecursive()){
				bRet = false;
				break;
			}
			if(!__MCFCRT_ModuleInit()){
				__MCFCRT_UninitRecursive();
				bRet = false;
				break;
			}
			break;
		case DLL_THREAD_DETACH:
			__MCFCRT_TlsCleanup();
			break;
		case DLL_PROCESS_DETACH:
			__MCFCRT_TlsCleanup();
			__MCFCRT_ModuleUninit();
			__MCFCRT_UninitRecursive();
			break;
		}
	}
	__MCFCRT_SEH_TOP_END

	return bRet;
}
