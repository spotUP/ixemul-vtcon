/* <stdint.h> for ixemul (C99 7.18), on ixemul's own machine types.
 *
 * Added for building with bebbo's gcc 6: its NDK 3.2 <exec/types.h>
 * includes <stdint.h>, and without one here newlib's was found, which
 * pulled in newlib's <sys/features.h> (it defines _POSIX_SOURCE and hid
 * u_char, u_long, fd_set ... from ixemul's own <sys/types.h>). */
#ifndef _STDINT_H_
#define _STDINT_H_

#include <machine/types.h>

typedef u_int8_t  uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;

typedef int8_t   int_least8_t;
typedef int16_t  int_least16_t;
typedef int32_t  int_least32_t;
typedef int64_t  int_least64_t;
typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

typedef int32_t  int_fast8_t;
typedef int32_t  int_fast16_t;
typedef int32_t  int_fast32_t;
typedef int64_t  int_fast64_t;
typedef uint32_t uint_fast8_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

typedef long          intptr_t;
typedef unsigned long uintptr_t;
typedef int64_t       intmax_t;
typedef uint64_t      uintmax_t;

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)
#define INT8_MIN   (-128)
#define INT16_MIN  (-32767 - 1)
#define INT32_MIN  (-2147483647L - 1)
#define INT64_MIN  (-9223372036854775807LL - 1)
#define INT8_MAX   127
#define INT16_MAX  32767
#define INT32_MAX  2147483647L
#define INT64_MAX  9223372036854775807LL
#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295UL
#define UINT64_MAX 18446744073709551615ULL
#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define INTMAX_MIN  INT64_MIN
#define INTMAX_MAX  INT64_MAX
#define UINTMAX_MAX UINT64_MAX
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#define SIZE_MAX    UINT32_MAX
#endif

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)
#define INT8_C(v)   (v)
#define INT16_C(v)  (v)
#define INT32_C(v)  (v ## L)
#define INT64_C(v)  (v ## LL)
#define UINT8_C(v)  (v ## U)
#define UINT16_C(v) (v ## U)
#define UINT32_C(v) (v ## UL)
#define UINT64_C(v) (v ## ULL)
#define INTMAX_C(v)  (v ## LL)
#define UINTMAX_C(v) (v ## ULL)
#endif

#endif
