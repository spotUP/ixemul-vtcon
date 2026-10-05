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
 * getline.c,v
 *
 * Revision 1.1  2026/07/06  Copilot modifications (JJ)
 *
 *    Added getline() as a wrapper around getdelim() using newline
 *    as the delimiter.
 */

#include <stdio.h>
#include <stdlib.h>

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);

ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}
