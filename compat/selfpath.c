/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* ix_self_path: the running program's own file, for a program that has to
 * start itself again because AmigaOS has no fork() (GNU screen's backend,
 * tmux's server: vfork + exec of this file). The program's directory from
 * dos.library (GetProgramDir) plus the name it was started by; argv0 as
 * it is when there is none. (UP-Term; was screen-amiga's amiga/selfpath.c) */
#include <string.h>
#include <proto/dos.h>

void
ix_self_path(char *buf, int max, const char *argv0)
{
	BPTR dir;

	buf[0] = 0;
	if ((dir = GetProgramDir()) && NameFromLock(dir, (STRPTR)buf, max))
		AddPart((STRPTR)buf, FilePart((STRPTR)argv0), max);
	else {
		strncpy(buf, argv0, max - 1);
		buf[max - 1] = 0;
	}
}
