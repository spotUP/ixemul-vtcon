/*  $NetBSD: fvwrite.c,v 1.4 1995/02/02 02:09:45 jtc Exp $  */

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
 * fvwrite.c,v
 *
 * Revision 1.4.2  2026/08/04  ChatGPT modifications (JJ)
 *
 *    Remove the invalid signed check of size_t uio_resid.
 *    Bound every underlying write and string-stream chunk to INT_MAX.
 *    Restore internal __sflush() calls and avoid flushing an empty buffer.
 *    Preserve efficient bounded direct writes for line-buffered streams.
 *    Remove normal-path usetup and preserve errors reported by cantwrite().
 *    Compute buffered space from the buffer pointers to avoid signed overflow.
 *
 * fvwrite.c,v 1.4.1 2026/06/25 JJ
 *
 * Modernize __sfvwrite() along the lines of later NetBSD stdio:
 *
 *  - Use ssize_t for write/copy progress and size_t for newline-distance
 *    and direct-write size calculations.
 *  - Validate uio_resid before processing and set errno explicitly for
 *    invalid input or non-writable streams (EINVAL, EBADF).
 *  - Allow unbuffered writes up to INT_MAX instead of BUFSIZ.
 *  - Improve the fully buffered direct-write path by writing the largest
 *    buffer-size multiple not exceeding INT_MAX.
 *  - Make newline-distance calculations size_t-clean.
 *
 * No intentional change to BSD stdio semantics; improves robustness and
 * large-write behavior while preserving the original buffering model.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fvwrite.c   8.1 (Berkeley) 6/4/93";
#endif
static char rcsid[] = "$NetBSD: fvwrite.c,v 1.4 1995/02/02 02:09:45 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "local.h"
#include "fvwrite.h"

/*
 * Write some memory regions.  Return zero on success, EOF on error.
 *
 * This routine is large and unsightly, but most of the ugliness due
 * to the three different kinds of output buffering is handled here.
 */
int
__sfvwrite(fp, uio)
    register FILE *fp;
    register struct __suio *uio;
{
    register size_t len;
    register char *p;
    register struct __siov *iov;
    register ssize_t w;
    char *nl;
    int nlknown, havenl;
    size_t n, s, nldist;
    size_t bufsize, used, space, copylen;

    if ((len = uio->uio_resid) == 0)
        return (0);

    /* make sure we can write */
    if (cantwrite(fp))
        return (EOF);

#define COPY(n) (void)memcpy((void *)fp->_p, (void *)p, (size_t)(n))

    iov = uio->uio_iov;
    p = iov->iov_base;
    len = iov->iov_len;
    iov++;

#define GETIOV(extra_work) \
    while (len == 0) { \
        extra_work; \
        p = iov->iov_base; \
        len = iov->iov_len; \
        iov++; \
    }

    if (fp->_flags & __SNBF) {
        /*
         * Unbuffered: write up to INT_MAX bytes at a time.
         */
        do {
            GETIOV(;);
            n = len;
            if (n > (size_t)INT_MAX)
                n = (size_t)INT_MAX;
            w = (*fp->_write)(fp->_cookie, p, (int)n);
            if (w <= 0)
                goto err;
            p += w;
            len -= (size_t)w;
        } while ((uio->uio_resid -= (size_t)w) != 0);

    } else if ((fp->_flags & __SLBF) == 0) {

        /*
         * Fully buffered.
         */
        do {
            GETIOV(;);

            if (fp->_flags & __SSTR) {
                /*
                 * Copy what fits, but consume the complete logical
                 * string-stream chunk so snprintf() can count it.
                 */
                n = len;
                if (n > (size_t)INT_MAX)
                    n = (size_t)INT_MAX;

                copylen = 0;
                if (fp->_w > 0) {
                    copylen = n;
                    if (copylen > (size_t)fp->_w)
                        copylen = (size_t)fp->_w;
                    if (copylen != 0) {
                        COPY(copylen);
                        fp->_w -= (int)copylen;
                        fp->_p += copylen;
                    }
                }
                w = (ssize_t)n;

            } else {
                bufsize = fp->_bf._size > 0 ?
                    (size_t)fp->_bf._size : 0;
                used = 0;
                if (fp->_bf._base != NULL &&
                    fp->_p > fp->_bf._base)
                    used = (size_t)(fp->_p - fp->_bf._base);
                space = used < bufsize ? bufsize - used : 0;

                if (used != 0 && len > space) {
                    /* Fill the partial buffer and flush it. */
                    copylen = space;
                    if (copylen != 0) {
                        COPY(copylen);
                        fp->_p += copylen;
                    }
                    if (__sflush(fp))
                        goto err;
                    w = (ssize_t)copylen;

                } else if (bufsize == 0 || len >= bufsize) {
                    /*
                     * Write the largest whole buffer multiple that
                     * does not exceed either len or INT_MAX.
                     */
                    n = len;
                    if (n > (size_t)INT_MAX)
                        n = (size_t)INT_MAX;
                    if (bufsize != 0)
                        n -= n % bufsize;
                    w = (*fp->_write)(fp->_cookie, p, (int)n);
                    if (w <= 0)
                        goto err;

                } else {
                    /* Copy the remainder into the stdio buffer. */
                    copylen = len;
                    COPY(copylen);
                    fp->_w -= (int)copylen;
                    fp->_p += copylen;
                    w = (ssize_t)copylen;
                }
            }

            p += w;
            len -= (size_t)w;

        } while ((uio->uio_resid -= (size_t)w) != 0);

    } else {

        /*
         * Line buffered.  Scan and emit no more than INT_MAX bytes
         * at a time so every callback length remains representable.
         */
        nlknown = 0;
        havenl = 0;
        nldist = 0;

        do {
            GETIOV(nlknown = 0; havenl = 0);

            if (!nlknown) {
                n = len;
                if (n > (size_t)INT_MAX)
                    n = (size_t)INT_MAX;
                nl = memchr((void *)p, '\n', n);
                if (nl != NULL) {
                    nldist = (size_t)(nl + 1 - p);
                    havenl = 1;
                } else {
                    nldist = n;
                    havenl = 0;
                }
                nlknown = 1;
            }

            s = nldist;
            bufsize = fp->_bf._size > 0 ?
                (size_t)fp->_bf._size : 0;
            used = 0;
            if (fp->_bf._base != NULL &&
                fp->_p > fp->_bf._base)
                used = (size_t)(fp->_p - fp->_bf._base);
            space = used < bufsize ? bufsize - used : 0;

            if (used != 0 && s > space) {
                /* Fill the partial buffer and flush it. */
                copylen = space;
                if (copylen != 0) {
                    COPY(copylen);
                    fp->_p += copylen;
                }
                if (__sflush(fp))
                    goto err;
                w = (ssize_t)copylen;

            } else if (bufsize == 0 || s >= bufsize) {
                /* Write directly through the newline or scan chunk. */
                w = (*fp->_write)(fp->_cookie, p, (int)s);
                if (w <= 0)
                    goto err;

            } else {
                /* Buffer the segment. */
                copylen = s;
                COPY(copylen);
                fp->_w -= (int)copylen;
                fp->_p += copylen;
                w = (ssize_t)copylen;
            }

            nldist -= (size_t)w;
            if (nldist == 0) {
                /* Flush only when a newline is buffered. */
                if (havenl && fp->_bf._base != NULL &&
                    fp->_p > fp->_bf._base && __sflush(fp))
                    goto err;
                nlknown = 0;
                havenl = 0;
            }

            p += w;
            len -= (size_t)w;

        } while ((uio->uio_resid -= (size_t)w) != 0);
    }

    return (0);

err:
    fp->_flags |= __SERR;
    return (EOF);
}
