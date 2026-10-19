/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* C99 and POSIX functions ixemul.library 48.2 has no vectors for, built on
 * what it has (UP-Term: libevent, tmux). One thread per ixemul program, so
 * the _r forms may copy from the library's static results.
 *
 * Each function is its own archive member, c99_<name>.o (compat/Makefile
 * compiles this file once per function with -DC99_<name>), as in any libc:
 * a program that defines one of them itself (gnulib's strtoumax in
 * findutils 4.11) and calls another links, where one c99.o brought both
 * definitions and the link failed. */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#ifdef C99_strtoll
long long
strtoll(const char *s, char **end, int base)
{
	return strtoq(s, end, base);
}
#endif

#ifdef C99_strtoull
unsigned long long
strtoull(const char *s, char **end, int base)
{
	return strtouq(s, end, base);
}
#endif

#ifdef C99_ftello
/* off_t is 32 bits here: the long offsets fseek/ftell take */
off_t
ftello(FILE *f)
{
	return ftell(f);
}
#endif

#ifdef C99_fseeko
int
fseeko(FILE *f, off_t off, int whence)
{
	return fseek(f, (long)off, whence);
}
#endif

#ifdef C99_ctime_r
char *
ctime_r(const time_t *t, char *buf)
{
	char *s = ctime(t);

	if (s == NULL)
		return NULL;
	return strcpy(buf, s);	/* POSIX: buf holds at least 26 bytes */
}
#endif

#ifdef C99_strsignal
char *
strsignal(int sig)
{
	static char unknown[32];

	if (sig > 0 && sig < NSIG)
		return (char *)sys_siglist[sig];
	sprintf(unknown, "Unknown signal %d", sig);
	return unknown;
}
#endif

#ifdef C99_fmod
/* x - n*y, n = x/y truncated toward zero, computed exactly (C99): the
 * remainder is exact, so it is a shift-and-subtract over the 53-bit
 * significands (fdlibm's e_fmod), not x - trunc(x/y)*y, which rounds the
 * quotient and is wrong once it needs more than 53 bits (fmod(1e300, 3)) */
double
fmod(double x, double y)
{
	unsigned long long ux, uy, ax, ay, mx, my, sign;
	int ex, ey;

	memcpy(&ux, &x, sizeof ux);
	memcpy(&uy, &y, sizeof uy);
	sign = ux & 0x8000000000000000ULL;
	ax = ux & ~0x8000000000000000ULL;
	ay = uy & ~0x8000000000000000ULL;
	if (ay == 0 || ax >= 0x7ff0000000000000ULL || ay > 0x7ff0000000000000ULL)
		return (x * y) / (x * y);	/* y == 0, x infinite, a NaN: NaN */
	if (ax < ay)
		return x;
	if (ax == ay) {
		ux = sign;
		memcpy(&x, &ux, sizeof x);
		return x;		/* a zero with the sign of x */
	}
	ex = (int)(ax >> 52);
	ey = (int)(ay >> 52);
	mx = ax & 0x000fffffffffffffULL;
	my = ay & 0x000fffffffffffffULL;
	if (ex)
		mx |= 0x0010000000000000ULL;
	else			/* subnormal: normalise, the exponent goes below 1 */
		for (ex = 1; mx < 0x0010000000000000ULL; ex--)
			mx <<= 1;
	if (ey)
		my |= 0x0010000000000000ULL;
	else
		for (ey = 1; my < 0x0010000000000000ULL; ey--)
			my <<= 1;
	for (; ex > ey; ex--) {
		if (mx >= my) {
			mx -= my;
			if (mx == 0) {
				ux = sign;
				memcpy(&x, &ux, sizeof x);
				return x;
			}
		}
		mx <<= 1;
	}
	if (mx >= my)
		mx -= my;
	if (mx == 0) {
		ux = sign;
		memcpy(&x, &ux, sizeof x);
		return x;
	}
	while (mx < 0x0010000000000000ULL) {	/* renormalise */
		mx <<= 1;
		ex--;
	}
	if (ex >= 1)
		ux = sign | ((unsigned long long)ex << 52) | (mx & 0x000fffffffffffffULL);
	else
		ux = sign | (mx >> (1 - ex));	/* subnormal result, exact */
	memcpy(&x, &ux, sizeof x);
	return x;
}
#endif

#ifdef C99_log2
/* base-2 logarithm (C99); libm's log is ixemul's own, no errno detour.
 * log(x) / ln 2 rounds twice: log2(8) came out 2.9999999999999996. Split x
 * into m * 2**e first, m in [sqrt(1/2), sqrt(2)): the exponent is exact and
 * a power of two leaves m = 1, whose log is exactly 0 */
double
log2(double x)
{
	double m;
	int e;

	if (!(x > 0.0) || x == HUGE_VAL)	/* NaN, zero, negative, inf: log decides */
		return log(x) * 1.44269504088896340736;
	m = frexp(x, &e);
	if (m < 0.70710678118654752440) {
		m *= 2.0;
		e--;
	}
	return e + log(m) * 1.44269504088896340736;
}
#endif

#ifdef C99_exp2
/* 2**x (C99) */
double
exp2(double x)
{
	return pow(2.0, x);
}
#endif

#ifdef C99_round
/* half away from zero (C99), on the bits: floor(x + 0.5) rounds up
 * 0.49999999999999994 (x + 0.5 is 1.0 in double), so add half an ulp-of-the-
 * integer-part to the magnitude and clear the fraction instead */
double
round(double x)
{
	unsigned long long u, sign;
	int e;

	memcpy(&u, &x, sizeof u);
	sign = u & 0x8000000000000000ULL;
	u &= ~0x8000000000000000ULL;
	e = (int)(u >> 52);			/* biased exponent */
	if (e >= 1075)
		return x;			/* integral already, infinity, NaN */
	if (e < 1022)
		u = 0;				/* |x| < 0.5 */
	else if (e == 1022)
		u = 0x3ff0000000000000ULL;	/* 0.5 <= |x| < 1 */
	else {
		u += 1ULL << (1074 - e);	/* + 0.5; a carry rolls into the exponent */
		u &= ~((1ULL << (1075 - e)) - 1);
	}
	u |= sign;
	memcpy(&x, &u, sizeof x);
	return x;
}
#endif

#ifdef C99_lldiv
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
#endif

#ifdef C99_llabs
long long
llabs(long long x)
{
	return x < 0 ? -x : x;
}
#endif

#ifdef C99_strtoimax
/* C99 strtoimax and strtoumax (<inttypes.h>; libarchive 3.8.9's 7zip and
 * zstd option parsers): intmax_t is long long here (stdint.h) */
long long
strtoimax(const char *s, char **end, int base)
{
	return strtoq(s, end, base);
}
#endif

#ifdef C99_strtoumax
unsigned long long
strtoumax(const char *s, char **end, int base)
{
	return strtouq(s, end, base);
}
#endif
