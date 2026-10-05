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
 * posix_memalign.c,v
 *
 * Revision 1.1  2026/08/16  ChatGPT modifications (JJ)
 *
 *    Added POSIX aligned-memory allocation using ixemul's existing
 *    memalign() allocator while preserving the caller's errno value.
 */

#define _KERNEL
#include "ixemul.h"

#include <stdlib.h>
#include <errno.h>

int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *p;
    int saved_errno;

    usetup;

    /*
     * POSIX requires alignment to be a power of two and a multiple
     * of sizeof(void *).  ixemul is a 32-bit m68k environment, so
     * the power-of-two test plus the sizeof(void *) lower bound is
     * sufficient.
     */
    if (alignment < sizeof(void *) ||
        (alignment & (alignment - 1)) != 0)
        return EINVAL;

    /*
     * ixemul's memalign() reports allocation errors through errno,
     * while posix_memalign() returns the error number directly.
     * Preserve the caller's errno value across the wrapper.
     */
    saved_errno = errno;
    p = memalign(alignment, size);
    errno = saved_errno;

    if (p == NULL)
        return ENOMEM;

    /*
     * Do not modify *memptr unless the allocation succeeded.
     */
    *memptr = p;
    return 0;
}
