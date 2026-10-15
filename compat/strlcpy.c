/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* strlcpy and strlcat (OpenBSD semantics): the return value is the length the
 * result would have had, so a result >= size means truncation. */
#include <string.h>

size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t n = strlen(src);

	if (size) {
		size_t c = n >= size ? size - 1 : n;
		memcpy(dst, src, c);
		dst[c] = '\0';
	}
	return n;
}

size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t dl = strnlen(dst, size);
	size_t sl = strlen(src);

	if (dl == size)		/* dst not terminated within size */
		return size + sl;
	if (sl >= size - dl) {
		memcpy(dst + dl, src, size - dl - 1);
		dst[size - 1] = '\0';
	} else {
		memcpy(dst + dl, src, sl + 1);
	}
	return dl + sl;
}
