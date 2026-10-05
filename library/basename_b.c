/*
 * Copyright (c) 2026
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

 /*
 * basename_b.c,v
 *
 * Revision 1.1  2026/07/06  JJ, ChatGPT implementation
 *  Initial basename_b() helper implementation.
 */

/*
 * basename_b.c - C89 deterministic basename helper for ixemul.library. ChatGPT implementation.
 *
 * BSD-style deterministic buffer helper with ixemul Amiga-path extension.
 *
 * It does not modify path.
 * It does not use a static result buffer.
 * It does not allocate memory.
 *
 * Path rules:
 *   '/' is treated as Unix separator.
 *   ':' is treated as Amiga root/separator if no '/' appears before it.
 */

#include "ixemul.h"
#include <sys/types.h>
#include <string.h>
#include <errno.h>

static char *copy_component(char *buf, size_t buflen,
                            const char *start, const char *end);

/*
 * basename_b() - copy final pathname component to caller buffer
 *
 * This function does not modify path, does not use a static result
 * buffer, and does not allocate memory.
 *
 * Path rules:
 *   '/' is treated as Unix separator.
 *   ':' is treated as Amiga root/separator if no '/' appears before it.
 *
 * Returns buf on success.
 * Returns NULL on error and sets errno.
 *
 * Errors:
 *   EINVAL       buf is NULL or buflen is 0
 *   ENAMETOOLONG result does not fit in buf
 */
char *
basename_b(const char *path, char *buf, size_t buflen)
{
    usetup;
    const char *end;
    const char *p;
    const char *base;
    const char *colon;
    const char *slash;
    size_t len;

    if (buf == NULL || buflen == 0) {
        errno = EINVAL;
        return NULL;
    }

    if (path == NULL || path[0] == '\0') {
        return copy_component(buf, buflen, ".", "." + 1);
    }

    len = strlen(path);
    end = path + len;

    /*
     * Trim trailing slashes virtually.
     *
     * "/"      -> "/"
     * "///"    -> "/"
     * "SYS:/"  -> "SYS:"
     * ":C/"    -> ":C"
     */
    while (end > path + 1 && end[-1] == '/') {
        end--;
    }

    /*
     * Detect Amiga-style ':' separator within the trimmed range.
     * Only treat ':' as Amiga separator if no '/' appears before it.
     */
    colon = NULL;
    p = path;
    while (p < end) {
        if (*p == ':') {
            colon = p;
            break;
        }
        p++;
    }

    if (colon != NULL) {
        slash = NULL;
        p = path;
        while (p < colon) {
            if (*p == '/') {
                slash = p;
                break;
            }
            p++;
        }

        if (slash != NULL) {
            colon = NULL;
        }
    }

    /*
     * Unix root.
     */
    if (end == path + 1 && path[0] == '/') {
        return copy_component(buf, buflen, path, end);
    }

    /*
     * Amiga root, e.g. ":" or "SYS:".
     */
    if (colon != NULL && colon + 1 == end) {
        return copy_component(buf, buflen, path, end);
    }

    /*
     * Prefer Unix slash as final component separator.
     */
    base = NULL;
    p = path;
    while (p < end) {
        if (*p == '/') {
            base = p + 1;
        }
        p++;
    }

    if (base != NULL) {
        return copy_component(buf, buflen, base, end);
    }

    /*
     * Amiga path without '/', e.g. "SYS:C" or ":C".
     */
    if (colon != NULL) {
        return copy_component(buf, buflen, colon + 1, end);
    }

    return copy_component(buf, buflen, path, end);
}

static char *
copy_component(char *buf, size_t buflen, const char *start, const char *end)
{
    usetup;
    size_t len;

    len = (size_t)(end - start);

    if (len + 1 > buflen) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    memcpy(buf, start, len);
    buf[len] = '\0';

    return buf;
}
