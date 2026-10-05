/*-
 * Copyright (c) 1990 The Regents of the University of California.
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

/*
 * Revision 1.1.1.1  2026/06/18  ChatGPT modifications (JJ)
 *
 * - Hardened calloc() against size_t multiplication overflow before
 *   allocating and zeroing memory.
 *
 * - Added an overflow check for num * size before calling SYS_malloc.
 * - Set errno to ENOMEM and return NULL when the requested allocation
 *   size cannot be represented.
 * - Keep cfree() behavior unchanged.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)calloc.c	5.5 (Berkeley) 5/17/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

void *
calloc (size_t num, size_t size)
{
	register void *p;
	size_t total;
	usetup;

	if (size != 0 && num > (size_t)-1 / size) {
		errno = ENOMEM;
		return NULL;
	}

	total = num * size;
	if ((p = (void *) syscall (SYS_malloc, total)))
		bzero(p, total);
	return(p);
}

void
cfree(void *p)
{
	(void)syscall (SYS_free, p);
}
