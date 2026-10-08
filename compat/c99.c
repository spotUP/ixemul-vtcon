/* C99 and POSIX functions ixemul.library 48.2 has no vectors for, built on
 * what it has (UP-Term: libevent, tmux). One thread per ixemul program, so
 * the _r forms may copy from the library's static results. */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>

long long
strtoll(const char *s, char **end, int base)
{
	return strtoq(s, end, base);
}

unsigned long long
strtoull(const char *s, char **end, int base)
{
	return strtouq(s, end, base);
}

/* off_t is 32 bits here: the long offsets fseek/ftell take */
off_t
ftello(FILE *f)
{
	return ftell(f);
}

int
fseeko(FILE *f, off_t off, int whence)
{
	return fseek(f, (long)off, whence);
}

char *
ctime_r(const time_t *t, char *buf)
{
	char *s = ctime(t);

	if (s == NULL)
		return NULL;
	return strcpy(buf, s);	/* POSIX: buf holds at least 26 bytes */
}

char *
strsignal(int sig)
{
	static char unknown[32];

	if (sig > 0 && sig < NSIG)
		return (char *)sys_siglist[sig];
	sprintf(unknown, "Unknown signal %d", sig);
	return unknown;
}

/* x - n*y with n = x/y rounded toward zero, the sign of x (C99) */
double
fmod(double x, double y)
{
	double q, n;

	if (y == 0.0 || x != x || y != y)
		return (x * y) / (x * y);	/* NaN */
	q = x / y;
	n = q < 0 ? ceil(q) : floor(q);
	return x - n * y;
}

/* half away from zero (C99) */
double
round(double x)
{
	return x < 0 ? -floor(-x + 0.5) : floor(x + 0.5);
}

/* C99 lldiv and llabs (libarchive 3.8.9's archive_time.c): C99 division
 * truncates toward zero, so quot and rem come straight from / and % */
lldiv_t
lldiv(long long n, long long d)
{
	lldiv_t r;

	r.quot = n / d;
	r.rem = n % d;
	return r;
}

long long
llabs(long long x)
{
	return x < 0 ? -x : x;
}

/* C99 strtoimax and strtoumax (<inttypes.h>; libarchive 3.8.9's 7zip and
 * zstd option parsers): intmax_t is long long here (stdint.h) */
long long
strtoimax(const char *s, char **end, int base)
{
	return strtoq(s, end, base);
}

unsigned long long
strtoumax(const char *s, char **end, int base)
{
	return strtouq(s, end, base);
}
