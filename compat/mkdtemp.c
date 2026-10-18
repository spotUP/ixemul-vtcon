/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* mkdtemp. The tree's stdio/mktemp.c has one (via _gettemp), but the
 * libc.a installed in the SDK predates it, so programs get a link error.
 * Same name generation as _gettemp: the trailing X run becomes the pid's
 * digits, and on EEXIST the run is stepped (digit -> a, then b..z, carrying).
 * mode 0700; EINVAL unless the template ends in at least six X.
 * The host test builds this with -DIXC_MKDTEMP=ixc_mkdtemp. */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#ifndef IXC_MKDTEMP
#define IXC_MKDTEMP mkdtemp
#endif

char *
IXC_MKDTEMP(char *path)
{
	char *start, *end, *trv;
	unsigned long pid;

	end = path + strlen(path);
	for (start = end; start > path && start[-1] == 'X'; --start)
		;
	if (end - start < 6) {
		errno = EINVAL;
		return NULL;
	}
	pid = (unsigned long)getpid();
	for (trv = end; trv > start;) {
		--trv;
		*trv = (pid % 10) + '0';
		pid /= 10;
	}
	for (;;) {
		if (mkdir(path, 0700) == 0)
			return path;
		if (errno != EEXIST)
			return NULL;
		for (trv = start;;) {
			if (!*trv)
				return NULL;	/* names exhausted, errno EEXIST */
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit((unsigned char)*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
}
