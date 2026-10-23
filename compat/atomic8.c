/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* 64-bit atomic helpers that gcc calls for __atomic_fetch_or on a long long: m68k has no 8-byte
 * compare-and-swap and the toolchain ships no libatomic. AmigaOS runs on one CPU, so a read-modify-write
 * is atomic against other tasks when interrupts and task switches are off; Forbid() stops the switch.
 * The data in question (nnn's hard-link bitmap) is private to the process. */
#include <proto/exec.h>

unsigned long long
__atomic_fetch_or_8(volatile void *ptr, unsigned long long val, int memorder)
{
	volatile unsigned long long *p = ptr;
	unsigned long long old;
	struct ExecBase *SysBase = *(struct ExecBase **)4;	/* not the program's global, which may be unset */

	(void)memorder;
	Forbid();
	old = *p;
	*p = old | val;
	Permit();
	return old;
}
