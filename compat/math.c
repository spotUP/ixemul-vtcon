/* The libm functions ixemul.library 48.2 has no vectors for and the SDK has
 * no ixemul libm to resolve (README.math: with soft float, <math.h> declares
 * them for an external libm). Built on the ones it has (atan). Measured on
 * onetrue-awk 20260426: undefined reference to atan2 (upterm-ports plan 1.3).
 * The host test builds this file with -Datan2=ixc_atan2. */
#include <math.h>

/* C99 7.12.4.4: the angle of (x, y) in [-pi, pi], by quadrant from atan */
double
atan2(double y, double x)
{
	static const double pi = 3.14159265358979323846;

	if (x != x || y != y)
		return x + y;			/* NaN */
	if (x == 0.0) {
		if (y == 0.0)			/* atan2(+-0, +0) = +-0, (+-0, -0) = +-pi */
			return (1.0 / x < 0) ? (1.0 / y < 0 ? -pi : pi) : y;
		return y > 0 ? pi / 2 : -pi / 2;
	}
	if (x > 0)
		return atan(y / x);
	/* x < 0: the result keeps y's sign, also for y == -0 */
	return (y > 0 || (y == 0.0 && 1.0 / y > 0)) ? atan(y / x) + pi : atan(y / x) - pi;
}
