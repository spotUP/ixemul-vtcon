/* <inttypes.h> has the C99 format macros, right for this ABI: file 5.48
 * failed on "'PRIx64' undeclared". -Werror=format makes gcc check every
 * macro against the type it names. Compile-only (bebbo's gcc, the repo's
 * headers); `make test` builds it. */
#include <inttypes.h>
#include <stdio.h>

int
main(void)
{
	int8_t a = 1;
	int16_t b = 2;
	int32_t c = 3;
	int64_t d = 4;
	uint64_t e = 5;
	intmax_t f = 6;
	intptr_t g = 7;
	uint32_t h = 8;
	char buf[128];

	sprintf(buf, "%" PRId8 " %" PRId16 " %" PRId32 " %" PRId64 " %" PRIx64 " %" PRIuMAX " %" PRIdPTR " %" PRIX32,
	    a, b, c, d, e, (uintmax_t)f, g, h);
	return (int)strtoimax(buf, 0, 10) + (int)strtoumax(buf, 0, 10) - 2;
}
