/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of compat/strtonum.c, reallocarray.c and isnormal.c (built with
 * -DIXC_STRTONUM=ixc_strtonum, -DIXC_REALLOCARRAY=ixc_reallocarray and
 * -DIXC_ISNORMAL=ixc_isnormal): the OpenBSD strtonum(3) contract (strings,
 * errno, the closed range), reallocarray's overflow check, C99 isnormal's
 * classes. Exit status = failures. */
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long long ixc_strtonum(const char *, long long, long long, const char **);
void *ixc_reallocarray(void *, size_t, size_t);
int ixc_isnormal(double);

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void
num(const char *s, long long lo, long long hi, long long want, const char *werr, int werrno)
{
	const char *e = (const char *)1;
	long long v;

	errno = 0;
	v = ixc_strtonum(s, lo, hi, &e);
	CHECK(v == want, "strtonum(\"%s\", %lld, %lld) = %lld, want %lld", s ? s : "NULL", lo, hi, v, want);
	CHECK((werr == NULL) == (e == NULL) && (e == NULL || !strcmp(e, werr)),
	      "strtonum(\"%s\") errstr %s, want %s", s ? s : "NULL", e ? e : "NULL", werr ? werr : "NULL");
	CHECK(errno == werrno, "strtonum(\"%s\") errno %d, want %d", s ? s : "NULL", errno, werrno);
}

int
main(void)
{
	const char *e;
	void *p, *q;
	static const double sub = DBL_MIN / 4;

	/* success: errstr NULL, errno untouched (here 0) */
	num("42", 0, 100, 42, NULL, 0);
	num("-7", -10, 10, -7, NULL, 0);
	num("0", 0, 0, 0, NULL, 0);
	num("100", 0, 100, 100, NULL, 0);	/* the range is closed */
	num("0", 0, 100, 0, NULL, 0);
	num("9223372036854775807", LLONG_MIN, LLONG_MAX, LLONG_MAX, NULL, 0);
	num("-9223372036854775808", LLONG_MIN, LLONG_MAX, LLONG_MIN, NULL, 0);
	/* range */
	num("101", 0, 100, 0, "too large", ERANGE);
	num("-1", 0, 100, 0, "too small", ERANGE);
	num("9223372036854775808", LLONG_MIN, LLONG_MAX, 0, "too large", ERANGE);
	num("-9223372036854775809", LLONG_MIN, LLONG_MAX, 0, "too small", ERANGE);
	/* invalid: trailing text, empty, not a number, bad bounds, NULL */
	num("12x", 0, 100, 0, "invalid", EINVAL);
	num("", 0, 100, 0, "invalid", EINVAL);
	num("abc", 0, 100, 0, "invalid", EINVAL);
	num("1 ", 0, 100, 0, "invalid", EINVAL);
	num("5", 10, 1, 0, "invalid", EINVAL);
	num(NULL, 0, 100, 0, "invalid", EINVAL);
	/* a NULL errstrp is allowed */
	CHECK(ixc_strtonum("5", 0, 9, NULL) == 5, "strtonum with errstrp NULL");
	/* success keeps the caller's errno */
	errno = ENOENT;
	ixc_strtonum("5", 0, 9, &e);
	CHECK(errno == ENOENT, "strtonum success changed errno to %d", errno);

	/* reallocarray */
	p = ixc_reallocarray(NULL, 10, 4);
	CHECK(p != NULL, "reallocarray(NULL, 10, 4) failed");
	memset(p, 7, 40);
	q = ixc_reallocarray(p, 20, 4);
	CHECK(q != NULL && ((unsigned char *)q)[39] == 7, "reallocarray growth lost the contents");
	errno = 0;
	p = q;
	q = ixc_reallocarray(p, SIZE_MAX / 2 + 1, 2);
	CHECK(q == NULL && errno == ENOMEM, "reallocarray overflow: %p errno %d", q, errno);
	errno = 0;
	q = ixc_reallocarray(p, 2, SIZE_MAX / 2 + 1);
	CHECK(q == NULL && errno == ENOMEM, "reallocarray overflow (size big): errno %d", errno);
	errno = 0;
	q = ixc_reallocarray(p, (size_t)1 << (sizeof(size_t) * 4), (size_t)1 << (sizeof(size_t) * 4));
	CHECK(q == NULL && errno == ENOMEM, "reallocarray 2^32 * 2^32 (4 * 4 bytes on 32-bit: 2^16 * 2^16)");
	/* the old block is still valid after the failure */
	CHECK(((unsigned char *)p)[0] == 7, "reallocarray overflow freed or damaged the block");
	/* a large factor that does not overflow is not refused */
	q = ixc_reallocarray(p, 1, 64);
	CHECK(q != NULL, "reallocarray(p, 1, 64) failed");
	free(q);
	q = ixc_reallocarray(NULL, 0, 4);	/* nmemb 0 never overflows */
	free(q);

	/* isnormal */
	CHECK(ixc_isnormal(1.0) && ixc_isnormal(-1.0) && ixc_isnormal(1e300) && ixc_isnormal(-1e-300), "isnormal of normals");
	CHECK(ixc_isnormal(DBL_MIN) && ixc_isnormal(-DBL_MIN) && ixc_isnormal(DBL_MAX), "isnormal at the limits");
	CHECK(!ixc_isnormal(0.0) && !ixc_isnormal(-0.0), "isnormal of zero");
	CHECK(!ixc_isnormal(sub) && !ixc_isnormal(-sub) && !ixc_isnormal(DBL_MIN / 2), "isnormal of subnormals");
	CHECK(!ixc_isnormal(INFINITY) && !ixc_isnormal(-INFINITY), "isnormal of infinity");
	CHECK(!ixc_isnormal(NAN), "isnormal of NaN");

	if (fails)
		return 1;
	puts("[OK] strtonum, reallocarray, isnormal");
	return 0;
}
