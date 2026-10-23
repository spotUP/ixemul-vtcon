/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* asprintf, vasprintf, strndup, strlcpy, strlcat, getaddrinfo, freeaddrinfo,
 * getnameinfo and gai_strerror. The Makefile builds the compat sources with
 * the functions renamed to ixc_* (this file gets the same -D flags), so the
 * checks run on compat's code, not the host libc. Exit status = failures. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int
va_front(char **out, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vasprintf(out, fmt, ap);
	va_end(ap);
	return n;
}

int
main(void)
{
	char *p, buf[16], host[64], serv[16];
	char big[600];
	struct addrinfo hints, *ai;
	struct sockaddr_in sin;
	struct sockaddr_in6 s6;
	int r;

	/* asprintf / vasprintf: format and length */
	p = NULL;
	r = asprintf(&p, "%s-%d-%c", "ab", 42, 'x');
	CHECK(r == 7 && p && strcmp(p, "ab-42-x") == 0);
	free(p);
	memset(big, 'q', 599); big[599] = 0;
	r = asprintf(&p, "%s!", big);
	CHECK(r == 600 && p && strlen(p) == 600 && p[599] == '!');
	free(p);
	r = va_front(&p, "%05d|%-3s|", 7, "z");
	CHECK(r == 10 && p && strcmp(p, "00007|z  |") == 0);
	free(p);
	r = asprintf(&p, "%s", "");
	CHECK(r == 0 && p && p[0] == 0);
	free(p);

	/* strndup */
	p = strndup("hello", 3);
	CHECK(p && strcmp(p, "hel") == 0);
	free(p);
	p = strndup("hi", 10);
	CHECK(p && strcmp(p, "hi") == 0);
	free(p);
	p = strndup("abc", 0);
	CHECK(p && p[0] == 0);
	free(p);

	/* strlcpy: returns strlen(src), truncates, always terminates */
	memset(buf, 'X', sizeof buf);
	CHECK(strlcpy(buf, "abcdef", 4) == 6 && strcmp(buf, "abc") == 0);
	CHECK(strlcpy(buf, "ab", 16) == 2 && strcmp(buf, "ab") == 0);
	memset(buf, 'X', sizeof buf);
	CHECK(strlcpy(buf, "abc", 0) == 3 && buf[0] == 'X');

	/* strlcat: returns the length it tried to make */
	strcpy(buf, "ab");
	CHECK(strlcat(buf, "cd", 16) == 4 && strcmp(buf, "abcd") == 0);
	strcpy(buf, "abcd");
	CHECK(strlcat(buf, "efgh", 6) == 8 && strcmp(buf, "abcde") == 0);
	memset(buf, 'Y', 4);
	CHECK(strlcat(buf, "zz", 4) == 6);	/* dst unterminated within size */
	strcpy(buf, "abc");
	CHECK(strlcat(buf, "def", 0) == 3);

	/* getaddrinfo: numeric, no resolver */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;
	ai = NULL;
	r = getaddrinfo("127.0.0.1", "8080", &hints, &ai);
	CHECK(r == 0 && ai && ai->ai_family == AF_INET && ai->ai_socktype == SOCK_STREAM
	    && ai->ai_protocol == IPPROTO_TCP && ai->ai_addrlen == sizeof(struct sockaddr_in)
	    && ai->ai_next == NULL);
	if (ai && ai->ai_addr) {
		struct sockaddr_in *a = (struct sockaddr_in *)ai->ai_addr;
		CHECK(a->sin_family == AF_INET && ntohs(a->sin_port) == 8080
		    && ntohl(a->sin_addr.s_addr) == 0x7f000001UL);
	}
	freeaddrinfo(ai);

	ai = NULL;
	r = getaddrinfo("::1", "443", &hints, &ai);
	CHECK(r == 0 && ai && ai->ai_family == AF_INET6 && ai->ai_addrlen == sizeof(struct sockaddr_in6));
	if (ai && ai->ai_addr) {
		struct sockaddr_in6 *a = (struct sockaddr_in6 *)ai->ai_addr;
		unsigned char want[16] = {0};
		want[15] = 1;
		CHECK(a->sin6_family == AF_INET6 && ntohs(a->sin6_port) == 443
		    && memcmp(&a->sin6_addr, want, 16) == 0);
	}
	freeaddrinfo(ai);

	/* family filter, bad numeric, passive, flags */
	hints.ai_family = AF_INET;
	CHECK(getaddrinfo("::1", "1", &hints, &ai) == EAI_ADDRFAMILY);
	hints.ai_family = AF_INET6;
	CHECK(getaddrinfo("127.0.0.1", "1", &hints, &ai) == EAI_ADDRFAMILY);
	hints.ai_family = AF_UNSPEC;
	CHECK(getaddrinfo("no.such.host.invalid", "1", &hints, &ai) == EAI_NONAME);
	CHECK(getaddrinfo("1.2.3.4", "notaservice", &hints, &ai) != 0);
	hints.ai_flags = AI_PASSIVE;
	r = getaddrinfo(NULL, "9", &hints, &ai);
	CHECK(r == 0 && ai && ((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr == 0);
	freeaddrinfo(ai);
	hints.ai_flags = 0x7000000;
	CHECK(getaddrinfo("127.0.0.1", "1", &hints, &ai) == EAI_BADFLAGS);
	CHECK(getaddrinfo(NULL, NULL, NULL, &ai) == EAI_NONAME);

	/* gai_strerror */
	CHECK(gai_strerror(0) != NULL && gai_strerror(EAI_NONAME) != NULL);
	CHECK(strlen(gai_strerror(EAI_NONAME)) > 0 && strlen(gai_strerror(9999)) > 0);

	/* getnameinfo, NI_NUMERICHOST round trip with getaddrinfo's output */
	hints.ai_flags = AI_NUMERICHOST;
	ai = NULL;
	CHECK(getaddrinfo("10.20.30.40", "8080", &hints, &ai) == 0 && ai);
	if (ai) {
		r = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof host,
		    serv, sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
		CHECK(r == 0 && strcmp(host, "10.20.30.40") == 0 && strcmp(serv, "8080") == 0);
		r = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, 5, serv, sizeof serv,
		    NI_NUMERICHOST | NI_NUMERICSERV);
		CHECK(r == EAI_OVERFLOW);
		freeaddrinfo(ai);
	}
	ai = NULL;
	CHECK(getaddrinfo("2001:db8::1", "22", &hints, &ai) == 0 && ai);
	if (ai) {
		r = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof host,
		    serv, sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
		CHECK(r == 0 && strcmp(host, "2001:db8::1") == 0 && strcmp(serv, "22") == 0);
		freeaddrinfo(ai);
	}
	memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(65000);
	sin.sin_addr.s_addr = htonl(0xc0a80101UL);
	r = getnameinfo((struct sockaddr *)&sin, sizeof sin, host, sizeof host, serv,
	    sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
	CHECK(r == 0 && strcmp(host, "192.168.1.1") == 0 && strcmp(serv, "65000") == 0);
	memset(&s6, 0, sizeof s6);
	s6.sin6_family = AF_INET6;
	s6.sin6_len = sizeof s6;
	s6.sin6_port = htons(1);
	CHECK(getnameinfo((struct sockaddr *)&s6, sizeof s6, host, sizeof host, NULL, 0,
	    NI_NUMERICHOST) == 0 && strcmp(host, "::") == 0);
	CHECK(getnameinfo((struct sockaddr *)&s6, sizeof s6, NULL, 0, NULL, 0, 0) == EAI_NONAME);

#ifdef __amigaos__
	/* memrchr: declared by <string.h>, linked from libixcompat's strbuiltin.o (link test only) */
	CHECK(memrchr("abcabc", 'b', 6) != NULL && memrchr("abc", 'z', 3) == NULL);
#endif

	printf("libc_gaps_test: %s\n", fails ? "FAIL" : "ok");
	return fails != 0;
}
