/*
 * Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this file; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place -
 * Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * strndup.c,v
 *
 * Revision 1.1  2026/09/23  ChatGPT modifications (JJ)
 *
 *    Adapt the GNU C Library strndup() implementation for
 *    ixemul.library and GCC 2.95.3.
 */

#include <stdlib.h>
#include <string.h>

char *
strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *new = malloc(len + 1);

    if (!new)
        return NULL;

    memcpy(new, s, len);
    new[len] = '\0';

    return new;
}
