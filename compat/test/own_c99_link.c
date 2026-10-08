/* A program that defines one of libixcompat's C99 functions itself and
 * calls another must link: findutils 4.11's gnulib brings its own
 * strtoumax (no <inttypes.h> declares one here) and find failed to link
 * with "multiple definition of strtoumax" once libixcompat gained it, as
 * every C99 function sat in one member, c99.o, and lldiv pulled it in.
 * Built and linked by `make test` (bebbo's gcc); it links or the test fails. */
#include <stdlib.h>

unsigned long long
strtoumax(const char *s, char **end, int base)
{
	(void)s;
	(void)end;
	(void)base;
	return 7;
}

int
main(void)
{
	lldiv_t r = lldiv(7, 2);

	return r.quot + (long long)strtoumax("1", 0, 10) == 10 ? 0 : 1;
}
