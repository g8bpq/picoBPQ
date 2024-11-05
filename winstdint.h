
#ifndef _STDINT_H
#define _STDINT_H
#define __need_wint_t
#define __need_wchar_t
#include <wchar.h>
#include <stddef.h>
 
#if _MSC_VER && (_MSC_VER < 1300)
/* using MSVC 6 or earlier - no "long long" type, but might have _int64 type */
#define __STDINT_LONGLONG           __int64
#define __STDINT_LONGLONG_SUFFIX    i64
#else
#define __STDINT_LONGLONG           long long
#define __STDINT_LONGLONG_SUFFIX    LL
#endif

 
/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef __STDINT_LONGLONG  int64_t;
typedef unsigned __STDINT_LONGLONG   uint64_t;
 
/* 7.18.1.2  Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char   uint_least8_t;
typedef short  int_least16_t;
typedef unsigned short  uint_least16_t;
typedef int  int_least32_t;
typedef unsigned   uint_least32_t;
typedef __STDINT_LONGLONG  int_least64_t;
typedef unsigned __STDINT_LONGLONG   uint_least64_t;
 
/*  7.18.1.3  Fastest minimum-width integer types 
 *  Not actually guaranteed to be fastest for all purposes
 *  Here we use the exact-width types for 8 and 16-bit ints. 
 */
typedef char int_fast8_t;
typedef unsigned char uint_fast8_t;
//typedef short  int_fast16_t;
//typedef unsigned short  uint_fast16_t;
typedef int  int_fast32_t;
typedef unsigned  int  uint_fast32_t;
typedef __STDINT_LONGLONG  int_fast64_t;
typedef unsigned __STDINT_LONGLONG   uint_fast64_t;
 
/* 7.18.1.4  Integer types capable of holding object pointers */
#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
#ifdef _WIN64
typedef __STDINT_LONGLONG intptr_t
#else
typedef int intptr_t;
#endif /* _WIN64 */
#endif /* _INTPTR_T_DEFINED */
 
#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifdef _WIN64
typedef unsigned __STDINT_LONGLONG uintptr_t
#else
typedef unsigned int uintptr_t;
#endif /* _WIN64 */
#endif /* _UINTPTR_T_DEFINED */
 
/* 7.18.1.5  Greatest-width integer types */
typedef __STDINT_LONGLONG  intmax_t;
typedef unsigned __STDINT_LONGLONG   uintmax_t;
#endif