/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mktemp.c	5.9 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

/*
 * mktemp.c,v
 *
 * Revision 1.1  2026/09/10  ChatGPT modifications (JJ)
 *
 *    Prevent pointer underflow on empty and all-X templates.
 *    Restore the parent-directory separator before every error return.
 *    Use lstat() for name-only checks so existing symbolic links are detected.
 *    Add mkdtemp() using atomic mkdir() creation with mode 0700 and
 *    require at least six trailing X characters for the new interface.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <ctype.h>

static int
_gettemp(char *path, int *doopen, int domkdir)
{
	register char *start, *trv, *end;
	struct stat sbuf;
	u_int pid;
	int rval;
	usetup;

	if (doopen && domkdir) {
		errno = EINVAL;
		return(0);
	}

	/*
	 * Locate the trailing X run without ever forming a pointer before
	 * the start of the template.  Preserve the historical behaviour
	 * that also accepts a template without trailing X characters.
	 */
	for (end = path; *end; ++end)
		;
	if (end == path) {
		errno = EINVAL;
		return(0);
	}

	for (start = end; start > path && start[-1] == 'X'; --start)
		;

	/* mkdtemp() requires at least six trailing X characters. */
	if (domkdir && end - start < 6) {
		errno = EINVAL;
		return(0);
	}

	pid = syscall (SYS_getpid);
	for (trv = end; trv > start;) {
		--trv;
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/*
	 * Check the target directory.  Temporarily terminate the path at
	 * the slash, but always restore the slash before inspecting the
	 * result or returning.
	 */
	for (trv = start; trv > path;) {
		--trv;
		if (trv == path)
			break;
		if (*trv == '/') {
			*trv = '\0';
			rval = syscall (SYS_stat, path, &sbuf);
			*trv = '/';
			if (rval)
				return(0);
			if (!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				KPRINTF (("&errno = %lx, errno = %ld\n",
				    &errno, errno));
				return(0);
			}
			break;
		}
	}

	for (;;) {
		if (doopen) {
			if ((*doopen =
			    syscall (SYS_open, path, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		}
		else if (domkdir) {
			if (syscall (SYS_mkdir, path, 0700) == 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		}
		else if (syscall (SYS_lstat, path, &sbuf))
			return(errno == ENOENT ? 1 : 0);

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv)
				return(0);
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit(*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	/*NOTREACHED*/
}

int
mkstemp(char *path)
{
	int fd;

	return (_gettemp(path, &fd, 0) ? fd : -1);
}

char *
mktemp(char *path)
{
	return(_gettemp(path, (int *)NULL, 0) ? path : (char *)NULL);
}

char *
mkdtemp(char *path)
{
	return(_gettemp(path, (int *)NULL, 1) ? path : (char *)NULL);
}
