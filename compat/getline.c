/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* POSIX.1-2008 getdelim and getline, which ixemul has no code for (OpenBSD
 * column: undefined reference to `getline'). Reads through getc, grows *lineptr
 * with realloc (initial size 120), keeps the delimiter, NUL-terminates, returns
 * the length without the NUL, or -1 at end of file with nothing read or on error
 * (errno EINVAL for a null pointer argument, ENOMEM when growing fails; the
 * stream's own error stays set). The host test builds this file with
 * -DIXC_GETDELIM=ixc_getdelim -DIXC_GETLINE=ixc_getline. */
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef IXC_GETDELIM
#define IXC_GETDELIM getdelim
#endif
#ifndef IXC_GETLINE
#define IXC_GETLINE getline
#endif

ssize_t
IXC_GETDELIM(char **lineptr, size_t *n, int delim, FILE *fp)
{
	size_t len = 0;
	int c;

	if (lineptr == NULL || n == NULL || fp == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (*lineptr == NULL || *n == 0) {
		char *p = realloc(*lineptr, 120);

		if (p == NULL) {
			errno = ENOMEM;
			return -1;
		}
		*lineptr = p;
		*n = 120;
	}
	while ((c = getc(fp)) != EOF) {
		if (len + 2 > *n) {
			size_t nn = *n * 2;
			char *p = nn > *n ? realloc(*lineptr, nn) : NULL;

			if (p == NULL) {
				errno = ENOMEM;
				return -1;
			}
			*lineptr = p;
			*n = nn;
		}
		(*lineptr)[len++] = (char)c;
		if (c == delim)
			break;
	}
	if (len == 0)
		return -1;
	(*lineptr)[len] = '\0';
	return (ssize_t)len;
}

ssize_t
IXC_GETLINE(char **lineptr, size_t *n, FILE *fp)
{
	return IXC_GETDELIM(lineptr, n, '\n', fp);
}
