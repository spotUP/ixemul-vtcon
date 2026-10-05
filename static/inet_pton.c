/*	$NetBSD: inet_pton.c,v 1.5 2005/06/01 11:48:49 lukem Exp $	*/
/*	from NetBSD: inet_pton.c,v 1.2 2004/05/20 23:12:33 christos Exp	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Revision 1.1  2026/08/01  ChatGPT modification (JJ)
 *
 *  Replaced the generic IPv4/IPv6 parser with a small, strict IPv4-only
 *  implementation suitable for ixemul.library and GCC 2.95.3.
 *
 *  The parser accepts exactly four decimal octets in the range 0..255,
 *  rejects shorthand, hexadecimal, octal, leading-zero forms and trailing
 *  characters, and leaves dst untouched when the address is invalid.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <string.h>

static int inet_pton4 __P((const char *, unsigned char *));

int
inet_pton(af, src, dst)
	int af;
	const char *src;
	void *dst;
{
	unsigned char tmp[4];

	if (af != AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (src == NULL || dst == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (!inet_pton4(src, tmp))
		return 0;

	memcpy(dst, tmp, sizeof(tmp));
	return 1;
}

static int
inet_pton4(src, dst)
	const char *src;
	unsigned char *dst;
{
	const unsigned char *p;
	unsigned int value;
	int octet;

	p = (const unsigned char *)src;

	for (octet = 0; octet < 4; ++octet) {
		if (*p < '0' || *p > '9')
			return 0;

		/*
		 * inet_pton() accepts decimal dotted-quad only.  Reject
		 * multi-digit octets with a leading zero to avoid octal-style
		 * ambiguity and to match strict modern presentation syntax.
		 */
		if (*p == '0' && p[1] >= '0' && p[1] <= '9')
			return 0;

		value = 0;
		do {
			value = value * 10U + (unsigned int)(*p - '0');
			if (value > 255U)
				return 0;
			++p;
		} while (*p >= '0' && *p <= '9');

		dst[octet] = (unsigned char)value;

		if (octet != 3) {
			if (*p != '.')
				return 0;
			++p;
		} else if (*p != '\0') {
			return 0;
		}
	}

	return 1;
}
