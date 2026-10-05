/*
 * stdint.h,v
 *
 * Revision 1.1  2026/09/02  ChatGPT modifications (JJ)
 *
 *    Add C99 fixed-width integer compatibility for ixemul v80.
 *    Reuse the m68k integer types already supplied by machine/types.h
 *    instead of redefining int8_t, int16_t, int32_t and int64_t.
 *    Add the standard uint*_t, least-width, fast-width, maximum-width
 *    and pointer-capable integer types.
 *    Add matching limits and integer-constant macros for the ixemul
 *    m68k ABI and GCC 2.95.x.
 *    Match SIZE_MAX, PTRDIFF_*, WCHAR_* and SIG_ATOMIC_* to ixemul's
 *    existing machine/ansi.h and machine/signal.h type definitions.
 */

#ifndef _STDINT_H
#define _STDINT_H

/*
 * ixemul's <machine/types.h> already provides:
 *
 *   int8_t,  int16_t,  int32_t,  int64_t
 *   u_int8_t, u_int16_t, u_int32_t, u_int64_t
 *
 * It also defines __BIT_TYPES_DEFINED__.  Always use those definitions
 * so that <stdint.h> can be included either before or after <sys/types.h>
 * without creating duplicate typedefs.
 */
#include <machine/types.h>

/* Exact-width unsigned integer types. */
typedef u_int8_t  uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;

/* Minimum-width integer types. */
typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t  int_least32_t;
typedef uint32_t uint_least32_t;
typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;

/*
 * Fastest minimum-width integer types.
 *
 * On ixemul/m68k, int is 32 bits, so use 32-bit int for the 8-, 16-
 * and 32-bit fast types.  The 64-bit fast type uses long long.
 */
typedef int32_t  int_fast8_t;
typedef uint32_t uint_fast8_t;
typedef int32_t  int_fast16_t;
typedef uint32_t uint_fast16_t;
typedef int32_t  int_fast32_t;
typedef uint32_t uint_fast32_t;
typedef int64_t  int_fast64_t;
typedef uint64_t uint_fast64_t;

/* Greatest-width integer types. */
typedef int64_t  intmax_t;
typedef uint64_t uintmax_t;

/*
 * Integer types capable of holding object pointers.
 * ixemul/m68k uses 32-bit pointers and 32-bit long.
 */
typedef long          intptr_t;
typedef unsigned long uintptr_t;

/* Exact-width limits. */
#define INT8_MIN        (-128)
#define INT8_MAX        127
#define UINT8_MAX       255

#define INT16_MIN       (-32767 - 1)
#define INT16_MAX       32767
#define UINT16_MAX      65535

#define INT32_MIN       (-2147483647 - 1)
#define INT32_MAX       2147483647
#define UINT32_MAX      4294967295U

#define INT64_MIN       (-9223372036854775807LL - 1LL)
#define INT64_MAX       9223372036854775807LL
#define UINT64_MAX      18446744073709551615ULL

/* Minimum-width limits. */
#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define UINT_LEAST8_MAX  UINT8_MAX

#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST16_MAX  INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX

#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST32_MAX  INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX

#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* Fast-width limits. */
#define INT_FAST8_MIN    INT32_MIN
#define INT_FAST8_MAX    INT32_MAX
#define UINT_FAST8_MAX   UINT32_MAX

#define INT_FAST16_MIN   INT32_MIN
#define INT_FAST16_MAX   INT32_MAX
#define UINT_FAST16_MAX  UINT32_MAX

#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST32_MAX   INT32_MAX
#define UINT_FAST32_MAX  UINT32_MAX

#define INT_FAST64_MIN   INT64_MIN
#define INT_FAST64_MAX   INT64_MAX
#define UINT_FAST64_MAX  UINT64_MAX

/* Greatest-width limits. */
#define INTMAX_MIN       INT64_MIN
#define INTMAX_MAX       INT64_MAX
#define UINTMAX_MAX      UINT64_MAX

/* Pointer-capable integer limits. */
#define INTPTR_MIN       (-2147483647L - 1L)
#define INTPTR_MAX       2147483647L
#define UINTPTR_MAX      4294967295UL

/*
 * Limits for related standard integer types already defined by ixemul.
 *
 * ptrdiff_t    -> int
 * size_t       -> unsigned long
 * wchar_t      -> int
 * sig_atomic_t -> int
 */
#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN      INT32_MIN
#endif
#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX      INT32_MAX
#endif

#ifndef SIZE_MAX
#define SIZE_MAX         4294967295UL
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN        INT32_MIN
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX        INT32_MAX
#endif

#ifndef SIG_ATOMIC_MIN
#define SIG_ATOMIC_MIN   INT32_MIN
#endif
#ifndef SIG_ATOMIC_MAX
#define SIG_ATOMIC_MAX   INT32_MAX
#endif

/*
 * Integer constant macros.
 *
 * For 8- and 16-bit types, integer promotion yields int on this target.
 * 32-bit unsigned constants require U; 64-bit constants use GCC's
 * long long suffixes, already used by ixemul's m68k type definitions.
 */
#define INT8_C(c)        c
#define UINT8_C(c)       c
#define INT16_C(c)       c
#define UINT16_C(c)      c
#define INT32_C(c)       c
#define UINT32_C(c)      c ## U
#define INT64_C(c)       c ## LL
#define UINT64_C(c)      c ## ULL

#define INTMAX_C(c)      c ## LL
#define UINTMAX_C(c)     c ## ULL

/*
 * WINT_MIN/WINT_MAX are intentionally not defined here.  The reviewed
 * ixemul v80 headers do not provide a wint_t definition, so advertising
 * limits for a non-existent implementation type would be misleading.
 */

#endif /* _STDINT_H */
