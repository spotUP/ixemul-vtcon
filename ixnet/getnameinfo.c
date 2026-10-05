/*
 *  This file is part of ixemul.library for the Amiga.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Revision 1.3  2026/08/02  ChatGPT modification (JJ)
 *
 *  Avoid errno in format_numeric_host(), since ixemul's errno macro
 *  requires a local u_ptr.  With fixed AF_INET and validated arguments,
 *  inet_ntop() failure here means that the output buffer is too small.
 *
 * Revision 1.1  2026/08/01  ChatGPT modification (JJ)
 * 
 *  Use inet_ntop() for numeric IPv4 output instead of inet_ntoa()'s
 *  static result buffer.  Treat non-NULL zero-length output buffers
 *  as EAI_OVERFLOW.
 *
 *  Return EAI_OVERFLOW when a result buffer is too small.
 *  Return EAI_NONAME when both host and service outputs are NULL.
 */

/*
 * IPv4-only getnameinfo for ixemul.
 *
 * This implementation deliberately avoids NetBSD/KAME resolver
 * dependencies.  Numeric IPv4 output uses inet_ntop(), and reverse lookup
 * uses ixemul's existing gethostbyaddr().
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AF_UNSPEC
#define AF_UNSPEC PF_UNSPEC
#endif

#ifndef NI_MASK
#define NI_MASK (NI_NOFQDN | NI_NUMERICHOST | NI_NAMEREQD | \
                 NI_NUMERICSERV | NI_DGRAM)
#endif

static int copy_result __P((char *, socklen_t, const char *));
static int format_port __P((char *, socklen_t, unsigned short));
static int format_numeric_host __P((char *, socklen_t,
    const struct in_addr *));

static int
copy_result(dst, dstlen, src)
	char *dst;
	socklen_t dstlen;
	const char *src;
{
	size_t len;

	if (dst == NULL)
		return 0;
	if (dstlen <= 0)
		return EAI_OVERFLOW;
	if (src == NULL)
		src = "";

	len = strlen(src);
	if (len + 1 > (size_t)dstlen) {
		dst[0] = '\0';
		return EAI_OVERFLOW;
	}
	memcpy(dst, src, len + 1);
	return 0;
}

static int
format_port(dst, dstlen, port)
	char *dst;
	socklen_t dstlen;
	unsigned short port;
{
	char buf[16];

	sprintf(buf, "%u", (unsigned int)port);
	return copy_result(dst, dstlen, buf);
}

static int
format_numeric_host(dst, dstlen, addr)
	char *dst;
	socklen_t dstlen;
	const struct in_addr *addr;
{
	if (dst == NULL)
		return 0;
	if (dstlen <= 0)
		return EAI_OVERFLOW;

	if (inet_ntop(AF_INET, addr, dst, dstlen) == NULL)
		return EAI_OVERFLOW;

	return 0;
}

int
getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)
	const struct sockaddr *sa;
	socklen_t salen;
	char *host;
	socklen_t hostlen;
	char *serv;
	socklen_t servlen;
	int flags;
{
	const struct sockaddr_in *sin;
	struct hostent *he;
	struct servent *se;
	int error;

	if (flags & ~NI_MASK)
		return EAI_BADFLAGS;
	if (sa == NULL)
		return EAI_FAIL;
	if (salen < (socklen_t)sizeof(struct sockaddr_in))
		return EAI_FAIL;
	if (sa->sa_family != AF_INET)
		return EAI_FAMILY;
	if (host == NULL && serv == NULL)
		return EAI_NONAME;

	sin = (const struct sockaddr_in *)(const void *)sa;

	if (host != NULL && hostlen <= 0)
		return EAI_OVERFLOW;
	if (serv != NULL && servlen <= 0)
		return EAI_OVERFLOW;

	if (host != NULL) {
		if (flags & NI_NUMERICHOST) {
			error = format_numeric_host(host, hostlen,
			    &sin->sin_addr);
			if (error != 0)
				return error;
		} else {
			he = gethostbyaddr((const char *)&sin->sin_addr,
			    sizeof(sin->sin_addr), AF_INET);
			if (he != NULL && he->h_name != NULL) {
				error = copy_result(host, hostlen, he->h_name);
				if (error != 0)
					return error;
			} else if (flags & NI_NAMEREQD) {
				return EAI_NONAME;
			} else {
				error = format_numeric_host(host, hostlen,
				    &sin->sin_addr);
				if (error != 0)
					return error;
			}
		}
	}

	if (serv != NULL) {
		if (flags & NI_NUMERICSERV) {
			error = format_port(serv, servlen, ntohs(sin->sin_port));
			if (error != 0)
				return error;
		} else {
			se = getservbyport(sin->sin_port,
			    (flags & NI_DGRAM) ? "udp" : "tcp");
			if (se != NULL && se->s_name != NULL) {
				error = copy_result(serv, servlen, se->s_name);
				if (error != 0)
					return error;
			} else {
				error = format_port(serv, servlen, ntohs(sin->sin_port));
				if (error != 0)
					return error;
			}
		}
	}

	return 0;
}
