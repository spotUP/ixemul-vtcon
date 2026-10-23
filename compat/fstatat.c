/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* fstatat (POSIX.1-2008), which ixemul has not (nnn 5.3 dentfill and du call it with dirfd(dirp)).
 * AT_FDCWD or an absolute path is plain stat/lstat. Otherwise the directory behind dirfd is found the
 * way ixemul already does it for fchdir (library/fchdir.c: the descriptor's f_name, a directory opened
 * by open/opendir): fchdir to it, stat the relative name, chdir back. The detour is not re-entrant, which
 * is fine: ixemul programs have one thread (compat/pthread.c is a stub). A directory-relative name is
 * resolved by ixemul's own chdir, so Amiga volume syntax ("Vol:x") in name stays absolute. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int
fstatat(int dirfd, const char *name, struct stat *sb, int flag)
{
	char saved[PATH_MAX];
	int r, err;

	if (flag & ~AT_SYMLINK_NOFOLLOW) {
		errno = EINVAL;
		return -1;
	}
	if (name == NULL) {
		errno = EFAULT;
		return -1;
	}
	if (dirfd == AT_FDCWD || name[0] == '/' || strchr(name, ':') != NULL)
		return (flag & AT_SYMLINK_NOFOLLOW) ? lstat(name, sb) : stat(name, sb);
	if (getcwd(saved, sizeof saved) == NULL)
		return -1;
	if (fchdir(dirfd) != 0)
		return -1;	/* EBADF, or ENOTDIR when dirfd is not a directory */
	r = (flag & AT_SYMLINK_NOFOLLOW) ? lstat(name, sb) : stat(name, sb);
	err = errno;
	if (chdir(saved) != 0 && r == 0) {
		r = -1;
		err = errno;
	}
	errno = err;
	return r;
}
