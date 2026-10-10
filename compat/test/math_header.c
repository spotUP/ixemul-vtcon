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

int
main(void)
{
	return (inf > 1.0f && huge > 1.0f && score_max > score_min) ? 0 : 1 + (nan_value != nan_value);
}
