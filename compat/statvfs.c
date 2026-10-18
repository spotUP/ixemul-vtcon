/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* statvfs and fstatvfs for opkg (free space = f_bavail * f_frsize). Built on
 * ixemul's statfs/fstatfs, which fill f_blocks/f_bfree/f_bavail in units of
 * f_fsize (the handler's id_BytesPerBlock) and f_bsize as that times the
 * buffer factor (library/statfs.c). So f_frsize = f_fsize, and the products
 * f_bavail * f_frsize are bytes. ixemul reports unknown inode counts as -1;
 * those become 0. The host test builds this with -DIXC_HOST (the host's
 * statfs has no f_fsize; its f_bsize is the fundamental block size) and
 * -DIXC_STATVFS=ixc_statvfs -DIXC_FSTATVFS=ixc_fstatvfs. */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <string.h>

#ifndef IXC_STATVFS
#define IXC_STATVFS statvfs
#endif
#ifndef IXC_FSTATVFS
#define IXC_FSTATVFS fstatvfs
#endif

static unsigned long
cnt(long v)
{
	return v < 0 ? 0 : (unsigned long)v;
}

static void
fill(struct statvfs *v, const struct statfs *s)
{
#ifdef IXC_HOST
	unsigned long frsize = s->f_bsize;
#else
	unsigned long frsize = s->f_fsize;
#endif
	memset(v, 0, sizeof *v);
	v->f_frsize = frsize ? frsize : s->f_bsize;
	v->f_bsize = s->f_bsize ? s->f_bsize : v->f_frsize;
	v->f_blocks = cnt(s->f_blocks);
	v->f_bfree = cnt(s->f_bfree);
	v->f_bavail = cnt(s->f_bavail);
	v->f_files = cnt(s->f_files);
	v->f_ffree = cnt(s->f_ffree);
	v->f_favail = v->f_ffree;
#ifndef IXC_HOST
	v->f_fsid = (unsigned long)s->f_fsid.val[0];
	if (s->f_flags & MNT_RDONLY)
		v->f_flag |= ST_RDONLY;
	if (s->f_flags & MNT_NOSUID)
		v->f_flag |= ST_NOSUID;
#else
	if (s->f_flags & MNT_RDONLY)
		v->f_flag |= ST_RDONLY;
#endif
	v->f_namemax = 107;
}

int
IXC_STATVFS(const char *path, struct statvfs *v)
{
	struct statfs s;

	memset(&s, 0, sizeof s);
	if (statfs(path, &s) < 0)
		return -1;
	fill(v, &s);
	return 0;
}

int
IXC_FSTATVFS(int fd, struct statvfs *v)
{
	struct statfs s;

	memset(&s, 0, sizeof s);
	if (fstatfs(fd, &s) < 0)
		return -1;
	fill(v, &s);
	return 0;
}
