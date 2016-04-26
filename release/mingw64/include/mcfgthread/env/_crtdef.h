// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_CRTDEF_H_
#define __MCFCRT_ENV_CRTDEF_H_

#if defined(__cplusplus) && __cplusplus >= 201402l
#	define _MCFCRT_CXX14                1
#endif

#if defined(__cplusplus) && __cplusplus >= 201103l
#	define _MCFCRT_CXX11                1
#endif

#ifdef __cplusplus
#	include <cstddef>
#	include <climits>
#	include <cassert>
#	define _MCFCRT_EXTERN_C_BEGIN       extern "C" {
#	define _MCFCRT_EXTERN_C_END         }
#	define _MCFCRT_STD                  ::std::
#else
#	include <stddef.h>
#	include <limits.h>
#	include <stdbool.h>
#	include <stdalign.h>
#	include <assert.h>
#	define _MCFCRT_EXTERN_C_BEGIN
#	define _MCFCRT_EXTERN_C_END
#	define _MCFCRT_STD                  //
#endif

#if defined(_MCFCRT_CXX11)
#	include <cstdint>
#elif defined(__cplusplus)
#	include <stdint.h>
// Assume the types required are in the global namespace. This is not only a dirty workaround but also our last hope.
#	undef _MCFCRT_STD
#	define _MCFCRT_STD                   ::
#else
#	include <stdint.h>
#endif

#define restrict                        __restrict__

#ifndef _MCFCRT_CXX11
#	define nullptr                      0
#endif

#define __MCFCRT_C_CALLBACK_DECL        __attribute__((__nothrow__, __force_align_arg_pointer__, __aligned__(16)))
#define __MCFCRT_C_CDECL                __attribute__((__cdecl__))    __MCFCRT_C_CALLBACK_DECL
#define __MCFCRT_C_STDCALL              __attribute__((__stdcall__))  __MCFCRT_C_CALLBACK_DECL
#define __MCFCRT_C_FASTCALL             __attribute__((__fastcall__)) __MCFCRT_C_CALLBACK_DECL

#if defined(_MCFCRT_CXX11)
#	define _MCFCRT_NOEXCEPT             noexcept
#	define _MCFCRT_CONSTEXPR            constexpr
#elif defined(__cplusplus)
#	define _MCFCRT_NOEXCEPT             throw()
#	define _MCFCRT_CONSTEXPR            static inline
#else
#	define _MCFCRT_NOEXCEPT             //
#	define _MCFCRT_CONSTEXPR            static inline
#endif

#endif