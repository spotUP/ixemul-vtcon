/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* reallocarray, which the SDK's 48.2 stub set declares but has no code for
 * (OpenBSD column: undefined reference to strtonum, reallocarray next).
 * OpenBSD reallocarray.c 1.2: realloc(ptr, nmemb * size), or NULL with
 * errno ENOMEM when the product overflows size_t (ptr is left allocated).
 * The host test builds this file with -DIXC_REALLOCARRAY=ixc_reallocarray. */
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#ifndef IXC_REALLOCARRAY
#define IXC_REALLOCARRAY reallocarray
#endif

#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void *
IXC_REALLOCARRAY(void *ptr, size_t nmemb, size_t size)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb != 0 && size > (size_t)-1 / nmemb) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, nmemb * size);
}
