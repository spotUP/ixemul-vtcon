/*  $NetBSD: fread.c,v 1.6 1995/02/02 02:09:34 jtc Exp $  */

/*-
 * Copyright (c) 1990, 1993
 *  The Regents of the University of California.  All rights reserved.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 * fread.c,v
 *
 * Revision 1.6.2  2026/08/04  ChatGPT modifications (JJ)
 *
 *  Restrict usetup to the overflow error path.
 *
 * Revision 1.6.1 2026/06/25 JJ
 *
 *  Modernize fread() along the lines of later NetBSD stdio:
 *
 *  - Add overflow check before count * size using MUL_NO_OVERFLOW.
 *  - Set errno (EOVERFLOW/ERANGE) and __SERR on invalid total size.
 *  - Replace legacy _r<0 fixup with a cleaner _r<=0 refill path.
 *  - Use (int) cast when adjusting fp->_r to match field type.
 *
 *  No intentional change to BSD stdio semantics; improves robustness
 *  while preserving original buffering and refill behavior.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fread.c  8.2 (Berkeley) 12/11/93";
#endif
static char rcsid[] = "$NetBSD: fread.c,v 1.6 1995/02/02 02:09:34 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

size_t
fread(buf, size, count, fp)
    void *buf;
    size_t size, count;
    register FILE *fp;
{
    register size_t resid;
    register char *p;
    register int r;
    size_t total;

    /*
     * The ANSI standard requires a return value of 0 for a count
     * or a size of 0.
     */
    if ((size >= MUL_NO_OVERFLOW || count >= MUL_NO_OVERFLOW) &&
        size > 0 && count > (size_t)-1 / size) {
#if defined(EOVERFLOW) || defined(ERANGE)
        usetup;
#endif
#ifdef EOVERFLOW
        errno = EOVERFLOW;
#elif defined(ERANGE)
        errno = ERANGE;
#endif
        fp->_flags |= __SERR;
        return (0);
    }

    if ((resid = count * size) == 0)
        return (0);

    total = resid;
    p = buf;

    if (fp->_r <= 0)
        goto refill;

    while (resid > (size_t)(r = fp->_r)) {
        (void)memcpy((void *)p, (void *)fp->_p, (size_t)r);
        fp->_p += r;
        p += r;
        resid -= r;

refill:
        if (__srefill(fp)) {
            return ((total - resid) / size);
        }
    }

    (void)memcpy((void *)p, (void *)fp->_p, resid);
    fp->_r -= (int)resid;
    fp->_p += resid;

    return (count);
}
