/* Known values of the libm.a functions: zeros, signs, infinity, NaN, and
 * negative arguments (cbrt). Exit status = failures. Float and long double
 * variants are checked at the same points to float / double precision. */
#include <stdio.h>
#include <string.h>

#define NAN_ (__builtin_nan(""))
#define INF_ (__builtin_inf())

double acosh(double), asinh(double), atanh(double), cbrt(double), copysign(double, double);
double erf(double), erfc(double), exp2(double), expm1(double), fdim(double, double);
double fma(double, double, double), fmax(double, double), fmin(double, double), fmod(double, double);
double hypot(double, double), lgamma(double), log1p(double), log2(double), logb(double);
double nearbyint(double), nextafter(double, double), remainder(double, double), rint(double);
double round(double), scalbn(double, int), tgamma(double), trunc(double), atan2(double, double);
double frexp(double, int *), ldexp(double, int), modf(double, double *), remquo(double, double, int *);
int ilogb(double);
long lrint(double), lround(double);
long long llrint(double), llround(double);
float hypotf(float, float), cbrtf(float), truncf(float), roundf(float), sqrtf(float), expf(float);
float logf(float), powf(float, float), fmodf(float, float), copysignf(float, float), fmaxf(float, float);
float fminf(float, float), expm1f(float), log1pf(float), rintf(float), log2f(float), exp2f(float);
float tgammaf(float), lgammaf(float), acoshf(float), atanhf(float), atan2f(float, float), ldexpf(float, int);
long double hypotl(long double, long double), cbrtl(long double), truncl(long double), sqrtl(long double);
long double fabsl(long double), copysignl(long double, long double), fmaxl(long double, long double);
long double expm1l(long double), log1pl(long double), roundl(long double), rintl(long double);

static int fails, total;

static int
same(double got, double want, double tol)
{
	if (want != want)
		return got != got;
	if (got != got)
		return 0;
	if (want == INF_ || want == -INF_)
		return got == want;
	if (want == 0.0)	/* sign of zero counts */
		return got == 0.0 && (1.0 / got < 0) == (1.0 / want < 0);
	return (got > want ? got - want : want - got) <= tol * (want > 0 ? want : -want);
}

#define T(expr, want, tol) do { double g_ = (double)(expr); total++; \
	if (!same(g_, (want), (tol))) { printf("[FAIL] %s = %.17g, want %.17g\n", #expr, g_, (double)(want)); fails++; } } while (0)

int
main(void)
{
	int e, q;
	double ip;

	T(hypot(3, 4), 5, 1e-15); T(hypot(-3, -4), 5, 1e-15); T(hypot(0, 0), 0, 0);
	T(hypot(INF_, NAN_), INF_, 0); T(hypot(NAN_, 1), NAN_, 0); T(hypotf(5, 12), 13, 1e-6); T(hypotl(8, 15), 17, 1e-14);
	T(cbrt(27), 3, 1e-15); T(cbrt(-27), -3, 1e-15); T(cbrt(0), 0, 0); T(cbrt(-0.0), -0.0, 0);
	T(cbrt(INF_), INF_, 0); T(cbrt(-INF_), -INF_, 0); T(cbrt(NAN_), NAN_, 0); T(cbrtf(-8), -2, 1e-6); T(cbrtl(-125), -5, 1e-14);
	T(trunc(2.7), 2, 0); T(trunc(-2.7), -2, 0); T(trunc(-0.5), -0.0, 0); T(trunc(INF_), INF_, 0); T(truncf(-1.5f), -1, 0); T(truncl(9.9L), 9, 0);
	T(rint(2.5), 2, 0); T(rint(3.5), 4, 0); T(rint(-2.5), -2, 0); T(rint(-0.2), -0.0, 0); T(nearbyint(1.5), 2, 0); T(rintf(0.5f), 0, 0); T(rintl(-1.5L), -2, 0);
	T(round(2.5), 3, 0); T(round(-2.5), -3, 0); T(round(0.49999999999999994), 0, 0); T(round(-0.0), -0.0, 0); T(roundf(-0.4f), -0.0, 0); T(roundl(2.5L), 3, 0);
	T(lround(2.5), 3, 0); T(lround(-2.5), -3, 0); T(llround(-1.5), -2, 0); T(lrint(2.5), 2, 0); T(llrint(-3.5), -4, 0);
	T(copysign(3, -0.0), -3, 0); T(copysign(-3, 0.0), 3, 0); T(copysign(0.0, -1), -0.0, 0); T(copysign(NAN_, -1) != copysign(NAN_, -1), 1, 0);
	T(copysignf(2, -1), -2, 0); T(copysignl(2, -1), -2, 0); T(fabsl(-4.5L), 4.5, 0);
	T(fmax(1, 2), 2, 0); T(fmax(NAN_, 2), 2, 0); T(fmax(1, NAN_), 1, 0); T(fmax(NAN_, NAN_), NAN_, 0); T(fmax(-INF_, -5), -5, 0);
	T(fmin(1, 2), 1, 0); T(fmin(NAN_, 2), 2, 0); T(fmin(-INF_, 3), -INF_, 0); T(fmaxf(-1, -2), -1, 0); T(fminf(-1, NAN_), -1, 0); T(fmaxl(1, 3), 3, 0);
	T(expm1(0), 0, 0); T(expm1(-0.0), -0.0, 0); T(expm1(1e-10), 1.00000000005e-10, 1e-15); T(expm1(-INF_), -1, 0); T(expm1(INF_), INF_, 0); T(expm1f(0), 0, 0); T(expm1l(1e-10L), 1.00000000005e-10, 1e-12);
	T(log1p(0), 0, 0); T(log1p(-0.0), -0.0, 0); T(log1p(-1), -INF_, 0); T(log1p(-2), NAN_, 0); T(log1p(1e-10), 9.9999999995e-11, 1e-15); T(log1pf(0), 0, 0); T(log1pl(1e-10L), 9.9999999995e-11, 1e-12);
	T(log2(8), 3, 1e-15); T(log2(1), 0, 0); T(log2(0), -INF_, 0); T(log2(-1), NAN_, 0); T(log2f(1024), 10, 1e-6);
	T(exp2(10), 1024, 0); T(exp2(-1), 0.5, 0); T(exp2(0), 1, 0); T(exp2(-INF_), 0, 0); T(exp2f(3), 8, 1e-6);
	T(asinh(0), 0, 0); T(asinh(-0.0), -0.0, 0); T(asinh(-1), -0.88137358701954305, 1e-15); T(acosh(1), 0, 0); T(acosh(0.5), NAN_, 0);
	T(acosh(INF_), INF_, 0); T(acoshf(1), 0, 0); T(atanh(0), 0, 0); T(atanh(-0.0), -0.0, 0); T(atanh(1), INF_, 0); T(atanh(-1), -INF_, 0); T(atanh(2), NAN_, 0); T(atanhf(0.5f), 0.54930614433405489, 1e-6);
	T(atan2(1, 1), 0.78539816339744831, 1e-15); T(atan2(-0.0, -1), -3.14159265358979324, 1e-15); T(atan2f(1, -1), 2.35619449019234492, 1e-6);
	T(erf(0), 0, 0); T(erf(-0.0), -0.0, 0); T(erf(1), 0.84270079294971487, 1e-15); T(erf(-INF_), -1, 0); T(erfc(0), 1, 0); T(erfc(INF_), 0, 0); T(erfc(-INF_), 2, 0);
	T(tgamma(5), 24, 1e-13); T(tgamma(0.5), 1.7724538509055161, 1e-13); T(tgamma(0.0), INF_, 0); T(tgamma(-0.0), -INF_, 0); T(tgamma(-1), NAN_, 0); T(tgamma(INF_), INF_, 0); T(tgamma(-INF_), NAN_, 0); T(tgammaf(4), 6, 1e-5);
	T(lgamma(1), 0, 0); T(lgamma(2), 0, 0); T(lgamma(5), 3.1780538303479457, 1e-14); T(lgamma(0), INF_, 0); T(lgamma(INF_), INF_, 0); T(lgammaf(3), 0.69314718055994531, 1e-6);
	T(fdim(5, 3), 2, 0); T(fdim(3, 5), 0, 0); T(fdim(INF_, INF_), 0, 0);
	T(fma(2, 3, 4), 10, 0); T(fma(0, INF_, 1), NAN_, 0); T(fma(-0.0, 1, -0.0), -0.0, 0);
	T(fmod(5.5, 2), 1.5, 0); T(fmod(-5.5, 2), -1.5, 0); T(fmod(5.5, INF_), 5.5, 0); T(fmod(1, 0), NAN_, 0); T(fmod(-0.0, 3), -0.0, 0); T(fmod(INF_, 3), NAN_, 0); T(fmodf(7, 4), 3, 0);
	T(remainder(5, 3), -1, 0); T(remainder(7, 2), -1, 0); T(remainder(1, 0), NAN_, 0); T(remainder(-6, 4), 2, 0);
	T(ilogb(8), 3, 0); T(ilogb(0.25), -2, 0); T(logb(8), 3, 0); T(logb(0), -INF_, 0); T(logb(-INF_), INF_, 0);
	T(scalbn(1, 10), 1024, 0); T(scalbn(-0.0, 5), -0.0, 0); T(ldexp(1.5, 2), 6, 0); T(ldexpf(1, -1), 0.5, 0);
	T(frexp(8, &e), 0.5, 0); T(e, 4, 0); T(modf(-2.25, &ip), -0.25, 0); T(ip, -2, 0); T(remquo(10, 3, &q), 1, 0); T(q, 3, 0);
	T(nextafter(1, 2) > 1, 1, 0); T(nextafter(1, 1), 1, 0); T(nextafter(0, -1) < 0, 1, 0);
	T(sqrtf(9), 3, 0); T(sqrtf(-1), NAN_, 0); T(sqrtf(-0.0f), -0.0, 0); T(sqrtl(2.25L), 1.5, 0);
	T(expf(0), 1, 0); T(expf(1), 2.7182818284590452, 1e-6); T(logf(1), 0, 0); T(logf(0), -INF_, 0); T(logf(-1), NAN_, 0);
	T(powf(2, 10), 1024, 1e-6); T(powf(2, -1), 0.5, 1e-6); T(powf(0, 0), 1, 0);
	printf("%d checks, %d failures\n", total, fails);
	return fails != 0;
}
