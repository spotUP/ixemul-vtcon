/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* __eprintf: what ixemul's <assert.h> calls on a failed assertion. libgcc
 * has one, but it writes through newlib's _impure_ptr, which an ixemul
 * program does not have (link error "undefined reference to _impure_ptr"
 * from _eprintf.o, measured on GNU grep 3.12). This one uses ixemul's
 * stderr. Moved here from neovim-amiga's amiga/compat/eprintf.c so every
 * port has it. */
#include <stdio.h>
#include <stdlib.h>

void
__eprintf(const char *fmt, const char *file, int line, const char *expr)
{
	fprintf(stderr, fmt, file, (unsigned)line, expr);
	fflush(stderr);
	abort();
}
