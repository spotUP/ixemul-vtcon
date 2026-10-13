/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* if_nametoindex (RFC 3493): ixemul has no interface index table, so no
 * name has an index -- 0, "no such interface", as the RFC says for an
 * unknown name. (libevent parses "fe80::1%eth0" scopes with it) */
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

unsigned int
if_nametoindex(const char *name)
{
	(void)name;
	return 0;
}
