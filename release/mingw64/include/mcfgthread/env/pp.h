// This file is part of MCFCRT.
// See MCFLicense.txt for licensing information.
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_PP_H_
#define __MCFCRT_ENV_PP_H_

#define _MCFCRT_PP_STRINGIFY(...)             #__VA_ARGS__

#define _MCFCRT_PP_WIDEN(__x_)                L##__x_
#define _MCFCRT_PP_UTF8(__x_)                 u8##__x_
#define _MCFCRT_PP_UTF16(__x_)                u##__x_
#define _MCFCRT_PP_UTF32(__x_)                U##__x_

#define _MCFCRT_PP_FIRST(__f_, ...)           __f_
#define _MCFCRT_PP_REST(__f_, ...)            __VA_ARGS__

#define _MCFCRT_PP_CAT2(__x_, __y_)           __x_##__y_
#define _MCFCRT_PP_CAT3(__x_, __y_, __z_)     __x_##__y_##__z_

#define _MCFCRT_PP_LAZY(__f_, ...)            __f_(__VA_ARGS__)

#endif
