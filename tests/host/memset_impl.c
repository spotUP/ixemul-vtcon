/* Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 * string/memset.c under another name, for tests/host/test_memset.c. */
#include <sys/types.h>
#include <string.h>
#undef memset
#define memset ixemul_memset
#include "../../string/memset.c"
