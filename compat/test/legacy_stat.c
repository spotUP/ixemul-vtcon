/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Calls the legacy (unversioned) stat, lstat and fstat of ixemul.library with the legacy struct stat,
 * for fstatat_probe. The legacy layout is selected with __IX_OLD_STAT before the include. */
#define __IX_OLD_STAT 1
#include <sys/stat.h>
#include "legacy_stat.h"

int
legacy_stat(int how, int fd, const char *path, struct legacy_result *r)
{
	struct stat s;
	int rc;

	rc = how == 0 ? stat(path, &s) : how == 1 ? lstat(path, &s) : fstat(fd, &s);
	if (rc == 0) {
		r->mtime = s.st_mtime;
		r->size = s.st_size;
		r->mode = s.st_mode;
	}
	return rc;
}
