/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Stubs for the versioned stat entry points of ixemul.library (library/stat50.c, vectors 657 to 660).
 * include/sys/stat.h renames stat, fstat, lstat and fstatat of a new build to __stat50, __fstat50,
 * __lstat50 and __fstatat50. The toolchain's libc.a has glue only for the vectors that existed when
 * it was built, so these four jump through the library base like the glue of gen_glue does (non
 * baserel: movel _ixemulbase,a0; jmp a0@(-(vector + 4) * 6)). A program that calls them needs an
 * ixemul.library with these vectors (vtcon-4 or newer); on an older one the jump lands outside the
 * table. Not for -fbaserel code: _ixemulbase would have to be a4 relative.
 * One object per entry point, so a program (gnulib has its own fstatat) that defines one of them does
 * not collide with the other three pulled from libixcompat.a. */
#include "vecstub.h"

VEC_STUB(__stat50, 3966);
