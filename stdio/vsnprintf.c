/*	$NetBSD: vsnprintf.c,v 1.5 1995/02/02 02:10:54 jtc Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  Revision 1.6  2026/06/07  Copilot modifications (JJ)
 *  - Fixed vsnprintf() handling of n == 0.
 *    Use a zero-sized __SSTR FILE so vfprintf() can count the
 *    formatted output without writing to the caller's buffer.
 *    Do not set __SERR here; vfprintf() returns EOF when the
 *    stream error flag is set.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)vsnprintf.c	8.1 (Berkeley) 6/4/93";
#endif
static char rcsid[] = "$NetBSD: vsnprintf.c,v 1.5 1995/02/02 02:10:54 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stdio.h>

int
vsnprintf(str, n, fmt, ap)
	char *str;
	size_t n;
	const char *fmt;
	_BSD_VA_LIST_ ap;
{
    int ret;
    FILE f;
    FILE f2;
    _BSD_VA_LIST_ ap2;

    if (n == 0) {
        unsigned char dummy[1];

        /*
         * Use a zero-sized string FILE.  In __SSTR mode,
         * __sfvwrite() copies only the bytes that fit, but
         * pretends that the whole iovec was consumed.  With
         * _w == 0 this gives count-only behaviour without
         * writing to the caller's buffer.
         *
         * Do not set __SERR here; vfprintf() returns EOF if
         * the stream error flag is set.
         */
        ap2 = ap;
        f2._flags = __SWR | __SSTR;
        f2._bf._base = f2._p = dummy;
        f2._bf._size = f2._w = 0;

        return vfprintf(&f2, fmt, ap2);
    }

    ap2 = ap;
    f._flags = __SWR | __SSTR;
    f._bf._base = f._p = (unsigned char *)str;
    f._bf._size = f._w = n - 1;
    ret = vfprintf(&f, fmt, ap2);
    *f._p = 0;
    return (ret);
}
