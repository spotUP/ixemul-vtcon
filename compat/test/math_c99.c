/* <math.h> must declare every C99 math function libm.a provides
 * (test/libm_names.txt): without a prototype a call is an implicit int
 * function and returns garbage (log2, trunc, lround, fmax, every f and l form
 * were missing). Compile-only; the Makefile's math_c99 rule builds it against
 * the SOURCE header (-I../include ahead of the installed one) with
 * -std=c99 -Werror=implicit-function-declaration, and once with -std=gnu89.
 * Taking an address needs a declaration in scope, so an undeclared name is a
 * compile error whatever the mode. Not run: the values are test/libm_num.c's. */
#include <math.h>

typedef void (*fn_t)(void);

static const fn_t names[] = {
	(fn_t)acos,
	(fn_t)acosf,
	(fn_t)acosh,
	(fn_t)acoshf,
	(fn_t)acoshl,
	(fn_t)acosl,
	(fn_t)asin,
	(fn_t)asinf,
	(fn_t)asinh,
	(fn_t)asinhf,
	(fn_t)asinhl,
	(fn_t)asinl,
	(fn_t)atan,
	(fn_t)atan2,
	(fn_t)atan2f,
	(fn_t)atan2l,
	(fn_t)atanf,
	(fn_t)atanh,
	(fn_t)atanhf,
	(fn_t)atanhl,
	(fn_t)atanl,
	(fn_t)cbrt,
	(fn_t)cbrtf,
	(fn_t)cbrtl,
	(fn_t)ceil,
	(fn_t)ceilf,
	(fn_t)ceill,
	(fn_t)copysign,
	(fn_t)copysignf,
	(fn_t)copysignl,
	(fn_t)cos,
	(fn_t)cosf,
	(fn_t)cosh,
	(fn_t)coshf,
	(fn_t)coshl,
	(fn_t)cosl,
	(fn_t)erf,
	(fn_t)erfc,
	(fn_t)erfcf,
	(fn_t)erfcl,
	(fn_t)erff,
	(fn_t)erfl,
	(fn_t)exp,
	(fn_t)exp2,
	(fn_t)exp2f,
	(fn_t)exp2l,
	(fn_t)expf,
	(fn_t)expl,
	(fn_t)expm1,
	(fn_t)expm1f,
	(fn_t)expm1l,
	(fn_t)fabs,
	(fn_t)fabsf,
	(fn_t)fabsl,
	(fn_t)fdim,
	(fn_t)fdimf,
	(fn_t)fdiml,
	(fn_t)floor,
	(fn_t)floorf,
	(fn_t)floorl,
	(fn_t)fma,
	(fn_t)fmaf,
	(fn_t)fmal,
	(fn_t)fmax,
	(fn_t)fmaxf,
	(fn_t)fmaxl,
	(fn_t)fmin,
	(fn_t)fminf,
	(fn_t)fminl,
	(fn_t)fmod,
	(fn_t)fmodf,
	(fn_t)fmodl,
	(fn_t)frexp,
	(fn_t)frexpf,
	(fn_t)frexpl,
	(fn_t)hypot,
	(fn_t)hypotf,
	(fn_t)hypotl,
	(fn_t)ilogb,
	(fn_t)ilogbf,
	(fn_t)ilogbl,
	(fn_t)ldexp,
	(fn_t)ldexpf,
	(fn_t)ldexpl,
	(fn_t)lgamma,
	(fn_t)lgammaf,
	(fn_t)lgammal,
	(fn_t)llrint,
	(fn_t)llrintf,
	(fn_t)llrintl,
	(fn_t)llround,
	(fn_t)llroundf,
	(fn_t)llroundl,
	(fn_t)log,
	(fn_t)log10,
	(fn_t)log10f,
	(fn_t)log10l,
	(fn_t)log1p,
	(fn_t)log1pf,
	(fn_t)log1pl,
	(fn_t)log2,
	(fn_t)log2f,
	(fn_t)log2l,
	(fn_t)logb,
	(fn_t)logbf,
	(fn_t)logbl,
	(fn_t)logf,
	(fn_t)logl,
	(fn_t)lrint,
	(fn_t)lrintf,
	(fn_t)lrintl,
	(fn_t)lround,
	(fn_t)lroundf,
	(fn_t)lroundl,
	(fn_t)modf,
	(fn_t)modff,
	(fn_t)modfl,
	(fn_t)nan,
	(fn_t)nanf,
	(fn_t)nanl,
	(fn_t)nearbyint,
	(fn_t)nearbyintf,
	(fn_t)nearbyintl,
	(fn_t)nextafter,
	(fn_t)nextafterf,
	(fn_t)nextafterl,
	(fn_t)nexttoward,
	(fn_t)nexttowardf,
	(fn_t)nexttowardl,
	(fn_t)pow,
	(fn_t)powf,
	(fn_t)powl,
	(fn_t)remainder,
	(fn_t)remainderf,
	(fn_t)remainderl,
	(fn_t)remquo,
	(fn_t)remquof,
	(fn_t)remquol,
	(fn_t)rint,
	(fn_t)rintf,
	(fn_t)rintl,
	(fn_t)round,
	(fn_t)roundf,
	(fn_t)roundl,
	(fn_t)scalbln,
	(fn_t)scalblnf,
	(fn_t)scalblnl,
	(fn_t)scalbn,
	(fn_t)scalbnf,
	(fn_t)scalbnl,
	(fn_t)sin,
	(fn_t)sinf,
	(fn_t)sinh,
	(fn_t)sinhf,
	(fn_t)sinhl,
	(fn_t)sinl,
	(fn_t)sqrt,
	(fn_t)sqrtf,
	(fn_t)sqrtl,
	(fn_t)tan,
	(fn_t)tanf,
	(fn_t)tanh,
	(fn_t)tanhf,
	(fn_t)tanhl,
	(fn_t)tanl,
	(fn_t)tgamma,
	(fn_t)tgammaf,
	(fn_t)tgammal,
	(fn_t)trunc,
	(fn_t)truncf,
	(fn_t)truncl,
};

/* types and macros, and calls whose result type matters (an implicit int
 * declaration truncates the double) */
static float_t ft = 1;
static double_t dt = 2;
static const float hvf = HUGE_VALF;
static const long double hvl = HUGE_VALL;

int
main(void)
{
	double a = 8.0, b = -2.5;
	int e = 0;

	if (log2(a) != 3.0 || exp2(3.0) != 8.0 || trunc(b) != -2.0 || fmax(a, b) != a ||
	    fmin(a, b) != b || lround(2.5) != 3L || llround(b) != -3LL || cbrt(a) != 2.0)
		return 1;
	if (log2f(8.0f) != 3.0f || truncl(-2.5L) != -2.0L || fmaxf(1.0f, 2.0f) != 2.0f)
		return 2;
	if (!isinf(INFINITY) || !isnan(NAN) || !isfinite(a) || !isnormal(a) || !signbit(b))
		return 3;
	if (fpclassify(a) != FP_NORMAL || fpclassify(0.0) != FP_ZERO || fpclassify(INFINITY) != FP_INFINITE ||
	    fpclassify(NAN) != FP_NAN)
		return 4;
	if (!isgreater(a, b) || !islessgreater(a, b) || isunordered(a, b))
		return 5;
	(void)frexp(a, &e);
	return (hvf > 1.0f && hvl > 1.0L && ft + dt == 3 && sizeof names > 0) ? 0 : 6;
}
