/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* The versioned stat family (vectors 657 to 660), in the style of NetBSD's __stat50.
 *
 * Programs built against include/sys/stat.h of this tree get a struct stat whose st_atim, st_mtim and
 * st_ctim are struct timespec, and stat, fstat, lstat and fstatat are renamed to these entry points.
 * Binaries built before keep calling the old vectors (stat, fstat, lstat) with the legacy layout. The
 * legacy functions stay the one source of the data: they fill the legacy struct, and st_spare1 carries
 * the nanoseconds of the DateStamp (ticks of 20 ms, see __stat and __fstat), which this file spreads
 * over the three times (a DateStamp is the only time an Amiga file has). The two layouts have the
 * same size and the same offsets for every field but the three times and the two Amiga fields. */

#define _KERNEL
#include "ixemul.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

/* st_amode and st_handler are macros for the legacy fields spare2 and spare3, which would also rewrite the
 * members of the new struct: name the legacy fields directly. */
#undef st_amode
#undef st_handler

static void
stat_to50 (struct __ix_stat50 *n, const struct stat *o)
{
  n->st_dev = o->st_dev;
  n->st_ino = o->st_ino;
  n->st_mode = o->st_mode;
  n->st_nlink = o->st_nlink;
  n->st_uid = o->st_uid;
  n->st_gid = o->st_gid;
  n->st_rdev = o->st_rdev;
  n->st_size = o->st_size;
  n->st_atim.tv_sec = o->st_atime;
  n->st_atim.tv_nsec = o->st_spare1;
  n->st_mtim.tv_sec = o->st_mtime;
  n->st_mtim.tv_nsec = o->st_spare1;
  n->st_ctim.tv_sec = o->st_ctime;
  n->st_ctim.tv_nsec = o->st_spare1;
  n->st_blksize = o->st_blksize;
  n->st_blocks = o->st_blocks;
  n->st_amode = o->st_spare2;
  n->st_handler = o->st_spare3;
}

int
__stat50 (const char *fname, struct __ix_stat50 *sb)
{
  struct stat o;

  if (stat (fname, &o) != 0)
    return -1;
  stat_to50 (sb, &o);
  return 0;
}

int
__lstat50 (const char *fname, struct __ix_stat50 *sb)
{
  struct stat o;

  if (lstat (fname, &o) != 0)
    return -1;
  stat_to50 (sb, &o);
  return 0;
}

int
__fstat50 (int fdes, struct __ix_stat50 *sb)
{
  struct stat o;

  if (fstat (fdes, &o) != 0)
    return -1;
  stat_to50 (sb, &o);
  return 0;
}

/* fstatat (POSIX.1-2008). AT_FDCWD, an absolute path or an Amiga volume name is plain stat/lstat.
 * Otherwise the directory behind dirfd is found the way fchdir does it (the descriptor's f_name):
 * fchdir to it, stat the relative name, chdir back. The detour is not re-entrant, which is fine:
 * ixemul programs have one thread (compat/pthread.c is a stub). */
int
__fstatat50 (int dirfd, const char *name, struct __ix_stat50 *sb, int flag)
{
  usetup;
  char saved[1024];
  int r, err;

  if (flag & ~AT_SYMLINK_NOFOLLOW)
    {
      errno = EINVAL;
      return -1;
    }
  if (name == NULL)
    {
      errno = EFAULT;
      return -1;
    }
  if (dirfd == AT_FDCWD || name[0] == '/' || strchr (name, ':') != NULL)
    return (flag & AT_SYMLINK_NOFOLLOW) ? __lstat50 (name, sb) : __stat50 (name, sb);
  if (getcwd (saved, sizeof saved) == NULL)
    return -1;
  if (fchdir (dirfd) != 0)
    return -1;	/* EBADF, or ENOTDIR when dirfd is not a directory */
  r = (flag & AT_SYMLINK_NOFOLLOW) ? __lstat50 (name, sb) : __stat50 (name, sb);
  err = errno;
  if (chdir (saved) != 0 && r == 0)
    {
      r = -1;
      err = errno;
    }
  errno = err;
  return r;
}
