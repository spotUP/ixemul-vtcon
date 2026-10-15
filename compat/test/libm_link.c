/* "-lm" must resolve sqrt, log2 and pow without newlib's libm.a (whose
 * errno goes through _impure_ptr, which ixemul.library does not export).
 * Link only; see the libm_link rule in compat/Makefile. */
#include <math.h>

int
main(int argc, char **argv)
{
	double x = argc + 1.0;

	(void)argv;
	return (int)(sqrt(x) + log2(x) + pow(x, 2.5));
}
