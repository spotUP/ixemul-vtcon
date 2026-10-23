/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* stpcpy, mempcpy and memrchr for programs built against the 80.x SDK headers.
 * <string.h> declares them (80.x has vectors 636 and up), and gcc then
 * writes calls to them itself, turning strcpy/memcpy chains into one
 * call: GNU screen had no stpcpy in its source and failed to link. The
 * SDK's libc.a keeps 48.2's stubs so a port runs on both libraries;
 * these static copies win at link and run on both. memrchr is declared
 * by the 80.x <string.h> but has no body in either library: dash's
 * configure found the declaration, not the function. (UP-Term) */
#include <string.h>

char *
stpcpy(char *d, const char *s)
{
	while ((*d = *s++) != '\0')
		d++;
	return d;
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
