/* <math.h> has the C99 constants INFINITY, NAN and HUGE_VALF: fzy 1.0's
 * match.h uses INFINITY for SCORE_MAX and failed to compile on its absence
 * ("'INFINITY' undeclared"). Compile-only (bebbo's gcc, the repo's
 * headers); `make test` builds it. */
#include <math.h>

static const float inf = INFINITY;
static const float huge = HUGE_VALF;
static const double score_max = INFINITY;
static const double score_min = -INFINITY;
static const float nan_value = NAN;

/* file 5.48's softmagic.c compares floats with these; they were undefined
 * references to functions of those names at link time */
static double a = 1.0, b = 2.0;

int
main(void)
{
	if (!(isless(a, b) && isgreater(b, a) && islessequal(a, a) && isgreaterequal(b, b) &&
	    islessgreater(a, b) && !isunordered(a, b) && isunordered(a, nan_value)))
		return 3;
	return (inf > 1.0f && huge > 1.0f && score_max > score_min) ? 0 : 1 + (nan_value != nan_value);
}
