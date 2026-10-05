/*  $NetBSD: fwrite.c,v 1.5 1995/02/02 02:09:51 jtc Exp $   */

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
 * fwrite.c,v
 *
 * Revision 1.5.2  2026/08/04  ChatGPT modifications (JJ)
 *
 *   Restrict usetup to the overflow error path.
 *
 * Revision 1.5.1  2026/06/25  ChatGPT modifications  (JJ)
 *
 *  Modernized fwrite() along the lines of later NetBSD stdio:
 *  add MUL_NO_OVERFLOW before the count * size overflow check, return 0 for
 *  zero-size requests, build __suio with size_t uio_resid, and use the
 *  NetBSD-style return path that adjusts count only on __sfvwrite() failure.
 *
 *  No intentional change to normal successful write semantics.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fwrite.c    8.1 (Berkeley) 6/4/93";
#endif
static char rcsid[] = "$NetBSD: fwrite.c,v 1.5 1995/02/02 02:09:51 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <errno.h>
#include <stdio.h>
#include "local.h"
#include "fvwrite.h"

#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

/*
 * Write `count' objects (each size `size') from memory to the given file.
 * Return the number of whole objects written.
 */
size_t
fwrite(buf, size, count, fp)
    const void *buf;
    size_t size, count;
    FILE *fp;
{
    size_t n;
    struct __suio uio;
    struct __siov iov;

    /*
     * Extension: catch integer overflow.
     *
     * Avoid the division in the common case.  If both operands are
     * smaller than sqrt(SIZE_MAX + 1), the product cannot overflow.
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

    /*
     * SUSv2 requires a return value of 0 for a count or a size of 0.
     */
    if ((n = count * size) == 0)
        return (0);

    iov.iov_base = (void *)buf;
    uio.uio_resid = iov.iov_len = n;
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;

    /*
     * The usual case is success (__sfvwrite returns 0);
     * skip the divide if this happens, since divides are
     * generally slow.
     */
    if (__sfvwrite(fp, &uio) != 0)
        count = ((n - uio.uio_resid) / size);

    return (count);
}
