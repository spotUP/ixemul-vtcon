/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* inet_pton and inet_ntop for AF_INET and AF_INET6: text to bytes and back,
 * "::" compression (longest run, first on a tie, never one group), IPv4-mapped
 * and IPv4-compatible forms, and invalid input giving 0. The same source runs
 * on the host (the Makefile renames the two functions to ixc_*, so the test
 * checks compat's code; built without the rename it checks the host libc
 * against the same vectors) and is linked for the Amiga against the
 * repo's headers and libixcompat.a (link only, no rig). Exit status = failures. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int fails;

#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* text -> bytes, and the canonical text back (canon NULL: no round trip) */
static void
v6(const char *text, const unsigned char *want, const char *canon)
{
	unsigned char b[16];
	char s[INET6_ADDRSTRLEN];

	memset(b, 0xaa, sizeof b);
	if (inet_pton(AF_INET6, text, b) != 1 || memcmp(b, want, 16)) {
		printf("[FAIL] pton6 \"%s\"\n", text);
		fails++;
		return;
	}
	if (canon && (inet_ntop(AF_INET6, b, s, sizeof s) == NULL || strcmp(s, canon))) {
		printf("[FAIL] ntop6 \"%s\" -> \"%s\", want \"%s\"\n", text, inet_ntop(AF_INET6, b, s, sizeof s) ? s : "NULL", canon);
		fails++;
	}
}

static void
bad6(const char *text)
{
	unsigned char b[16];

	memset(b, 0xaa, sizeof b);
	if (inet_pton(AF_INET6, text, b) != 0) {
		printf("[FAIL] pton6 accepted \"%s\"\n", text);
		fails++;
	}
}

static void
bad4(const char *text)
{
	unsigned char b[4] = { 9, 9, 9, 9 };

	if (inet_pton(AF_INET, text, b) != 0 || b[0] != 9) {
		printf("[FAIL] pton4 accepted \"%s\"\n", text);
		fails++;
	}
}

int
main(void)
{
	static const unsigned char zero[16], loop[16] = { [15] = 1 };
	static const unsigned char doc[16] = { 0x20, 0x01, 0x0d, 0xb8, [15] = 1 };
	static const unsigned char full[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0, 0, 0, 0, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34 };
	static const unsigned char mapped[16] = { [10] = 0xff, 0xff, 192, 0, 2, 128 };
	static const unsigned char compat4[16] = { [12] = 1, 2, 3, 4 };
	static const unsigned char tail[16] = { 0xfe, 0x80 };
	static const unsigned char tie[16] = { 0x20, 0x01, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1 };
	static const unsigned char allf[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
	unsigned char b[16];
	char s[64], small[7];

	/* IPv4 */
	memset(b, 0, sizeof b);
	CHECK(inet_pton(AF_INET, "192.0.2.128", b) == 1);
	CHECK(b[0] == 192 && b[1] == 0 && b[2] == 2 && b[3] == 128);
	CHECK(inet_ntop(AF_INET, b, s, sizeof s) && !strcmp(s, "192.0.2.128"));
	CHECK(inet_pton(AF_INET, "0.0.0.0", b) == 1 && !b[0] && !b[3]);
	CHECK(inet_pton(AF_INET, "255.255.255.255", b) == 1 && b[0] == 255 && b[3] == 255);
	CHECK(inet_ntop(AF_INET, b, s, sizeof s) && !strcmp(s, "255.255.255.255"));
	bad4(""); bad4("1.2.3"); bad4("1.2.3.4.5"); bad4("256.1.1.1"); bad4("1.2.3.256");
	bad4("01.2.3.4"); bad4("1..2.3"); bad4(".1.2.3"); bad4("1.2.3."); bad4("1.2.3.4 ");
	bad4("a.b.c.d"); bad4("0x7f.0.0.1"); bad4("-1.2.3.4"); bad4("1.2.3.4/8");

	/* IPv6 */
	v6("::", zero, "::");
	v6("::1", loop, "::1");
	v6("2001:db8::1", doc, "2001:db8::1");
	v6("2001:DB8:0:0:0:0:0:1", doc, "2001:db8::1");
	v6("2001:0db8:85a3:0000:0000:8a2e:0370:7334", full, "2001:db8:85a3::8a2e:370:7334");
	v6("::ffff:192.0.2.128", mapped, "::ffff:192.0.2.128");
	v6("::FFFF:C000:280", mapped, "::ffff:192.0.2.128");
	v6("::1.2.3.4", compat4, "::1.2.3.4");
	v6("fe80::", tail, "fe80::");
	v6("2001::1:0:0:1:1", tie, "2001::1:0:0:1:1");	/* ::1:0:0 run is 2 and 2: the first wins */
	v6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", allf, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
	v6("1:2:3:4:5:6:7:8", (const unsigned char[16]){ 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8 }, "1:2:3:4:5:6:7:8");
	v6("1:2:3:4:5:6:7::", (const unsigned char[16]){ 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7 }, "1:2:3:4:5:6:7:0");
	v6("1:0:3:4:5:6:7:8", (const unsigned char[16]){ 0, 1, 0, 0, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8 }, "1:0:3:4:5:6:7:8");	/* a single zero group is not "::" */
	v6("1::", (const unsigned char[16]){ 0, 1 }, "1::");
	bad6(""); bad6(":"); bad6(":1"); bad6("1:"); bad6("::1::"); bad6(":::"); bad6("1:::2");
	bad6("12345::"); bad6("g::"); bad6("1:2:3:4:5:6:7"); bad6("1:2:3:4:5:6:7:8:9");
	bad6("1:2:3:4:5:6:7::8"); bad6("::1.2.3"); bad6("::1.2.3.4.5"); bad6("::1.2.3.256");
	bad6("1.2.3.4"); bad6("::1.2.3.4:1"); bad6("2001:db8::1 "); bad6("::ffff:01.2.3.4");

	/* buffer sizes and error returns */
	CHECK(inet_ntop(AF_INET, "\1\2\3\4", small, 8) && !strcmp(small, "1.2.3.4"));
	errno = 0;
	CHECK(inet_ntop(AF_INET, "\1\2\3\4", small, 7) == NULL && errno == ENOSPC);
	errno = 0;
	CHECK(inet_ntop(AF_INET6, loop, small, 3) == NULL && errno == ENOSPC);
	CHECK(inet_ntop(AF_INET6, loop, s, 4) && !strcmp(s, "::1"));
	errno = 0;
	CHECK(inet_pton(AF_UNIX, "1.2.3.4", b) == -1 && errno == EAFNOSUPPORT);
	errno = 0;
	CHECK(inet_ntop(AF_UNIX, b, s, sizeof s) == NULL && errno == EAFNOSUPPORT);

	if (!fails)
		printf("[OK] inet_pton, inet_ntop\n");
	return fails;
}
