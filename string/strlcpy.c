/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * strlcpy.c,v
 *
 * Revision 1.1  2026/07/06  JJ, ChatGPT implementation
 *  Added C89-compatible strlcpy() implementation for ixemul.
 */

#include <sys/types.h>

size_t
strlcpy(char *dst, const char *src, size_t dstsize)
{
    const char *s;
    size_t n;
    size_t len;

    s = src;
    n = dstsize;

    /* copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*dst++ = *s++) == '\0') {
                return (s - src - 1);
            }
        }
        /* no space left, terminate */
        *dst = '\0';
    }

    /* count remaining length of src */
    while (*s++) {
        /* empty */
    }

    len = s - src - 1;
    return len;
}
