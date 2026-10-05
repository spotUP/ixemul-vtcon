/*-
 * Copyright (c) 2005 Pascal Gloor <pascal.gloor@spale.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copilot modification (JJ) for ixemul.library, 2026.
 *
 * Adapted Pascal Gloor's memmem() implementation for ixemul.library.
 * An empty needle now returns the haystack, and additional fast paths
 * were added while retaining C89 compatibility.
 *
 * Local changes:
 *      - empty needle returns haystack
 *      - C89-compatible implementation
 */

#include <stddef.h>
#include <string.h>

void *
memmem(const void *haystack, size_t haystacklen,
       const void *needle, size_t needlelen)
{
    const unsigned char *h = haystack;
    const unsigned char *n = needle;
    size_t i;

    /* GNU-style early exits */
    if (needlelen == 0)
        return (void *)h;

    if (haystacklen < needlelen)
        return NULL;

    if (haystack == needle)
        return (void *)h;

    if (haystacklen == needlelen)
        return memcmp(h, n, needlelen) == 0 ? (void *)h : NULL;

    if (needlelen == 1)
        return memchr(h, n[0], haystacklen);

    /* Standard search */
    for (i = 0; i <= haystacklen - needlelen; i++) {
        if (h[i] == n[0] &&
            memcmp(h + i, n, needlelen) == 0)
            return (void *)(h + i);
    }

    return NULL;
}
