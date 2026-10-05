/*
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * vasprintf.c,v
 *
 * Revision 1.1  2026/09/10  ChatGPT  (JJ)
 *
 *    Adapt the OpenBSD vasprintf() interface to ixemul.library.
 *    Use ixemul's vsnprintf() count-only path instead of OpenBSD __SALC.
 *    Allocate the exact required buffer and guard the INT_MAX + 1 case.
 *    Keep the implementation compatible with GCC 2.95.3 and ANSI C89.
 */

#define _KERNEL
#include "ixemul.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

int
vasprintf(strp, fmt, ap)
	char **strp;
	const char *fmt;
	_BSD_VA_LIST_ ap;
{
	char *buf;
	int len;
	int ret;
	usetup;

	if (strp == NULL) {
		errno = EINVAL;
		return (-1);
	}

	*strp = NULL;

	/* First pass: count the complete formatted result. */
	len = vsnprintf(NULL, 0, fmt, ap);
	if (len < 0)
		return (-1);

	/*
	 * vsnprintf() returns int, so INT_MAX is the only successful
	 * count for which adding the terminating NUL cannot be represented
	 * by ixemul's normal signed-size allocation path.
	 */
	if (len == INT_MAX) {
		errno = ENOMEM;
		return (-1);
	}

	buf = (char *)malloc((size_t)len + 1);
	if (buf == NULL)
		return (-1);

	/* Second pass: format into the exact-size allocation. */
	ret = vsnprintf(buf, (size_t)len + 1, fmt, ap);
	if (ret < 0) {
		free(buf);
		return (-1);
	}

	*strp = buf;
	return (ret);
}
