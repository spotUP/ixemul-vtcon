/* Modified 2026-10-10 by spotUP for UP-Term (ixemul-vtcon); see the git history. */
/*
 * Copyright (c) 1985, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)math.h	5.8 (Berkeley) 4/2/91
 */

#ifndef	_MATH_H_
#define	_MATH_H_

#include <float.h>

#ifndef BITS
#define BITS(type)	(8 * (int)sizeof(type))
#endif

#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#include <sys/cdefs.h>

/* C99 7.12: fzy 1.0 scores with INFINITY (libixcompat, UP-Term) */
#if defined(__GNUC__)
#ifndef INFINITY
#define INFINITY	__builtin_inff()
#endif
#ifndef NAN
#define NAN		__builtin_nanf("")
#endif
#ifndef HUGE_VALF
#define HUGE_VALF	__builtin_inff()
#endif
#ifndef HUGE_VALL
#define HUGE_VALL	__builtin_infl()
#endif
/* C99 7.12.3 classification; FP_* values are what __builtin_fpclassify takes */
#ifndef FP_NAN
#define FP_NAN		0
#define FP_INFINITE	1
#define FP_ZERO		2
#define FP_SUBNORMAL	3
#define FP_NORMAL	4
#endif
/* C99 7.12.14, quiet comparisons (file 5.48's softmagic.c) */
#define isgreater(x, y)		__builtin_isgreater(x, y)
#define isgreaterequal(x, y)	__builtin_isgreaterequal(x, y)
#define isless(x, y)		__builtin_isless(x, y)
#define islessequal(x, y)	__builtin_islessequal(x, y)
#define islessgreater(x, y)	__builtin_islessgreater(x, y)
#define isunordered(x, y)	__builtin_isunordered(x, y)
#endif

/* __NO_MATH_INLINES (glibc's name): the library's own math sources, built
   for the FPU, define these functions and must not see the inlines */
#if (defined(__GNUC__) || defined(__cplusplus)) && defined(__HAVE_68881__) && !defined(__NO_MATH_INLINES)
#include "math-68881.h"
#else

#define	HUGE_VAL	1e500			/* IEEE: positive infinity */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#define HUGE		DBL_MAX
#endif

__BEGIN_DECLS
double	acos __P((double));
double	asin __P((double));
double	atan __P((double));
double	atan2 __P((double, double));
double	ceil __P((double));
double	cos __P((double));
double	cosh __P((double));
double	exp __P((double));
double	fabs __P((double));
double	floor __P((double));
double	fmod __P((double, double));
double	round __P((double));	/* libixcompat.a (UP-Term) */
double	frexp __P((double, int *));
double	ldexp __P((double, int));
double	log __P((double));
double	log10 __P((double));
double	modf __P((double, double *));
double	pow __P((double, double));
double	sin __P((double));
double	sinh __P((double));
double	sqrt __P((double));
double	tan __P((double));
double	tanh __P((double));

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
double	acosh __P((double));
double	asinh __P((double));
double	atanh __P((double));
double	cabs();		/* we can't describe cabs()'s argument properly */
double	cbrt __P((double));
double	copysign __P((double, double));
double	drem __P((double, double));
double	erf __P((double));
double	erfc __P((double));
double	expm1 __P((double));
int	finite __P((double));
double	hypot __P((double, double));
#if defined(vax) || defined(tahoe)
double	infnan __P((int));
#endif
double	j0 __P((double));
double	j1 __P((double));
double	jn __P((int, double));
double	lgamma __P((double));
double	log1p __P((double));
double	logb __P((double));
double	rint __P((double));
double	scalb __P((double, int));
double	y0 __P((double));
double	y1 __P((double));
double	yn __P((int, double));
#endif

__END_DECLS

#endif /* __HAVE_68881__ */

__BEGIN_DECLS
int	isinf __P((double));
int	isnan __P((double));
int	isnormal __P((double));
__END_DECLS

/*
 * C99 7.12: the double, float and long double functions libm.a (compat/libm.c,
 * test/libm_names.txt) provides and the lines above do not declare. Without
 * a prototype a call is an implicit int function, which returns garbage.
 * Guarded as cbrt and hypot are, so code that declares its own fmax etc.
 * under _ANSI_SOURCE/_POSIX_SOURCE still compiles.
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ == 2
typedef long double float_t;
typedef long double double_t;
#else
typedef float float_t;
typedef double double_t;
#endif
#define MATH_ERRNO	1
#define MATH_ERREXCEPT	2
#define math_errhandling	MATH_ERRNO

__BEGIN_DECLS
float	acosf __P((float));
float	acoshf __P((float));
long double	acoshl __P((long double));
long double	acosl __P((long double));
float	asinf __P((float));
float	asinhf __P((float));
long double	asinhl __P((long double));
long double	asinl __P((long double));
float	atan2f __P((float,float));
long double	atan2l __P((long double,long double));
float	atanf __P((float));
float	atanhf __P((float));
long double	atanhl __P((long double));
long double	atanl __P((long double));
float	cbrtf __P((float));
long double	cbrtl __P((long double));
float	ceilf __P((float));
long double	ceill __P((long double));
float	copysignf __P((float,float));
long double	copysignl __P((long double,long double));
float	cosf __P((float));
float	coshf __P((float));
long double	coshl __P((long double));
long double	cosl __P((long double));
float	erfcf __P((float));
long double	erfcl __P((long double));
float	erff __P((float));
long double	erfl __P((long double));
long double	exp2l __P((long double));
float	expf __P((float));
long double	expl __P((long double));
float	expm1f __P((float));
long double	expm1l __P((long double));
float	fabsf __P((float));
long double	fabsl __P((long double));
double	fdim __P((double,double));
float	fdimf __P((float,float));
long double	fdiml __P((long double,long double));
float	floorf __P((float));
long double	floorl __P((long double));
double	fma __P((double,double,double));
float	fmaf __P((float,float,float));
long double	fmal __P((long double,long double,long double));
double	fmax __P((double,double));
float	fmaxf __P((float,float));
long double	fmaxl __P((long double,long double));
double	fmin __P((double,double));
float	fminf __P((float,float));
long double	fminl __P((long double,long double));
float	fmodf __P((float,float));
long double	fmodl __P((long double,long double));
float	frexpf __P((float,int *));
long double	frexpl __P((long double,int *));
float	hypotf __P((float,float));
long double	hypotl __P((long double,long double));
int	ilogb __P((double));
int	ilogbf __P((float));
int	ilogbl __P((long double));
float	ldexpf __P((float,int));
long double	ldexpl __P((long double,int));
float	lgammaf __P((float));
long double	lgammal __P((long double));
long long	llrint __P((double));
long long	llrintf __P((float));
long long	llrintl __P((long double));
long long	llround __P((double));
long long	llroundf __P((float));
long long	llroundl __P((long double));
long double	log10l __P((long double));
float	log1pf __P((float));
long double	log1pl __P((long double));
long double	log2l __P((long double));
float	logbf __P((float));
long double	logbl __P((long double));
float	logf __P((float));
long double	logl __P((long double));
long	lrint __P((double));
long	lrintf __P((float));
long	lrintl __P((long double));
long	lround __P((double));
long	lroundf __P((float));
long	lroundl __P((long double));
float	modff __P((float,float *));
long double	modfl __P((long double,long double *));
double	nan __P((const char *));
float	nanf __P((const char *));
long double	nanl __P((const char *));
double	nearbyint __P((double));
float	nearbyintf __P((float));
long double	nearbyintl __P((long double));
double	nextafter __P((double,double));
float	nextafterf __P((float,float));
long double	nextafterl __P((long double,long double));
double	nexttoward __P((double,long double));
float	nexttowardf __P((float,long double));
long double	nexttowardl __P((long double,long double));
float	powf __P((float,float));
long double	powl __P((long double,long double));
double	remainder __P((double,double));
float	remainderf __P((float,float));
long double	remainderl __P((long double,long double));
double	remquo __P((double,double,int *));
float	remquof __P((float,float,int *));
long double	remquol __P((long double,long double,int *));
float	rintf __P((float));
long double	rintl __P((long double));
long double	roundl __P((long double));
double	scalbln __P((double,long));
float	scalblnf __P((float,long));
long double	scalblnl __P((long double,long));
double	scalbn __P((double,int));
float	scalbnf __P((float,int));
long double	scalbnl __P((long double,int));
float	sinf __P((float));
float	sinhf __P((float));
long double	sinhl __P((long double));
long double	sinl __P((long double));
float	sqrtf __P((float));
long double	sqrtl __P((long double));
float	tanf __P((float));
float	tanhf __P((float));
long double	tanhl __P((long double));
long double	tanl __P((long double));
double	tgamma __P((double));
float	tgammaf __P((float));
long double	tgammal __P((long double));
double	trunc __P((double));
long double	truncl __P((long double));
/* the 68881 header defines these as static inlines of its own */
#if !((defined(__GNUC__) || defined(__cplusplus)) && defined(__HAVE_68881__) && !defined(__NO_MATH_INLINES))
double	exp2 __P((double));
float	exp2f __P((float));
float	log10f __P((float));
double	log2 __P((double));
float	log2f __P((float));
float	roundf __P((float));
float	truncf __P((float));
#else
/* declared in the non-68881 list above, which that branch does not reach */
double	acosh __P((double));
double	asinh __P((double));
double	cbrt __P((double));
double	copysign __P((double, double));
double	erf __P((double));
double	erfc __P((double));
double	lgamma __P((double));
double	round __P((double));
#endif
__END_DECLS

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
/* C99 7.12.3 classification macros (type-generic through the builtins). The
 * isinf/isnan/isnormal functions stay declared above for old code, and for
 * gcc before 4.4 (the library itself is built with 2.95), which lacks
 * __builtin_fpclassify and would turn the builtins into undefined calls. */
#undef fpclassify
#undef isfinite
#undef isinf
#undef isnan
#undef isnormal
#undef signbit
#define fpclassify(x)	__builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)
#define isfinite(x)	__builtin_isfinite(x)
#define isinf(x)	__builtin_isinf(x)
#define isnan(x)	__builtin_isnan(x)
#define isnormal(x)	__builtin_isnormal(x)
#define signbit(x)	__builtin_signbit(x)
#endif
#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

#endif /* _MATH_H_ */
