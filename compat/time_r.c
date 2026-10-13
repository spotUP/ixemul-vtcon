/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* gmtime_r and localtime_r (POSIX) over ixemul's gmtime/localtime: the
 * result copied out of the library's static struct. ixemul programs have
 * one thread, so nothing can overwrite it in between. (libevent, tmux) */
#include <time.h>

struct tm *
gmtime_r(const time_t *t, struct tm *out)
{
	struct tm *r = gmtime(t);
	if (!r)
		return 0;
	*out = *r;
	return out;
}

struct tm *
localtime_r(const time_t *t, struct tm *out)
{
	struct tm *r = localtime(t);
	if (!r)
		return 0;
	*out = *r;
	return out;
}
