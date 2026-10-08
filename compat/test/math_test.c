/* Host test of compat/math.c (built with -Datan2=ixc_atan2): its atan2
 * against the host libm in every quadrant, on the axes and at the signed
 * zeros. Exit status = failures. */
#include <math.h>
#include <stdio.h>

double ixc_atan2(double, double);

static int fails;

static void
check(double y, double x)
{
	double want = atan2(y, x), got = ixc_atan2(y, x);
	int ok = fabs(want - got) <= 1e-15 * (1 + fabs(want)) && signbit(want) == signbit(got);

	if (!ok) {
		printf("[FAIL] atan2(%g, %g) = %.17g, want %.17g\n", y, x, got, want);
		fails++;
	}
}

int
main(void)
{
	static const double v[] = { 0.0, -0.0, 1.0, -1.0, 0.5, -2.5, 1e-300, -1e300, 3.0, -7.0 };
	int i, j, n = sizeof v / sizeof v[0];

	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			check(v[i], v[j]);
	if (!isnan(ixc_atan2(NAN, 1.0)) || !isnan(ixc_atan2(1.0, NAN))) {
		printf("[FAIL] atan2 of a NaN is not NaN\n");
		fails++;
	}
	printf("%s atan2: %d of %d cases agree with the host libm\n",
	    fails ? "[FAIL]" : "[OK]  ", n * n + 1 - fails, n * n + 1);
	return (fails);
}
