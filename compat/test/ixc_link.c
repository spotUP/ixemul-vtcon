/* A program calling getopt_long, pselect and posix_openpt must link against
 * libixcompat.a: ixemul has none of them (fzy 1.0: "undefined reference to
 * `getopt_long'" and `pselect'; script and ptyrun need the pty calls).
 * Built by `make test` (bebbo's gcc, the repo's headers); it links or the
 * test fails. */
#include <getopt.h>
#include <sys/select.h>
#include <stdlib.h>
#include <fcntl.h>

int
main(int argc, char **argv)
{
	static const struct option lo[] = { { "x", no_argument, 0, 'x' }, { 0, 0, 0, 0 } };
	fd_set r;
	struct timespec ts = { 0, 0 };
	int m;

	FD_ZERO(&r);
	m = posix_openpt(O_RDWR);
	return getopt_long(argc, argv, "x", lo, 0) + pselect(0, &r, 0, 0, &ts, 0) +
	    (m >= 0 && grantpt(m) == 0 && unlockpt(m) == 0 && ptsname(m) != 0 ? 0 : 1);
}
