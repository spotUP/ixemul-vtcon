/* "-lm" must resolve every C99 <math.h> function (double, float, long double;
 * test/libm_names.txt) without newlib's libm.a (whose errno goes through
 * _impure_ptr, which ixemul.library does not export). Link only; the
 * libm_link rule in compat/Makefile checks the symbols with nm, and
 * test/libm_num.c checks the values. Compiled with -fno-builtin: the names are
 * only addresses here. */
extern void acos(void);
extern void acosf(void);
extern void acosh(void);
extern void acoshf(void);
extern void acoshl(void);
extern void acosl(void);
extern void asin(void);
extern void asinf(void);
extern void asinh(void);
extern void asinhf(void);
extern void asinhl(void);
extern void asinl(void);
extern void atan(void);
extern void atan2(void);
extern void atan2f(void);
extern void atan2l(void);
extern void atanf(void);
extern void atanh(void);
extern void atanhf(void);
extern void atanhl(void);
extern void atanl(void);
extern void cbrt(void);
extern void cbrtf(void);
extern void cbrtl(void);
extern void ceil(void);
extern void ceilf(void);
extern void ceill(void);
extern void copysign(void);
extern void copysignf(void);
extern void copysignl(void);
extern void cos(void);
extern void cosf(void);
extern void cosh(void);
extern void coshf(void);
extern void coshl(void);
extern void cosl(void);
extern void erf(void);
extern void erfc(void);
extern void erfcf(void);
extern void erfcl(void);
extern void erff(void);
extern void erfl(void);
extern void exp(void);
extern void exp2(void);
extern void exp2f(void);
extern void exp2l(void);
extern void expf(void);
extern void expl(void);
extern void expm1(void);
extern void expm1f(void);
extern void expm1l(void);
extern void fabs(void);
extern void fabsf(void);
extern void fabsl(void);
extern void fdim(void);
extern void fdimf(void);
extern void fdiml(void);
extern void floor(void);
extern void floorf(void);
extern void floorl(void);
extern void fma(void);
extern void fmaf(void);
extern void fmal(void);
extern void fmax(void);
extern void fmaxf(void);
extern void fmaxl(void);
extern void fmin(void);
extern void fminf(void);
extern void fminl(void);
extern void fmod(void);
extern void fmodf(void);
extern void fmodl(void);
extern void frexp(void);
extern void frexpf(void);
extern void frexpl(void);
extern void hypot(void);
extern void hypotf(void);
extern void hypotl(void);
extern void ilogb(void);
extern void ilogbf(void);
extern void ilogbl(void);
extern void ldexp(void);
extern void ldexpf(void);
extern void ldexpl(void);
extern void lgamma(void);
extern void lgammaf(void);
extern void lgammal(void);
extern void llrint(void);
extern void llrintf(void);
extern void llrintl(void);
extern void llround(void);
extern void llroundf(void);
extern void llroundl(void);
extern void log(void);
extern void log10(void);
extern void log10f(void);
extern void log10l(void);
extern void log1p(void);
extern void log1pf(void);
extern void log1pl(void);
extern void log2(void);
extern void log2f(void);
extern void log2l(void);
extern void logb(void);
extern void logbf(void);
extern void logbl(void);
extern void logf(void);
extern void logl(void);
extern void lrint(void);
extern void lrintf(void);
extern void lrintl(void);
extern void lround(void);
extern void lroundf(void);
extern void lroundl(void);
extern void modf(void);
extern void modff(void);
extern void modfl(void);
extern void nan(void);
extern void nanf(void);
extern void nanl(void);
extern void nearbyint(void);
extern void nearbyintf(void);
extern void nearbyintl(void);
extern void nextafter(void);
extern void nextafterf(void);
extern void nextafterl(void);
extern void nexttoward(void);
extern void nexttowardf(void);
extern void nexttowardl(void);
extern void pow(void);
extern void powf(void);
extern void powl(void);
extern void remainder(void);
extern void remainderf(void);
extern void remainderl(void);
extern void remquo(void);
extern void remquof(void);
extern void remquol(void);
extern void rint(void);
extern void rintf(void);
extern void rintl(void);
extern void round(void);
extern void roundf(void);
extern void roundl(void);
extern void scalbln(void);
extern void scalblnf(void);
extern void scalblnl(void);
extern void scalbn(void);
extern void scalbnf(void);
extern void scalbnl(void);
extern void sin(void);
extern void sinf(void);
extern void sinh(void);
extern void sinhf(void);
extern void sinhl(void);
extern void sinl(void);
extern void sqrt(void);
extern void sqrtf(void);
extern void sqrtl(void);
extern void tan(void);
extern void tanf(void);
extern void tanh(void);
extern void tanhf(void);
extern void tanhl(void);
extern void tanl(void);
extern void tgamma(void);
extern void tgammaf(void);
extern void tgammal(void);
extern void trunc(void);
extern void truncf(void);
extern void truncl(void);

static void (*volatile const table[])(void) = {
	(void (*)(void))acos,
	(void (*)(void))acosf,
	(void (*)(void))acosh,
	(void (*)(void))acoshf,
	(void (*)(void))acoshl,
	(void (*)(void))acosl,
	(void (*)(void))asin,
	(void (*)(void))asinf,
	(void (*)(void))asinh,
	(void (*)(void))asinhf,
	(void (*)(void))asinhl,
	(void (*)(void))asinl,
	(void (*)(void))atan,
	(void (*)(void))atan2,
	(void (*)(void))atan2f,
	(void (*)(void))atan2l,
	(void (*)(void))atanf,
	(void (*)(void))atanh,
	(void (*)(void))atanhf,
	(void (*)(void))atanhl,
	(void (*)(void))atanl,
	(void (*)(void))cbrt,
	(void (*)(void))cbrtf,
	(void (*)(void))cbrtl,
	(void (*)(void))ceil,
	(void (*)(void))ceilf,
	(void (*)(void))ceill,
	(void (*)(void))copysign,
	(void (*)(void))copysignf,
	(void (*)(void))copysignl,
	(void (*)(void))cos,
	(void (*)(void))cosf,
	(void (*)(void))cosh,
	(void (*)(void))coshf,
	(void (*)(void))coshl,
	(void (*)(void))cosl,
	(void (*)(void))erf,
	(void (*)(void))erfc,
	(void (*)(void))erfcf,
	(void (*)(void))erfcl,
	(void (*)(void))erff,
	(void (*)(void))erfl,
	(void (*)(void))exp,
	(void (*)(void))exp2,
	(void (*)(void))exp2f,
	(void (*)(void))exp2l,
	(void (*)(void))expf,
	(void (*)(void))expl,
	(void (*)(void))expm1,
	(void (*)(void))expm1f,
	(void (*)(void))expm1l,
	(void (*)(void))fabs,
	(void (*)(void))fabsf,
	(void (*)(void))fabsl,
	(void (*)(void))fdim,
	(void (*)(void))fdimf,
	(void (*)(void))fdiml,
	(void (*)(void))floor,
	(void (*)(void))floorf,
	(void (*)(void))floorl,
	(void (*)(void))fma,
	(void (*)(void))fmaf,
	(void (*)(void))fmal,
	(void (*)(void))fmax,
	(void (*)(void))fmaxf,
	(void (*)(void))fmaxl,
	(void (*)(void))fmin,
	(void (*)(void))fminf,
	(void (*)(void))fminl,
	(void (*)(void))fmod,
	(void (*)(void))fmodf,
	(void (*)(void))fmodl,
	(void (*)(void))frexp,
	(void (*)(void))frexpf,
	(void (*)(void))frexpl,
	(void (*)(void))hypot,
	(void (*)(void))hypotf,
	(void (*)(void))hypotl,
	(void (*)(void))ilogb,
	(void (*)(void))ilogbf,
	(void (*)(void))ilogbl,
	(void (*)(void))ldexp,
	(void (*)(void))ldexpf,
	(void (*)(void))ldexpl,
	(void (*)(void))lgamma,
	(void (*)(void))lgammaf,
	(void (*)(void))lgammal,
	(void (*)(void))llrint,
	(void (*)(void))llrintf,
	(void (*)(void))llrintl,
	(void (*)(void))llround,
	(void (*)(void))llroundf,
	(void (*)(void))llroundl,
	(void (*)(void))log,
	(void (*)(void))log10,
	(void (*)(void))log10f,
	(void (*)(void))log10l,
	(void (*)(void))log1p,
	(void (*)(void))log1pf,
	(void (*)(void))log1pl,
	(void (*)(void))log2,
	(void (*)(void))log2f,
	(void (*)(void))log2l,
	(void (*)(void))logb,
	(void (*)(void))logbf,
	(void (*)(void))logbl,
	(void (*)(void))logf,
	(void (*)(void))logl,
	(void (*)(void))lrint,
	(void (*)(void))lrintf,
	(void (*)(void))lrintl,
	(void (*)(void))lround,
	(void (*)(void))lroundf,
	(void (*)(void))lroundl,
	(void (*)(void))modf,
	(void (*)(void))modff,
	(void (*)(void))modfl,
	(void (*)(void))nan,
	(void (*)(void))nanf,
	(void (*)(void))nanl,
	(void (*)(void))nearbyint,
	(void (*)(void))nearbyintf,
	(void (*)(void))nearbyintl,
	(void (*)(void))nextafter,
	(void (*)(void))nextafterf,
	(void (*)(void))nextafterl,
	(void (*)(void))nexttoward,
	(void (*)(void))nexttowardf,
	(void (*)(void))nexttowardl,
	(void (*)(void))pow,
	(void (*)(void))powf,
	(void (*)(void))powl,
	(void (*)(void))remainder,
	(void (*)(void))remainderf,
	(void (*)(void))remainderl,
	(void (*)(void))remquo,
	(void (*)(void))remquof,
	(void (*)(void))remquol,
	(void (*)(void))rint,
	(void (*)(void))rintf,
	(void (*)(void))rintl,
	(void (*)(void))round,
	(void (*)(void))roundf,
	(void (*)(void))roundl,
	(void (*)(void))scalbln,
	(void (*)(void))scalblnf,
	(void (*)(void))scalblnl,
	(void (*)(void))scalbn,
	(void (*)(void))scalbnf,
	(void (*)(void))scalbnl,
	(void (*)(void))sin,
	(void (*)(void))sinf,
	(void (*)(void))sinh,
	(void (*)(void))sinhf,
	(void (*)(void))sinhl,
	(void (*)(void))sinl,
	(void (*)(void))sqrt,
	(void (*)(void))sqrtf,
	(void (*)(void))sqrtl,
	(void (*)(void))tan,
	(void (*)(void))tanf,
	(void (*)(void))tanh,
	(void (*)(void))tanhf,
	(void (*)(void))tanhl,
	(void (*)(void))tanl,
	(void (*)(void))tgamma,
	(void (*)(void))tgammaf,
	(void (*)(void))tgammal,
	(void (*)(void))trunc,
	(void (*)(void))truncf,
	(void (*)(void))truncl,
};

int
main(int argc, char **argv)
{
	(void)argv;
	return table[argc % (int)(sizeof table / sizeof table[0])] == 0;
}
