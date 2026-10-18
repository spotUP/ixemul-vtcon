/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* lockf, which ixemul has not (opkg locks its lock file with it). ixemul's
 * fcntl F_SETLK/F_WRLCK are compiled out (RECORD_LOCKING_NOT_IMPLEMENTED in
 * sys/fcntl.h, so fcntl gives EINVAL); flock works, so lockf is built on it.
 * flock locks the whole file: for len != 0 (a byte range) that is a superset
 * of the range asked for, so a lock that POSIX would grant on a disjoint
 * range of the same file is refused here. Callers locking one file as a
 * mutex (len 0, as opkg does) get exactly POSIX behaviour.
 * The lock belongs to the open file, not the process, and goes with the
 * last close. A held lock is reported as EACCES (POSIX lets EACCES or
 * EAGAIN; EACCES is what F_TEST and F_TLOCK give on BSD and Linux).
 * The Makefile's host test builds this with -DIXC_LOCKF=ixc_lockf. */
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>

#ifndef IXC_LOCKF
#define IXC_LOCKF lockf
#endif

int
IXC_LOCKF(int fd, int cmd, off_t len)
{
	(void)len;		/* whole-file lock; see above */

	switch (cmd) {
	case F_ULOCK:
		return flock(fd, LOCK_UN);
	case F_LOCK:
		return flock(fd, LOCK_EX);
	case F_TLOCK:
	case F_TEST:
		if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				errno = EACCES;
			return -1;
		}
		if (cmd == F_TEST)
			return flock(fd, LOCK_UN);
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}
