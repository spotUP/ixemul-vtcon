/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* mkdtemp (compat/mkdtemp.c): host build uses ixc_mkdtemp, the Amiga build
 * passes -Dixc_mkdtemp=mkdtemp to reach libixcompat.a through <stdlib.h>. */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#ifndef ixc_mkdtemp
char *ixc_mkdtemp(char *);
#endif

static int fails;
#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s (errno %d)\n", __LINE__, #c, errno); fails++; } } while (0)

int
main(void)
{
	char t1[64], t2[64], bad[64], few[64];
	struct stat st;

	sprintf(t1, "mkdtemp_test.%ldXXXXXX", (long)getpid());
	strcpy(t2, t1);
	CHECK(ixc_mkdtemp(t1) == t1);
	CHECK(strlen(t1) == strlen(t2) && strncmp(t1, t2, strlen(t2) - 6) == 0);
	CHECK(strcmp(t1 + strlen(t1) - 6, "XXXXXX") != 0);
	CHECK(stat(t1, &st) == 0 && S_ISDIR(st.st_mode) && (st.st_mode & 0777) == 0700);

	/* the same template again: EEXIST must retry to another name */
	CHECK(ixc_mkdtemp(t2) == t2);
	CHECK(strcmp(t1, t2) != 0);
	CHECK(stat(t2, &st) == 0 && S_ISDIR(st.st_mode));

	strcpy(bad, "mkdtemp_test.noX");
	errno = 0;
	CHECK(ixc_mkdtemp(bad) == NULL && errno == EINVAL);
	strcpy(few, "mkdtemp_test.XXXXX");
	errno = 0;
	CHECK(ixc_mkdtemp(few) == NULL && errno == EINVAL);
	strcpy(bad, "no_such_dir_here/XXXXXX");
	CHECK(ixc_mkdtemp(bad) == NULL && errno == ENOENT);

	rmdir(t1);
	rmdir(t2);
	printf(fails ? "[FAIL] mkdtemp: %d\n" : "[OK] mkdtemp\n", fails);
	return fails != 0;
}
