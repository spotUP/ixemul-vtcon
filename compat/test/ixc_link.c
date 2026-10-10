/* A program calling getopt_long and pselect must link against
 * libixcompat.a: ixemul has neither (fzy 1.0: "undefined reference to
 * `getopt_long'" and `pselect'). Built by `make test` (bebbo's gcc, the
 * repo's headers); it links or the test fails. */
#include <getopt.h>
#include <sys/select.h>

int
main(int argc, char **argv)
{
	static const struct option lo[] = { { "x", no_argument, 0, 'x' }, { 0, 0, 0, 0 } };
	fd_set r;
	struct timespec ts = { 0, 0 };

	FD_ZERO(&r);
	return getopt_long(argc, argv, "x", lo, 0) + pselect(0, &r, 0, 0, &ts, 0);
}
