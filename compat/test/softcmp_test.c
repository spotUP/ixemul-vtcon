/* Soft-float compare helpers in ixemul.library (general/arith.c). Before the
 * fix they called IEEEDPCmp/IEEESPCmp, which know nothing of Inf or NaN:
 * __eqdf2(Inf, Inf) returned 1, so x == INFINITY was false and every libm
 * row expecting Inf failed. Built 68000 soft-float so the compares go
 * through the library. Prints "[OK] softcmp" or one line per failure. */
#include <stdio.h>
#include <float.h>

int __eqdf2(double, double), __nedf2(double, double), __gtdf2(double, double),
    __gedf2(double, double), __ltdf2(double, double), __ledf2(double, double);
int __eqsf2(float, float), __nesf2(float, float), __gtsf2(float, float),
    __gesf2(float, float), __ltsf2(float, float), __lesf2(float, float);

static int fails;

static void
check(const char *what, int got, int want)
{
	if (got != want) {
		printf("[ERROR] %s = %d, want %d\n", what, got, want);
		fails++;
	}
}

/* Sign of a libgcc compare result: what the compiler tests it against. */
static int sgn(int r) { return r < 0 ? -1 : r > 0; }

int
main(void)
{
	volatile double inf = __builtin_inf(), mx = DBL_MAX, nn = __builtin_nan("");
	volatile double pz = 0.0, nz = -0.0;
	/* Two negatives that share their high 32 bits: IEEEDPCmp misordered them. */
	volatile double a = -1.0, b = -1.0000000000000002;
	volatile float finf = __builtin_inff(), fmx = FLT_MAX, fnn = __builtin_nanf("");
	volatile double x = inf;

	check("__eqdf2(Inf, Inf)", __eqdf2(inf, inf), 0);
	check("sgn __gtdf2(Inf, DBL_MAX)", sgn(__gtdf2(inf, mx)), 1);
	check("sgn __ltdf2(-Inf, -DBL_MAX)", sgn(__ltdf2(-inf, -mx)), -1);
	check("__eqdf2(+0, -0)", __eqdf2(pz, nz), 0);
	check("sgn __ltdf2(b, a)", sgn(__ltdf2(b, a)), -1);
	check("sgn __gtdf2(a, b)", sgn(__gtdf2(a, b)), 1);
	check("__eqdf2(NaN, NaN) != 0", __eqdf2(nn, nn) != 0, 1);
	check("__nedf2(NaN, NaN) != 0", __nedf2(nn, nn) != 0, 1);
	check("__gtdf2(NaN, 0) > 0", __gtdf2(nn, pz) > 0, 0);
	check("__gedf2(NaN, 0) >= 0", __gedf2(nn, pz) >= 0, 0);
	check("__ltdf2(NaN, 0) < 0", __ltdf2(nn, pz) < 0, 0);
	check("__ledf2(NaN, 0) <= 0", __ledf2(nn, pz) <= 0, 0);
	check("x == INFINITY", x == __builtin_inf(), 1);
	check("NaN == NaN", nn == nn, 0);

	check("__eqsf2(Inf, Inf)", __eqsf2(finf, finf), 0);
	check("sgn __gtsf2(Inf, FLT_MAX)", sgn(__gtsf2(finf, fmx)), 1);
	check("sgn __ltsf2(-Inf, -FLT_MAX)", sgn(__ltsf2(-finf, -fmx)), -1);
	check("__eqsf2(NaN, NaN) != 0", __eqsf2(fnn, fnn) != 0, 1);
	check("__nesf2(NaN, NaN) != 0", __nesf2(fnn, fnn) != 0, 1);
	check("__gtsf2(NaN, 0) > 0", __gtsf2(fnn, 0.0f) > 0, 0);
	check("__gesf2(NaN, 0) >= 0", __gesf2(fnn, 0.0f) >= 0, 0);
	check("__ltsf2(NaN, 0) < 0", __ltsf2(fnn, 0.0f) < 0, 0);
	check("__lesf2(NaN, 0) <= 0", __lesf2(fnn, 0.0f) <= 0, 0);

	if (fails == 0)
		printf("[OK] softcmp\n");
	return fails != 0;
}
