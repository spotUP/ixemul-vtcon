/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* clock_gettime of ixemul.library (library/clock_gettime.c, vector 623): the toolchain's libc.a has no
 * glue for it. Jump through the library base like the glue of gen_glue (non baserel). */
#include "vecstub.h"

VEC_STUB(_clock_gettime, 3762);		/* -(623 + 4) * 6 */
