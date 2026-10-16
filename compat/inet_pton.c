/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* inet_pton (AF_INET and AF_INET6), the ISC/BIND implementation as in ixemul
 * 80.1 static/inet_pton.c (IPv4 only there, and not built into libc.a),
 * extended with the BIND IPv6 parser. ixemul.library has no vector for it. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define NS_INADDRSZ 4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2

static int inet_pton4(const char *src, unsigned char *dst);
static int inet_pton6(const char *src, unsigned char *dst);

int
inet_pton(int af, const char *src, void *dst)
{
	if (src == NULL || dst == NULL) {
		errno = EFAULT;
		return -1;
	}
	switch (af) {
	case AF_INET:
		return inet_pton4(src, (unsigned char *)dst);
	case AF_INET6:
		return inet_pton6(src, (unsigned char *)dst);
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}

/* dotted quad, exactly four decimal octets 0..255 without a leading zero
 * (but "0"), nothing else; dst is written only on success */
static int
inet_pton4(const char *src, unsigned char *dst)
{
	unsigned char tmp[NS_INADDRSZ], *tp;
	int saw_digit = 0, octets = 0;
	int ch;

	*(tp = tmp) = 0;
	while ((ch = (unsigned char)*src++) != '\0') {
		if (ch >= '0' && ch <= '9') {
			unsigned int n = *tp * 10 + (ch - '0');

			if (saw_digit && *tp == 0)
				return 0;
			if (n > 255)
				return 0;
			*tp = n;
			if (!saw_digit) {
				if (++octets > 4)
					return 0;
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return 0;
			*++tp = 0;
			saw_digit = 0;
		} else
			return 0;
	}
	if (octets < 4)
		return 0;
	memcpy(dst, tmp, NS_INADDRSZ);
	return 1;
}

/* RFC 4291 text forms, including "::" and a trailing dotted quad */
static int
inet_pton6(const char *src, unsigned char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *curtok;
	int ch, seen_xdigits;
	unsigned int val;

	memset((tp = tmp), '\0', NS_IN6ADDRSZ);
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;
	/* leading "::" is fine, a single leading ":" is not */
	if (*src == ':')
		if (*++src != ':')
			return 0;
	curtok = src;
	seen_xdigits = 0;
	val = 0;
	while ((ch = (unsigned char)*src++) != '\0') {
		const char *pch;
		int lc = (ch >= 'A' && ch <= 'F') ? ch + ('a' - 'A') : ch;

		if (lc != '\0' && (pch = strchr(xdigits, lc)) != NULL) {
			if (seen_xdigits >= 4)
				return 0;
			val = (val << 4) | (unsigned int)(pch - xdigits);
			seen_xdigits++;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!seen_xdigits) {
				if (colonp)
					return 0;
				colonp = tp;
				continue;
			} else if (*src == '\0')
				return 0;
			if (tp + NS_INT16SZ > endp)
				return 0;
			*tp++ = (unsigned char)(val >> 8);
			*tp++ = (unsigned char)val;
			seen_xdigits = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && tp + NS_INADDRSZ <= endp &&
		    inet_pton4(curtok, tp) > 0) {
			tp += NS_INADDRSZ;
			seen_xdigits = 0;
			break;	/* '\0' was seen by inet_pton4 */
		}
		return 0;
	}
	if (seen_xdigits) {
		if (tp + NS_INT16SZ > endp)
			return 0;
		*tp++ = (unsigned char)(val >> 8);
		*tp++ = (unsigned char)val;
	}
	if (colonp != NULL) {
		/* "::" stands for the zero groups: shift the tail to the end */
		const int n = (int)(tp - colonp);
		int i;

		if (tp == endp)
			return 0;
		for (i = 1; i <= n; i++) {
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return 0;
	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return 1;
}
