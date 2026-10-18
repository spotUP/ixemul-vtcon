/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* lockf on flock (compat/lockf.c). The host build compiles lockf.c with
 * -DIXC_LOCKF=ixc_lockf and calls it by that name; the Amiga build passes
 * -Dixc_lockf=lockf to reach libixcompat.a through the repo's <unistd.h>.
 * Two descriptors of one file stand in for two lockers. opkg's use is
 * F_TLOCK then F_ULOCK with len 0. Exit status = failures. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef F_TLOCK
#error unistd.h lacks F_TLOCK
#endif
#ifdef ixc_lockf
#else
int ixc_lockf(int, int, off_t);
#endif

static int fails;

#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s (errno %d)\n", __LINE__, #c, errno); fails++; } } while (0)

int
main(void)
{
	char name[64];
	int a, b;

	sprintf(name, "lockf_test.%ld.tmp", (long)getpid());
	a = open(name, O_RDWR | O_CREAT, 0600);
	b = open(name, O_RDWR);
	CHECK(a >= 0 && b >= 0);

	CHECK(ixc_lockf(a, F_TLOCK, 0) == 0);
	errno = 0;
	CHECK(ixc_lockf(b, F_TLOCK, 0) == -1 && errno == EACCES);
	errno = 0;
	CHECK(ixc_lockf(b, F_TEST, 0) == -1 && errno == EACCES);
	CHECK(ixc_lockf(a, F_ULOCK, 0) == 0);
	CHECK(ixc_lockf(b, F_TEST, 0) == 0);	/* F_TEST leaves nothing held */
	CHECK(ixc_lockf(a, F_TLOCK, 0) == 0);
	CHECK(ixc_lockf(a, F_ULOCK, 0) == 0);
	CHECK(ixc_lockf(b, F_LOCK, 0) == 0);
	errno = 0;
	CHECK(ixc_lockf(a, F_TLOCK, 4096) == -1 && errno == EACCES);	/* range: whole file */
	CHECK(ixc_lockf(b, F_ULOCK, 0) == 0);
	errno = 0;
	CHECK(ixc_lockf(a, 99, 0) == -1 && errno == EINVAL);

	close(a);
	close(b);
	unlink(name);
	if (!fails)
		printf("[OK] lockf\n");
	return fails;
}
