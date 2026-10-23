/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* stpcpy, stpncpy, mempcpy and memrchr for programs built against the 80.x SDK headers.
 * <string.h> declares them (80.x has vectors 636 and up), and gcc then
 * writes calls to them itself, turning strcpy/memcpy chains into one
 * call: GNU screen had no stpcpy in its source and failed to link. The
 * SDK's libc.a keeps 48.2's stubs so a port runs on both libraries;
 * these static copies win at link and run on both. memrchr is declared
 * by the 80.x <string.h> but has no body in either library: dash's
 * configure found the declaration, not the function. stpncpy: dash's
 * jobs.c calls it, the SDK's libc has no body. memmem and strcasestr: declared by <string.h> too,
 * no body in the ixemul libc (the toolchain's top-level libc.a has newlib's, which ixemul programs
 * do not link): nnn 5.3 calls both. (UP-Term) */
#include <string.h>

char *
stpcpy(char *d, const char *s)
{
	while ((*d = *s++) != '\0')
		d++;
	return d;
}

char *
stpncpy(char *d, const char *s, size_t n)
{
	while (n > 0 && *s != '\0') {
		*d++ = *s++;
		n--;
	}
	{
		char *end = d;

		while (n-- > 0)
			*d++ = '\0';
		return end;
	}
}

void *
mempcpy(void *d, const void *s, size_t n)
{
	return (char *)memcpy(d, s, n) + n;
}

void *
memrchr(const void *s, int c, size_t n)
{
	const unsigned char *p = (const unsigned char *)s + n;

	while (p > (const unsigned char *)s)
		if (*--p == (unsigned char)c)
			return (void *)p;
	return NULL;
}

void *
memmem(const void *hay, size_t hlen, const void *needle, size_t nlen)
{
	const unsigned char *h = hay, *n = needle;
	size_t i;

	if (nlen == 0)
		return (void *)h;
	if (hlen < nlen)
		return NULL;
	for (i = 0; i + nlen <= hlen; i++)
		if (h[i] == n[0] && memcmp(h + i, n, nlen) == 0)
			return (void *)(h + i);
	return NULL;
}

static int
lower_ascii(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

char *
strcasestr(const char *hay, const char *needle)
{
	size_t i;

	if (*needle == '\0')
		return (char *)hay;
	for (; *hay != '\0'; hay++) {
		for (i = 0; needle[i] != '\0' && hay[i] != '\0' &&
		    lower_ascii((unsigned char)hay[i]) == lower_ascii((unsigned char)needle[i]); i++)
			;
		if (needle[i] == '\0')
			return (char *)hay;
		if (hay[i] == '\0')
			return NULL;
	}
	return NULL;
}
