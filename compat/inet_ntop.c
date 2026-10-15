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
/* inet_ntop (AF_INET and AF_INET6), the ISC/BIND implementation as in ixemul
 * 80.1 static/inet_ntop.c (IPv4 only there, and not built into libc.a),
 * extended with the BIND IPv6 formatter. ixemul.library has no vector for it. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2

static const char *inet_ntop4(const unsigned char *src, char *dst, socklen_t size);
static const char *inet_ntop6(const unsigned char *src, char *dst, socklen_t size);

const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	if (src == NULL || dst == NULL) {
		errno = EFAULT;
		return NULL;
	}
	switch (af) {
	case AF_INET:
		return inet_ntop4((const unsigned char *)src, dst, size);
	case AF_INET6:
		return inet_ntop6((const unsigned char *)src, dst, size);
	default:
		errno = EAFNOSUPPORT;
		return NULL;
	}
}

static const char *
inet_ntop4(const unsigned char *src, char *dst, socklen_t size)
{
	char tmp[sizeof "255.255.255.255"];

	if (sprintf(tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]) >= (int)size) {
		errno = ENOSPC;
		return NULL;
	}
	strcpy(dst, tmp);
	return dst;
}

/* RFC 5952: lower case, the longest run (two groups or more, the first on a
 * tie) of zero groups becomes "::", and ::/96 (not :: and ::1) and
 * ::ffff:0:0/96 print their last 32 bits as a dotted quad */
static const char *
inet_ntop6(const unsigned char *src, char *dst, socklen_t size)
{
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best, cur;
	unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	memset(words, '\0', sizeof words);
	for (i = 0; i < NS_IN6ADDRSZ; i++)
		words[i / 2] |= (unsigned int)src[i] << ((1 - (i % 2)) << 3);
	best.base = -1; best.len = 0;
	cur.base = -1; cur.len = 0;
	for (i = 0; i < NS_IN6ADDRSZ / NS_INT16SZ; i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else if (cur.base != -1) {
			if (best.base == -1 || cur.len > best.len)
				best = cur;
			cur.base = -1;
		}
	}
	if (cur.base != -1 && (best.base == -1 || cur.len > best.len))
		best = cur;
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	tp = tmp;
	for (i = 0; i < NS_IN6ADDRSZ / NS_INT16SZ; i++) {
		if (best.base != -1 && i >= best.base && i < best.base + best.len) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		if (i != 0)
			*tp++ = ':';
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src + 12, tp, (socklen_t)(sizeof tmp - (tp - tmp))))
				return NULL;
			tp += strlen(tp);
			break;
		}
		tp += sprintf(tp, "%x", words[i]);
	}
	if (best.base != -1 && best.base + best.len == NS_IN6ADDRSZ / NS_INT16SZ)
		*tp++ = ':';
	*tp++ = '\0';
	if ((socklen_t)(tp - tmp) > size) {
		errno = ENOSPC;
		return NULL;
	}
	strcpy(dst, tmp);
	return dst;
}
