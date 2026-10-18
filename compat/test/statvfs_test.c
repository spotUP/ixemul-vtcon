/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* statvfs/fstatvfs (compat/statvfs.c). Host: -Dixc_statvfs/-Dixc_fstatvfs names
 * of the compat code; Amiga: reaches libixcompat.a through <sys/statvfs.h>.
 * Checks the invariants opkg relies on (free bytes = f_bavail * f_frsize) and
 * that path and descriptor forms agree. A write test: free space must drop when
 * a file of 1 MB is written and synced, by at least 512 KB (host filesystems allocate lazily) and
 * at most 3 MB: a wrong unit (512 vs 4096 byte blocks) misses both bounds. */
#include <sys/types.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef ixc_statvfs
int ixc_statvfs(const char *, struct statvfs *);
int ixc_fstatvfs(int, struct statvfs *);
#endif

static int fails;
#define CHECK(c) do { if (!(c)) { printf("[FAIL] line %d: %s (errno %d)\n", __LINE__, #c, errno); fails++; } } while (0)

static double
freebytes(const struct statvfs *v)
{
	return (double)v->f_bavail * (double)v->f_frsize;
}

int
main(void)
{
	struct statvfs a, b, c, d;
	char name[64], blk[4096];
	int fd, i;
	double before, after;

	CHECK(ixc_statvfs(".", &a) == 0);
	CHECK(a.f_frsize > 0 && a.f_bsize >= a.f_frsize);
	CHECK(a.f_blocks > 0);
	CHECK(a.f_bavail <= a.f_bfree && a.f_bfree <= a.f_blocks);
	CHECK(a.f_namemax > 0);

	fd = open(".", O_RDONLY);
	CHECK(fd >= 0);
	CHECK(ixc_fstatvfs(fd, &b) == 0);
	CHECK(b.f_frsize == a.f_frsize && b.f_blocks == a.f_blocks);
	close(fd);

	errno = 0;
	CHECK(ixc_statvfs("no_such_path_here", &c) == -1 && errno == ENOENT);
	errno = 0;
	CHECK(ixc_fstatvfs(-1, &c) == -1 && errno == EBADF);

	sprintf(name, "statvfs_test.%ld.tmp", (long)getpid());
	fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0600);
	CHECK(fd >= 0);
	CHECK(ixc_statvfs(".", &c) == 0);
	before = freebytes(&c);
	memset(blk, 'x', sizeof blk);
	for (i = 0; i < 256; i++)
		CHECK(write(fd, blk, sizeof blk) == sizeof blk);
	fsync(fd);
	CHECK(ixc_statvfs(".", &d) == 0);
	after = freebytes(&d);
	printf("free before %.0f after %.0f drop %.0f\n", before, after, before - after);
#ifndef IXC_HOST	/* a shared host disk moves under us; the Amiga volume is ours */
	CHECK(before - after >= 1048576.0 / 2);
	CHECK(before - after <= 1048576.0 + 2.0 * 1048576.0);
#endif
	close(fd);
	unlink(name);

	printf(fails ? "[FAIL] statvfs: %d\n" : "[OK] statvfs\n", fails);
	return fails != 0;
}
