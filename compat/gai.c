/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* getaddrinfo, freeaddrinfo, gai_strerror and getnameinfo for libixcompat.a:
 * ixemul.library 80.1's own ixnet/getaddrinfo.c and getnameinfo.c (header
 * comments there; they need ixnet.h, so the library build never held them),
 * built on gethostbyname, getservbyname, gethostbyaddr, getservbyport, plus
 * numeric AF_INET6 through inet_pton / inet_ntop (compat). Names are resolved
 * over IPv4 only: the stack has no IPv6 resolver. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos__
#define GAI_STR_T char *
#else
#define GAI_STR_T const char * /* host <netdb.h> */
#endif
#ifndef AF_UNSPEC
#define AF_UNSPEC PF_UNSPEC
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001UL
#endif

#ifndef EAI_MAX
#define EAI_MAX 14
#endif

#ifndef AI_MASK
#define AI_MASK (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST)
#endif

static const struct in6_addr in6addr_any_ixc = {{{0}}};

static int ascii_isdigit (int);
static int parse_decimal_port (const char *, int *);
static int parse_ipv4_addr (const char *, struct in_addr *);
static int validate_hints (const struct addrinfo *, int *, int *, int *);
static int resolve_service (const char *, int, int, int *);
static int make_addrinfo (const struct in_addr *, int, int, int,
    const char *, struct addrinfo **);
static char *gai_strdup (const char *);
static int host_error_to_eai (int);
static int make_addrinfo6 (const struct in6_addr *, int, int, int,
    const char *, struct addrinfo **);


static int
ascii_isdigit(int c)
{
	return c >= '0' && c <= '9';
}

static char *
gai_strdup(const char *s)
{
	char *p;
	size_t len;

	if (s == NULL)
		return NULL;
	len = strlen(s) + 1;
	p = (char *)malloc(len);
	if (p != NULL)
		memcpy(p, s, len);
	return p;
}

static int
parse_decimal_port(const char *s, int *portp)
{
	const char *p;
	unsigned long value;

	if (s == NULL || *s == '\0' || portp == NULL)
		return 0;

	value = 0;
	for (p = s; *p != '\0'; ++p) {
		if (!ascii_isdigit((unsigned char)*p))
			return 0;
		value = value * 10UL + (unsigned long)(*p - '0');
		if (value > 65535UL)
			return 0;
	}

	*portp = htons((unsigned short)value);
	return 1;
}

static int
parse_ipv4_addr(const char *s, struct in_addr *addr)
{
	unsigned long part[4];
	const char *p;
	int i;
	unsigned long value;

	if (s == NULL || *s == '\0' || addr == NULL)
		return 0;

	p = s;
	for (i = 0; i < 4; ++i) {
		if (!ascii_isdigit((unsigned char)*p))
			return 0;

		value = 0;
		while (ascii_isdigit((unsigned char)*p)) {
			value = value * 10UL + (unsigned long)(*p - '0');
			if (value > 255UL)
				return 0;
			++p;
		}

		part[i] = value;
		if (i != 3) {
			if (*p != '.')
				return 0;
			++p;
		} else {
			if (*p != '\0')
				return 0;
		}
	}

	addr->s_addr = htonl((part[0] << 24) | (part[1] << 16) |
	    (part[2] << 8) | part[3]);
	return 1;
}

static int
validate_hints(const struct addrinfo *hints, int *flagsp, int *socktypep, int *protocolp)
{
	int flags;
	int family;
	int socktype;
	int protocol;
	int mask;

	flags = 0;
	family = AF_UNSPEC;
	socktype = 0;
	protocol = 0;

	if (hints != NULL) {
		flags = hints->ai_flags;
		family = hints->ai_family;
		socktype = hints->ai_socktype;
		protocol = hints->ai_protocol;

		mask = AI_MASK;
#ifdef AI_NUMERICSERV
		mask |= AI_NUMERICSERV;
#endif
		if (flags & ~mask)
			return EAI_BADFLAGS;

		if (hints->ai_addrlen != 0 || hints->ai_addr != NULL ||
		    hints->ai_canonname != NULL || hints->ai_next != NULL)
			return EAI_BADHINTS;

		if (family != AF_UNSPEC && family != AF_INET &&
		    family != AF_INET6)
			return EAI_FAMILY;
	}

	if (socktype == 0) {
		if (protocol == IPPROTO_UDP)
			socktype = SOCK_DGRAM;
		else if (protocol == IPPROTO_TCP)
			socktype = SOCK_STREAM;
		else
			socktype = SOCK_STREAM;
	}

	switch (socktype) {
	case SOCK_STREAM:
		if (protocol == 0)
			protocol = IPPROTO_TCP;
		else if (protocol != IPPROTO_TCP)
			return EAI_BADHINTS;
		break;
	case SOCK_DGRAM:
		if (protocol == 0)
			protocol = IPPROTO_UDP;
		else if (protocol != IPPROTO_UDP)
			return EAI_BADHINTS;
		break;
	case SOCK_RAW:
		break;
	default:
		return EAI_SOCKTYPE;
	}

	*flagsp = flags;
	*socktypep = socktype;
	*protocolp = protocol;
	return 0;
}

static int
resolve_service(const char *servname, int socktype, int flags, int *portp)
{
	struct servent *se;
	const char *proto;

	*portp = 0;
	if (servname == NULL)
		return 0;
	if (parse_decimal_port(servname, portp))
		return 0;

#ifdef AI_NUMERICSERV
	if (flags & AI_NUMERICSERV)
		return EAI_NONAME;
#endif

	if (socktype == SOCK_DGRAM)
		proto = "udp";
	else if (socktype == SOCK_STREAM)
		proto = "tcp";
	else
		return EAI_SERVICE;

	se = getservbyname(servname, proto);
	if (se == NULL)
		return EAI_SERVICE;
	*portp = se->s_port;
	return 0;
}

static int
make_addrinfo(const struct in_addr *addr, int port, int socktype, int protocol, const char *canonname, struct addrinfo **aip)
{
	struct addrinfo *ai;
	struct sockaddr_in *sin;

	ai = (struct addrinfo *)calloc(1, sizeof(*ai) + sizeof(*sin));
	if (ai == NULL)
		return EAI_MEMORY;

	sin = (struct sockaddr_in *)(void *)(ai + 1);
#ifdef BSD4_4
	sin->sin_len = sizeof(*sin);
#endif
	sin->sin_family = AF_INET;
	sin->sin_port = port;
	sin->sin_addr = *addr;

	ai->ai_family = AF_INET;
	ai->ai_socktype = socktype;
	ai->ai_protocol = protocol;
	ai->ai_addrlen = sizeof(*sin);
	ai->ai_addr = (struct sockaddr *)(void *)sin;
	ai->ai_next = NULL;

	if (canonname != NULL) {
		ai->ai_canonname = gai_strdup(canonname);
		if (ai->ai_canonname == NULL) {
			free(ai);
			return EAI_MEMORY;
		}
	}

	*aip = ai;
	return 0;
}

static int
make_addrinfo6(const struct in6_addr *addr, int port, int socktype, int protocol, const char *canonname, struct addrinfo **aip)
{
	struct addrinfo *ai;
	struct sockaddr_in6 *sin6;

	ai = (struct addrinfo *)calloc(1, sizeof(*ai) + sizeof(*sin6));
	if (ai == NULL)
		return EAI_MEMORY;

	sin6 = (struct sockaddr_in6 *)(void *)(ai + 1);
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = port;
	sin6->sin6_addr = *addr;

	ai->ai_family = AF_INET6;
	ai->ai_socktype = socktype;
	ai->ai_protocol = protocol;
	ai->ai_addrlen = sizeof(*sin6);
	ai->ai_addr = (struct sockaddr *)(void *)sin6;
	ai->ai_next = NULL;

	if (canonname != NULL) {
		ai->ai_canonname = gai_strdup(canonname);
		if (ai->ai_canonname == NULL) {
			free(ai);
			return EAI_MEMORY;
		}
	}

	*aip = ai;
	return 0;
}

static int
host_error_to_eai(int err)
{
	switch (err) {
	case TRY_AGAIN:
		return EAI_AGAIN;
	case NO_RECOVERY:
		return EAI_FAIL;
#ifdef NO_DATA
	case NO_DATA:
		return EAI_NODATA;
#endif
	case HOST_NOT_FOUND:
	default:
		return EAI_NONAME;
	}
}

GAI_STR_T
gai_strerror(int ecode)
{
	static const char *ai_errlist[] = {
		"Success",
		"Address family for hostname not supported",
		"Temporary failure in name resolution",
		"Invalid value for ai_flags",
		"Non-recoverable failure in name resolution",
		"ai_family not supported",
		"Memory allocation failure",
		"No address associated with hostname",
		"hostname nor servname provided, or not known",
		"servname not supported for ai_socktype",
		"ai_socktype not supported",
		"System error returned in errno",
		"Invalid value for hints",
		"Resolved protocol is unknown",
		"Argument buffer overflow"
	};

	if (ecode < 0 || ecode > EAI_MAX)
		return (GAI_STR_T)"Unknown error";
	return (GAI_STR_T)ai_errlist[ecode];
}

void
freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	while (ai != NULL) {
		next = ai->ai_next;
		if (ai->ai_canonname != NULL)
			free(ai->ai_canonname);
		free(ai);
		ai = next;
	}
}

int
getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
	struct hostent *he;
	struct addrinfo *head;
	struct addrinfo *tail;
	struct addrinfo *ai;
	struct in_addr addr;
	int flags;
	int socktype;
	int protocol;
	int port;
	int error;
	int result;
	int i;
	int herrno;
	int family;
	struct in6_addr addr6;
	int saved_h_errno;
	const char *canonname;

	/*
	 * getaddrinfo() reports resolver failures through EAI_* return values.
	 * Preserve the legacy resolver state visible through h_errno.
	 */
	saved_h_errno = h_errno;
	head = NULL;
	tail = NULL;
	result = 0;

	if (res == NULL) {
		result = EAI_FAIL;
		goto done;
	}
	*res = NULL;

	if (hostname == NULL && servname == NULL) {
		result = EAI_NONAME;
		goto done;
	}

	family = hints != NULL ? hints->ai_family : AF_UNSPEC;
	error = validate_hints(hints, &flags, &socktype, &protocol);
	if (error != 0) {
		result = error;
		goto done;
	}

	error = resolve_service(servname, socktype, flags, &port);
	if (error != 0) {
		result = error;
		goto done;
	}

	if (hostname == NULL && family == AF_INET6) {
		addr6 = in6addr_any_ixc;
		if (!(flags & AI_PASSIVE))
			addr6.s6_addr[15] = 1;
		error = make_addrinfo6(&addr6, port, socktype, protocol, NULL,
		    &head);
		if (error != 0) {
			result = error;
			goto done;
		}
		*res = head;
		head = NULL;
		goto done;
	}

	if (hostname == NULL) {
		if (flags & AI_PASSIVE)
			addr.s_addr = htonl(INADDR_ANY);
		else
			addr.s_addr = htonl(INADDR_LOOPBACK);

		error = make_addrinfo(&addr, port, socktype, protocol, NULL, &head);
		if (error != 0) {
			result = error;
			goto done;
		}
		*res = head;
		head = NULL;
		goto done;
	}

	if (strchr(hostname, ':') != NULL &&
	    inet_pton(AF_INET6, hostname, &addr6) == 1) {
		if (family == AF_INET) {
			result = EAI_ADDRFAMILY;
			goto done;
		}
		canonname = (flags & AI_CANONNAME) ? hostname : NULL;
		error = make_addrinfo6(&addr6, port, socktype, protocol,
		    canonname, &head);
		if (error != 0) {
			result = error;
			goto done;
		}
		*res = head;
		head = NULL;
		goto done;
	}

	if (parse_ipv4_addr(hostname, &addr)) {
		if (family == AF_INET6) {
			result = EAI_ADDRFAMILY;
			goto done;
		}
		canonname = (flags & AI_CANONNAME) ? hostname : NULL;
		error = make_addrinfo(&addr, port, socktype, protocol,
		    canonname, &head);
		if (error != 0) {
			result = error;
			goto done;
		}
		*res = head;
		head = NULL;
		goto done;
	}

	if ((flags & AI_NUMERICHOST) || family == AF_INET6) {
		result = EAI_NONAME;
		goto done;
	}

	he = gethostbyname(hostname);
	if (he == NULL) {
		herrno = h_errno;
		result = host_error_to_eai(herrno);
		goto done;
	}
	if (he->h_addrtype != AF_INET ||
	    he->h_length < (int)sizeof(struct in_addr) ||
	    he->h_addr_list == NULL) {
		result = EAI_FAMILY;
		goto done;
	}

	for (i = 0; he->h_addr_list[i] != NULL; ++i) {
		memcpy(&addr, he->h_addr_list[i], sizeof(addr));
		canonname = (head == NULL && (flags & AI_CANONNAME)) ?
		    he->h_name : NULL;
		error = make_addrinfo(&addr, port, socktype, protocol,
		    canonname, &ai);
		if (error != 0) {
			result = error;
			goto done;
		}
		if (head == NULL)
			head = ai;
		else
			tail->ai_next = ai;
		tail = ai;
	}

	if (head == NULL) {
		result = EAI_NONAME;
		goto done;
	}

	*res = head;
	head = NULL;

done:
	if (head != NULL)
		freeaddrinfo(head);
	h_errno = saved_h_errno;
	return result;
}

#ifndef NI_MASK
#define NI_MASK (NI_NOFQDN | NI_NUMERICHOST | NI_NAMEREQD | \
                 NI_NUMERICSERV | NI_DGRAM)
#endif

static int copy_result (char *, socklen_t, const char *);
static int getnameinfo6 (const struct sockaddr_in6 *, socklen_t, char *,
    socklen_t, char *, socklen_t, int);
static int format_port (char *, socklen_t, unsigned short);
static int format_numeric_host (char *, socklen_t,
    const struct in_addr *);

static int
copy_result(char *dst, socklen_t dstlen, const char *src)
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
format_port(char *dst, socklen_t dstlen, unsigned short port)
{
	char buf[16];

	snprintf(buf, sizeof(buf), "%u", (unsigned int)port);
	return copy_result(dst, dstlen, buf);
}

static int
format_numeric_host(char *dst, socklen_t dstlen, const struct in_addr *addr)
{
	if (dst == NULL)
		return 0;
	if (dstlen <= 0)
		return EAI_OVERFLOW;

	if (inet_ntop(AF_INET, addr, dst, dstlen) == NULL)
		return EAI_OVERFLOW;

	return 0;
}

/* AF_INET6: numeric host only (no IPv6 reverse lookup here); a name is
 * required (NI_NAMEREQD) is therefore never satisfiable */
static int
getnameinfo6(const struct sockaddr_in6 *sin6, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
	char buf[INET6_ADDRSTRLEN];
	struct servent *se;
	int error;

	if (salen < (socklen_t)sizeof(struct sockaddr_in6))
		return EAI_FAIL;
	if (host != NULL && (flags & NI_NAMEREQD))
		return EAI_NONAME;
	if (host != NULL) {
		if (hostlen <= 0)
			return EAI_OVERFLOW;
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf)) == NULL)
			return EAI_FAIL;
		error = copy_result(host, hostlen, buf);
		if (error != 0)
			return error;
	}
	if (serv != NULL) {
		if (servlen <= 0)
			return EAI_OVERFLOW;
		se = (flags & NI_NUMERICSERV) ? NULL :
		    getservbyport(sin6->sin6_port, (flags & NI_DGRAM) ? "udp" : "tcp");
		if (se != NULL && se->s_name != NULL)
			return copy_result(serv, servlen, se->s_name);
		return format_port(serv, servlen, ntohs(sin6->sin6_port));
	}
	return 0;
}

int
getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
	const struct sockaddr_in *sin;
	struct hostent *he;
	struct servent *se;
	int error;

	if (flags & ~NI_MASK)
		return EAI_BADFLAGS;
	if (sa == NULL)
		return EAI_FAIL;
	if (host == NULL && serv == NULL)
		return EAI_NONAME;
	if (salen < (socklen_t)sizeof(struct sockaddr_in))
		return EAI_FAIL;
	if (sa->sa_family == AF_INET6)
		return getnameinfo6((const struct sockaddr_in6 *)(const void *)sa,
		    salen, host, hostlen, serv, servlen, flags);
	if (sa->sa_family != AF_INET)
		return EAI_FAMILY;

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
