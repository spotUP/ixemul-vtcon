/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* strtonum, which the SDK's 48.2 stub set declares but has no code for
 * (OpenBSD column: undefined reference to strtonum). The OpenBSD contract
 * (strtonum.c 1.7, Ted Unangst and Todd Miller, ISC): the whole string is a
 * base 10 number in [minval, maxval], else 0 and *errstrp is "invalid",
 * "too small" or "too large" (errno EINVAL / ERANGE / ERANGE); on success
 * *errstrp is NULL and errno is left as it was. The same algorithm as the
 * library's own stdlib/strtonum.c, without its usetup. The host test builds
 * this file with -DIXC_STRTONUM=ixc_strtonum. */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#ifndef IXC_STRTONUM
#define IXC_STRTONUM strtonum
#endif

#ifndef LLONG_MAX
#define LLONG_MAX 0x7fffffffffffffffLL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-0x7fffffffffffffffLL - 1)
#endif

#define INVALID  1
#define TOOSMALL 2
#define TOOLARGE 3

long long
IXC_STRTONUM(const char *numstr, long long minval, long long maxval, const char **errstrp)
{
	long long ll = 0;
	int error = 0;
	char *ep;
	struct errval {
		const char *errstr;
		int err;
	} ev[4] = {
		{ NULL,        0 },
		{ "invalid",   EINVAL },
		{ "too small", ERANGE },
		{ "too large", ERANGE },
	};

	ev[0].err = errno;
	errno = 0;
	if (numstr == NULL || minval > maxval) {
		error = INVALID;
	} else {
		ll = strtoll(numstr, &ep, 10);
		if (numstr == ep || *ep != '\0')
			error = INVALID;
		else if ((ll == LLONG_MIN && errno == ERANGE) || ll < minval)
			error = TOOSMALL;
		else if ((ll == LLONG_MAX && errno == ERANGE) || ll > maxval)
			error = TOOLARGE;
	}
	if (errstrp != NULL)
		*errstrp = ev[error].errstr;
	errno = ev[error].err;
	if (error)
		ll = 0;
	return ll;
}
