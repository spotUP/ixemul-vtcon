/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * strlcat.c,v
 *
 * Revision 1.1  2026/07/06  ChatGPT modifications (JJ)
 *
 *    Adapt the Todd C. Miller strlcat() implementation for ixemul and
 *    GCC 2.95.3 using C89-compatible declarations.
 */

#include <sys/types.h>

size_t
strlcat(char *dst, const char *src, size_t dstsize)
{
    const char *s;
    char *d;
    size_t n;
    size_t dlen;

    d = dst;
    s = src;
    n = dstsize;

    /* find end of dst */
    while (n != 0 && *d != '\0') {
        d++;
        n--;
    }

    dlen = d - dst;
    n = dstsize - dlen;

    if (n == 0) {
        /* no space left */
        while (*s++) {
            /* empty */
        }
        return dlen + (size_t)(s - src - 1);
    }

    /* copy src */
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }

    *d = '\0';

    return dlen + (size_t)(s - src);
}
