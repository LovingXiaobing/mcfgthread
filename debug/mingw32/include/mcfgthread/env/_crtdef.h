// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_CRTDEF_H_
#define __MCFCRT_ENV_CRTDEF_H_

#ifdef __STDC_VERSION__
#  if __STDC_VERSION__ >= 199409l
#    define _MCFCRT_C95                1
#  endif
#  if __STDC_VERSION__ >= 199901l
#    define _MCFCRT_C99                1
#  endif
#  if __STDC_VERSION__ >= 201112l
#    define _MCFCRT_C11                1
#  endif
#endif

#ifdef __cplusplus
#  if __cplusplus >= 199711l
#    define _MCFCRT_CXX98              1
#  endif
#  if __cplusplus >= 201103l
#    define _MCFCRT_CXX11              1
#  endif
#  if __cplusplus >= 201402l
#    define _MCFCRT_CXX14              1
#  endif
#  if __cplusplus >= 201703l
#    define _MCFCRT_CXX17              1
#  endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <uchar.h>
#include <assert.h>

#ifdef __cplusplus
#  define _MCFCRT_EXTERN_C_BEGIN       extern "C" {
#  define _MCFCRT_EXTERN_C_END         }
#  define _MCFCRT_STD                  ::
#else
#  define _MCFCRT_EXTERN_C_BEGIN       //
#  define _MCFCRT_EXTERN_C_END         //
#  define _MCFCRT_STD                  //
#endif

#ifdef _WIN64
#  define __MCFCRT_C_CALLBACK_DECL     __attribute__((__nothrow__, __aligned__(16)))
#else
#  define __MCFCRT_C_CALLBACK_DECL     __attribute__((__nothrow__, __force_align_arg_pointer__, __aligned__(8)))
#endif

#define __MCFCRT_C_CDECL               __attribute__((__cdecl__))    __MCFCRT_C_CALLBACK_DECL
#define __MCFCRT_C_STDCALL             __attribute__((__stdcall__))  __MCFCRT_C_CALLBACK_DECL
#define __MCFCRT_C_FASTCALL            __attribute__((__fastcall__)) __MCFCRT_C_CALLBACK_DECL

#if defined(_MCFCRT_CXX11)
#  define _MCFCRT_NOEXCEPT             noexcept
#  define _MCFCRT_CONSTEXPR            constexpr
#  define _MCFCRT_RESTRICT             __restrict__
#  define _MCFCRT_NULLPTR              nullptr
#elif defined(__cplusplus)
#  define _MCFCRT_NOEXCEPT             throw()
#  define _MCFCRT_CONSTEXPR            //
#  define _MCFCRT_RESTRICT             __restrict__
#  define _MCFCRT_NULLPTR              ((__INTPTR_TYPE__)0)
#else
#  define _MCFCRT_NOEXCEPT             //
#  define _MCFCRT_CONSTEXPR            //
#  define _MCFCRT_RESTRICT             restrict
#  define _MCFCRT_NULLPTR              ((void *)0)
#endif

#endif
