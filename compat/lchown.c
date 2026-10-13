/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* lchown, which ixemul has not (GNU coreutils 9 copy.c: HAVE_LCHOWN undeclared without it). ixemul's chown
 * already acts on the directory entry the name ends in (ACTION_SET_OWNER on the parent lock and the last
 * component, library/chown.c), so a soft link's own owner changes and the link is not followed: that is
 * lchown's meaning. */
#include <sys/types.h>
#include <unistd.h>

int
lchown(const char *path, uid_t owner, gid_t group)
{
	return chown(path, owner, group);
}
