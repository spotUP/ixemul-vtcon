/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* fstatat_probe: rig test for the versioned stat entry points (library/stat50.c, compat/stat50.c) and
 * fstatat. It also calls the legacy entry points through test/legacy_stat.c. Run it from a writable directory (RAM:). It builds
 * ./fsa/{file,sub/} and checks the dirfd forms against plain stat, the cwd afterwards, and the errors.
 * Prints [OK]/[FAIL] lines; exit status is the number of failures. */
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "legacy_stat.h"

static int fails;
#define CHECK(c) do { if (c) printf("[OK] %s\n", #c); else { printf("[FAIL] line %d: %s (errno %d)\n", __LINE__, #c, errno); fails++; } } while (0)

int
main(void)
{
	struct stat a, b, c;
	struct legacy_result o;
	char before[512], after[512];
	DIR *d;
	int fd, dfd;

	mkdir("fsa", 0777);
	mkdir("fsa/sub", 0777);
	fd = open("fsa/file", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	write(fd, "12345", 5);
	close(fd);
	getcwd(before, sizeof before);

	/* versioned stat, fstat and lstat against the legacy entry points */
	CHECK(stat("fsa/file", &a) == 0 && legacy_stat(0, -1, "fsa/file", &o) == 0);
	CHECK(a.st_mtim.tv_sec == o.mtime && a.st_mtime == o.mtime && a.st_size == o.size && a.st_mode == o.mode);
	CHECK(a.st_mtim.tv_nsec >= 0 && a.st_mtim.tv_nsec < 1000000000 && a.st_mtim.tv_nsec % 20000000 == 0);
	CHECK(a.st_atim.tv_sec == a.st_mtim.tv_sec && a.st_ctim.tv_sec == a.st_mtim.tv_sec);
	CHECK(lstat("fsa/file", &b) == 0 && legacy_stat(1, -1, "fsa/file", &o) == 0 && b.st_mtim.tv_sec == o.mtime && b.st_size == o.size);
	fd = open("fsa/file", O_RDONLY);
	CHECK(fd >= 0 && fstat(fd, &c) == 0 && legacy_stat(2, fd, NULL, &o) == 0);
	CHECK(c.st_mtim.tv_sec == o.mtime && c.st_mtim.tv_sec == a.st_mtim.tv_sec && c.st_size == 5 && S_ISREG(c.st_mode));
	CHECK(c.st_mtim.tv_nsec == a.st_mtim.tv_nsec);
	close(fd);
	CHECK(stat("fsa/nope", &b) == -1 && errno == ENOENT);

	CHECK(stat("fsa/file", &a) == 0 && a.st_size == 5);
	CHECK(fstatat(AT_FDCWD, "fsa/file", &b, 0) == 0 && b.st_size == 5 && b.st_mtime == a.st_mtime && b.st_mtim.tv_sec == a.st_mtim.tv_sec && b.st_mtim.tv_nsec == a.st_mtim.tv_nsec);
	CHECK(fstatat(AT_FDCWD, "fsa/file", &b, AT_SYMLINK_NOFOLLOW) == 0 && b.st_size == 5);

	d = opendir("fsa");
	CHECK(d != NULL);
	dfd = dirfd(d);
	CHECK(fstatat(dfd, "file", &b, 0) == 0 && b.st_size == 5 && S_ISREG(b.st_mode));
	CHECK(fstatat(dfd, "file", &b, AT_SYMLINK_NOFOLLOW) == 0 && b.st_size == 5);
	CHECK(fstatat(dfd, "sub", &b, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(b.st_mode));
	CHECK(fstatat(dfd, "nope", &b, 0) == -1 && errno == ENOENT);
	getcwd(after, sizeof after);
	CHECK(strcmp(before, after) == 0);	/* the detour restored the cwd */
	CHECK(fstatat(dfd, "file", &b, 0x7000) == -1 && errno == EINVAL);
	CHECK(fstatat(-1, "file", &b, 0) == -1 && errno == EBADF);
	closedir(d);

	fd = open("fsa/file", O_RDONLY | O_CLOEXEC);
	CHECK(fd >= 0);
	CHECK(fstatat(fd, "x", &b, 0) == -1 && errno == ENOTDIR);	/* dirfd is a file */
	close(fd);
	CHECK(sysconf(_SC_NPROCESSORS_ONLN) == 1 || sysconf(_SC_NPROCESSORS_ONLN) == -1);

	unlink("fsa/file");
	rmdir("fsa/sub");
	rmdir("fsa");
	printf("fstatat_probe: %s\n", fails ? "FAIL" : "ok");
	return fails;
}
