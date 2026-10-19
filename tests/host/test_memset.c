/* Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 *
 * Host check of string/memset.c: every destination alignment 0-7 x lengths
 * 0-300 plus large sizes x values 0, 0x7f, 0x80, 0xff, 0x1ff, against a byte
 * fill, with guard bytes before and after. string/memset.c is compiled as
 * ixemul_memset (the host libc's own memset stays the reference's neighbour).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *ixemul_memset(void *, int, size_t);

#define G 16
#define GUARD 0xA5

static int check(size_t off, size_t n, int c)
{
	size_t total = n + 2 * G + 16, i;
	unsigned char *raw = malloc(total + 16), *buf, *ref;
	void *r;
	int bad = 0;

	ref = malloc(total + 16);
	/* raw is 16-aligned (malloc): buf + G + off has alignment off */
	buf = raw;
	memset(raw, GUARD, total + 16);
	memset(ref, GUARD, total + 16);
	for (i = 0; i < n; i++)
		ref[G + off + i] = (unsigned char)c;
	r = ixemul_memset(buf + G + off, c, n);
	if (r != buf + G + off) { printf("bad return off=%zu n=%zu\n", off, n); bad = 1; }
	if (memcmp(buf, ref, total + 16) != 0) {
		printf("MISMATCH off=%zu n=%zu c=0x%x\n", off, n, c);
		bad = 1;
	}
	free(raw); free(ref);
	return bad;
}

int main(void)
{
	static const int vals[] = { 0, 0x7f, 0x80, 0xff, 0x1ff, 0x12345a5 };
	static const size_t big[] = { 1000, 4095, 4096, 4097, 65536, 100003, 1u << 20 };
	size_t off, n, k;
	int v, fails = 0, runs = 0;

	for (v = 0; v < (int)(sizeof vals / sizeof *vals); v++) {
		for (off = 0; off < 8; off++) {
			for (n = 0; n <= 300; n++) { fails += check(off, n, vals[v]); runs++; }
			for (k = 0; k < sizeof big / sizeof *big; k++) { fails += check(off, big[k], vals[v]); runs++; }
		}
	}
	printf("memset: %d cases, %d failures\n", runs, fails);
	return fails != 0;
}
