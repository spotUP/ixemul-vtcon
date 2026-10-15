/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* asprintf, vasprintf and strndup: <stdio.h> and <string.h> declare them,
 * ixemul.library has no code for them. Built on vsnprintf / strnlen. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
vasprintf(char **ret, const char *fmt, va_list ap)
{
	va_list ap2;
	int n;

	va_copy(ap2, ap);
	n = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);
	if (n < 0 || (*ret = malloc((size_t)n + 1)) == NULL) {
		*ret = NULL;
		return -1;
	}
	return vsnprintf(*ret, (size_t)n + 1, fmt, ap);
}

int
asprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vasprintf(ret, fmt, ap);
	va_end(ap);
	return n;
}

char *
strndup(const char *s, size_t max)
{
	size_t n = strnlen(s, max);
	char *p = malloc(n + 1);

	if (p != NULL) {
		memcpy(p, s, n);
		p[n] = '\0';
	}
	return p;
}
