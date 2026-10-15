/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* The rest of the C99 <math.h> set for libm.a (compat/Makefile, LIBM_FUNCS),
 * one archive member per function (libm_<name>.o, -DLIBM_<name>), as c99.c.
 *
 * - The double functions that newlib wraps in errno/matherr code (hypot,
 *   acosh, atanh, remainder, lgamma, tgamma) call newlib's fdlibm kernels
 *   (__ieee754_*, pulled from the toolchain's libm.a, see LIBM_NEWLIB in the
 *   Makefile): no _impure_ptr, no errno; the results are the IEEE ones
 *   (NaN, infinity) the kernels return.
 * - The float functions newlib wraps the same way go through the double
 *   function and round once to float.
 * - The long double functions (96-bit on m68k) go through the double
 *   function: their result has double precision, not 64 mantissa bits.
 *   fabsl and copysignl are exact. */
#include <math.h>

/* the SDK's <math.h> declares only the classic set */
extern double exp2(double), log2(double), tgamma(double), trunc(double);
extern double nearbyint(double), nan(const char *);
extern double fdim(double, double), fmax(double, double), fmin(double, double);
extern double remainder(double, double), fma(double, double, double);
extern double remquo(double, double, int *), scalbn(double, int), scalbln(double, long);
extern float scalbnf(float, int);
extern int ilogb(double);
extern long lrint(double), lround(double);
extern long long llrint(double), llround(double);
extern double nextafter(double, double);
extern float nextafterf(float, float);
extern double __ieee754_hypot(double, double);
extern double __ieee754_acosh(double);
extern double __ieee754_atanh(double);
extern double __ieee754_remainder(double, double);
extern double __ieee754_lgamma_r(double, int *);
extern double __ieee754_gamma_r(double, int *);

#ifdef LIBM_hypot
double
hypot(double x, double y)
{
	return __ieee754_hypot(x, y);
}
#endif

#ifdef LIBM_acosh
double
acosh(double x)
{
	return __ieee754_acosh(x);
}
#endif

#ifdef LIBM_atanh
double
atanh(double x)
{
	return __ieee754_atanh(x);
}
#endif

#ifdef LIBM_remainder
double
remainder(double x, double y)
{
	return __ieee754_remainder(x, y);
}
#endif

#ifdef LIBM_lgamma
/* the sign of gamma(x) is not kept (no signgam) */
double
lgamma(double x)
{
	int sign;

	return __ieee754_lgamma_r(x, &sign);
}
#endif

#ifdef LIBM_tgamma
/* gamma(x) = sign * exp(lgamma(x)); poles at 0 and the negative integers */
double
tgamma(double x)
{
	int sign;
	double l;

	if (x == 0.0)
		return 1.0 / x;
	if (x < 0.0 && (x == floor(x) || x == -__builtin_inf()))
		return (x - x) / (x - x);	/* NaN */
	if (x == __builtin_inf())
		return x;
	l = __ieee754_gamma_r(x, &sign);
	return sign < 0 ? -exp(l) : exp(l);
}
#endif

#ifdef LIBM_nexttoward
double
nexttoward(double x, long double y)
{
	if (x != x || y != y)
		return x + (double)y;
	if (y == (long double)x)
		return (double)y;
	return nextafter(x, y > x ? __builtin_inf() : -__builtin_inf());
}
#endif

#ifdef LIBM_nexttowardf
float
nexttowardf(float x, long double y)
{
	if (x != x || y != y)
		return x + (float)y;
	if (y == (long double)x)
		return (float)y;
	return nextafterf(x, y > x ? __builtin_inf() : -__builtin_inf());
}
#endif

#ifdef LIBM_atan2f
float
atan2f(float y, float x)
{
	return (float)atan2(y, x);
}
#endif

#ifdef LIBM_acosf
float
acosf(float x)
{
	return (float)acos(x);
}
#endif

#ifdef LIBM_asinf
float
asinf(float x)
{
	return (float)asin(x);
}
#endif

#ifdef LIBM_coshf
float
coshf(float x)
{
	return (float)cosh(x);
}
#endif

#ifdef LIBM_expf
float
expf(float x)
{
	return (float)exp(x);
}
#endif

#ifdef LIBM_exp2f
float
exp2f(float x)
{
	return (float)exp2(x);
}
#endif

#ifdef LIBM_logf
float
logf(float x)
{
	return (float)log(x);
}
#endif

#ifdef LIBM_log10f
float
log10f(float x)
{
	return (float)log10(x);
}
#endif

#ifdef LIBM_log2f
float
log2f(float x)
{
	return (float)log2(x);
}
#endif

#ifdef LIBM_sinhf
float
sinhf(float x)
{
	return (float)sinh(x);
}
#endif

#ifdef LIBM_sqrtf
float
sqrtf(float x)
{
	return (float)sqrt(x);
}
#endif

#ifdef LIBM_acoshf
float
acoshf(float x)
{
	return (float)acosh(x);
}
#endif

#ifdef LIBM_atanhf
float
atanhf(float x)
{
	return (float)atanh(x);
}
#endif

#ifdef LIBM_lgammaf
float
lgammaf(float x)
{
	return (float)lgamma(x);
}
#endif

#ifdef LIBM_tgammaf
float
tgammaf(float x)
{
	return (float)tgamma(x);
}
#endif

#ifdef LIBM_powf
float
powf(float x, float y)
{
	return (float)pow(x, y);
}
#endif

#ifdef LIBM_fmodf
float
fmodf(float x, float y)
{
	return (float)fmod(x, y);
}
#endif

#ifdef LIBM_remainderf
float
remainderf(float x, float y)
{
	return (float)remainder(x, y);
}
#endif

#ifdef LIBM_hypotf
float
hypotf(float x, float y)
{
	return (float)hypot(x, y);
}
#endif

#ifdef LIBM_ldexpf
float
ldexpf(float x, int e)
{
	return scalbnf(x, e);
}
#endif

#ifdef LIBM_acosl
long double
acosl(long double x)
{
	return acos(x);
}
#endif

#ifdef LIBM_acoshl
long double
acoshl(long double x)
{
	return acosh(x);
}
#endif

#ifdef LIBM_asinl
long double
asinl(long double x)
{
	return asin(x);
}
#endif

#ifdef LIBM_asinhl
long double
asinhl(long double x)
{
	return asinh(x);
}
#endif

#ifdef LIBM_atanl
long double
atanl(long double x)
{
	return atan(x);
}
#endif

#ifdef LIBM_atanhl
long double
atanhl(long double x)
{
	return atanh(x);
}
#endif

#ifdef LIBM_cbrtl
long double
cbrtl(long double x)
{
	return cbrt(x);
}
#endif

#ifdef LIBM_ceill
long double
ceill(long double x)
{
	return ceil(x);
}
#endif

#ifdef LIBM_cosl
long double
cosl(long double x)
{
	return cos(x);
}
#endif

#ifdef LIBM_coshl
long double
coshl(long double x)
{
	return cosh(x);
}
#endif

#ifdef LIBM_erfl
long double
erfl(long double x)
{
	return erf(x);
}
#endif

#ifdef LIBM_erfcl
long double
erfcl(long double x)
{
	return erfc(x);
}
#endif

#ifdef LIBM_expl
long double
expl(long double x)
{
	return exp(x);
}
#endif

#ifdef LIBM_exp2l
long double
exp2l(long double x)
{
	return exp2(x);
}
#endif

#ifdef LIBM_expm1l
long double
expm1l(long double x)
{
	return expm1(x);
}
#endif

#ifdef LIBM_floorl
long double
floorl(long double x)
{
	return floor(x);
}
#endif

#ifdef LIBM_lgammal
long double
lgammal(long double x)
{
	return lgamma(x);
}
#endif

#ifdef LIBM_logl
long double
logl(long double x)
{
	return log(x);
}
#endif

#ifdef LIBM_log10l
long double
log10l(long double x)
{
	return log10(x);
}
#endif

#ifdef LIBM_log1pl
long double
log1pl(long double x)
{
	return log1p(x);
}
#endif

#ifdef LIBM_log2l
long double
log2l(long double x)
{
	return log2(x);
}
#endif

#ifdef LIBM_logbl
long double
logbl(long double x)
{
	return logb(x);
}
#endif

#ifdef LIBM_nearbyintl
long double
nearbyintl(long double x)
{
	return nearbyint(x);
}
#endif

#ifdef LIBM_rintl
long double
rintl(long double x)
{
	return rint(x);
}
#endif

#ifdef LIBM_roundl
long double
roundl(long double x)
{
	return round(x);
}
#endif

#ifdef LIBM_sinl
long double
sinl(long double x)
{
	return sin(x);
}
#endif

#ifdef LIBM_sinhl
long double
sinhl(long double x)
{
	return sinh(x);
}
#endif

#ifdef LIBM_tanl
long double
tanl(long double x)
{
	return tan(x);
}
#endif

#ifdef LIBM_tanhl
long double
tanhl(long double x)
{
	return tanh(x);
}
#endif

#ifdef LIBM_tgammal
long double
tgammal(long double x)
{
	return tgamma(x);
}
#endif

#ifdef LIBM_truncl
long double
truncl(long double x)
{
	return trunc(x);
}
#endif

#ifdef LIBM_fabsl
long double
fabsl(long double x)
{
	return __builtin_fabsl(x);
}
#endif

#ifdef LIBM_copysignl
long double
copysignl(long double x, long double y)
{
	return __builtin_copysignl(x, y);
}
#endif

#ifdef LIBM_atan2l
long double
atan2l(long double x, long double y)
{
	return atan2(x, y);
}
#endif

#ifdef LIBM_fdiml
long double
fdiml(long double x, long double y)
{
	return fdim(x, y);
}
#endif

#ifdef LIBM_fmaxl
long double
fmaxl(long double x, long double y)
{
	return fmax(x, y);
}
#endif

#ifdef LIBM_fminl
long double
fminl(long double x, long double y)
{
	return fmin(x, y);
}
#endif

#ifdef LIBM_fmodl
long double
fmodl(long double x, long double y)
{
	return fmod(x, y);
}
#endif

#ifdef LIBM_hypotl
long double
hypotl(long double x, long double y)
{
	return hypot(x, y);
}
#endif

#ifdef LIBM_nextafterl
long double
nextafterl(long double x, long double y)
{
	return nextafter(x, y);
}
#endif

#ifdef LIBM_powl
long double
powl(long double x, long double y)
{
	return pow(x, y);
}
#endif

#ifdef LIBM_remainderl
long double
remainderl(long double x, long double y)
{
	return remainder(x, y);
}
#endif

#ifdef LIBM_fmal
long double
fmal(long double x, long double y, long double z)
{
	return fma(x, y, z);
}
#endif

#ifdef LIBM_nexttowardl
long double
nexttowardl(long double x, long double y)
{
	return nextafter(x, y);
}
#endif

#ifdef LIBM_ilogbl
int
ilogbl(long double x)
{
	return ilogb(x);
}
#endif

#ifdef LIBM_lrintl
long
lrintl(long double x)
{
	return lrint(x);
}
#endif

#ifdef LIBM_lroundl
long
lroundl(long double x)
{
	return lround(x);
}
#endif

#ifdef LIBM_llrintl
long long
llrintl(long double x)
{
	return llrint(x);
}
#endif

#ifdef LIBM_llroundl
long long
llroundl(long double x)
{
	return llround(x);
}
#endif

#ifdef LIBM_ldexpl
long double
ldexpl(long double x, int e)
{
	return ldexp(x, e);
}
#endif

#ifdef LIBM_scalbnl
long double
scalbnl(long double x, int e)
{
	return scalbn(x, e);
}
#endif

#ifdef LIBM_scalblnl
long double
scalblnl(long double x, long e)
{
	return scalbln(x, e);
}
#endif

#ifdef LIBM_frexpl
long double
frexpl(long double x, int *e)
{
	return frexp(x, e);
}
#endif

#ifdef LIBM_modfl
long double
modfl(long double x, long double *ip)
{
	double i;
	double f = modf(x, &i);

	*ip = i;
	return f;
}
#endif

#ifdef LIBM_remquol
long double
remquol(long double x, long double y, int *q)
{
	return remquo(x, y, q);
}
#endif

#ifdef LIBM_nanl
long double
nanl(const char *tag)
{
	return nan(tag);
}
#endif
