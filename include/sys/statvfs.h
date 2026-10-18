/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* statvfs(3) on statfs (compat/statvfs.c, libixcompat.a). */
#ifndef _SYS_STATVFS_H_
#define _SYS_STATVFS_H_

#include <sys/cdefs.h>
#include <sys/types.h>

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

struct statvfs {
	unsigned long	f_bsize;	/* file system block size */
	unsigned long	f_frsize;	/* fundamental block size: unit of f_blocks, f_bfree, f_bavail */
	fsblkcnt_t	f_blocks;	/* total blocks, in f_frsize units */
	fsblkcnt_t	f_bfree;	/* free blocks */
	fsblkcnt_t	f_bavail;	/* free blocks for unprivileged users */
	fsfilcnt_t	f_files;	/* inodes; 0 when the handler does not say */
	fsfilcnt_t	f_ffree;
	fsfilcnt_t	f_favail;
	unsigned long	f_fsid;
	unsigned long	f_flag;		/* ST_* */
	unsigned long	f_namemax;	/* longest file name (AmigaDOS: 107) */
};

#define	ST_RDONLY	0x00000001	/* read only file system */
#define	ST_NOSUID	0x00000002	/* setuid/setgid not honoured */

__BEGIN_DECLS
int	statvfs __P((const char *, struct statvfs *));
int	fstatvfs __P((int, struct statvfs *));
__END_DECLS

#endif /* !_SYS_STATVFS_H_ */
