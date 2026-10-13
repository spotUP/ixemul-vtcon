/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* alloca.h for the ixemul SDK (UP-Term): gcc's builtin, which <stdlib.h>
 * declares too. Without this header the toolchain's newlib <alloca.h> was
 * found, whose <sys/reent.h> redefines struct __sFILE against ixemul's
 * stdio (ncurses 6.6's nc_alloc.h, in the UP-Term ports build). */
#ifndef _ALLOCA_H_
#define _ALLOCA_H_

#include <sys/cdefs.h>
#include <stddef.h>

#undef alloca
#define alloca(size)	__builtin_alloca(size)

#endif /* _ALLOCA_H_ */
