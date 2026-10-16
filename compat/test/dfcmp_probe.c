/* Probe for the "Inf, want Inf" rows of libm_num: prints the soft-float
 * double compare helpers on infinities and what == and isinf() give. Built
 * like libm_num (68000 soft-float, -fno-builtin). Not part of `make test`. */
#include <stdio.h>
#include <float.h>
#include <math.h>

int __gtdf2(double, double), __eqdf2(double, double), __ltdf2(double, double);

int
main(void)
{
	volatile double inf = __builtin_inf(), mx = DBL_MAX, nn = __builtin_nan("");
	volatile double x = inf;

	printf("__gtdf2(INF, DBL_MAX) = %d\n", __gtdf2(inf, mx));
	printf("__eqdf2(INF, INF) = %d\n", __eqdf2(inf, inf));
	printf("__ltdf2(-INF, -DBL_MAX) = %d\n", __ltdf2(-inf, -mx));
	printf("__eqdf2(NaN, NaN) = %d\n", __eqdf2(nn, nn));
	printf("x == INFINITY: %d\n", x == INFINITY);
	printf("isinf(x): %d\n", isinf(x));
	return 0;
}
