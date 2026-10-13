/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Included first in string/'s all.c: these files implement memset,
 * memcpy, strlen and the rest. gcc 4.9+ turn a byte loop into a call to
 * memset/memcpy/memmove (and gcc 12+ strlen-style loops into strlen):
 * inside the implementation that is a call to itself, and memset built
 * with gcc 16 was `jra _memset` -- programs hung (UP-Term). */
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9))
#pragma GCC optimize ("no-tree-loop-distribute-patterns")
#endif
