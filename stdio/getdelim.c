/*
 * This file is part of ixemul.library for the Amiga.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * getdelim.c,v
 *
 * Revision 1.1  2026/07/06  Copilot modifications (JJ)
 *
 *    Added a C89-compatible getdelim() implementation for
 *    ixemul.library using its existing stdio interfaces.
 */

/*
 * getdelim() ixemul compatible full implementation
 *
 * This version avoids flockfile(), funlockfile(), getc_unlocked(),
 * and other glibc specific features. It is fully POSIX compatible
 * and works with ixemul's 1991 FILE implementation.
 */

/* usetup: the library's own header, not whichever file all.c put first */
#define _KERNEL
#include "ixemul.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/* ixemul lacks SIZE_MAX in <limits.h> */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

ssize_t
getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    int c;
    size_t pos = 0;
    char *buf;
    size_t new_size;
    char *new_buf;
    usetup;



    if (!lineptr || !n || !stream)
        return -1;

    if (delim == EOF)
        return -1;

    /* Allocate initial buffer if needed */
    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (!*lineptr) {
            errno = ENOMEM;
            return -1;
        }
    }

    buf = *lineptr;

    /* Early error check */
    if (ferror(stream))
        return -1;

    /* Main read loop */
    while ((c = fgetc(stream)) != EOF) {

        /* Ensure space for c + '\0' */
        if (pos + 1 >= *n) {

            /* Overflow guard */
            if (*n > SIZE_MAX / 2) {
                errno = ENOMEM;
                return -1;
            }

            new_size = *n * 2;
            if (new_size < pos + 2)
                new_size = pos + 2;

            new_buf = realloc(buf, new_size);
            if (!new_buf) {
                errno = ENOMEM;
                return -1;
            }

            buf = new_buf;
            *lineptr = buf;
            *n = new_size;
        }

        buf[pos++] = (char)c;

        if (c == delim)
            break;
    }

    /* Distinguish EOF from error */
    if (ferror(stream))
        return -1;

    /* If nothing read and EOF, return -1 */
    if (pos == 0 && c == EOF)
        return -1;

    buf[pos] = '\0';
    return pos;
}
