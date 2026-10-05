/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 Jeff Shepherd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id: $
 *
 *  $Log:$
 *
 */

/*
 * misc.c,v
 *
 * Revision 1.2  2026/08/05  ChatGPT modifications (JJ)
 *
 *    Guard AmiTCP usergroup calls with the actual usergroup.library base.
 *    Roadshow without that optional library now keeps the existing local
 *    umask implementation and returns NULL for unavailable crypt().
 */

/*
 * Revision 1.1 2026/06/08 ChatGPT modifications (JJ)
 *  Align ixnet sethostname() prototype with <unistd.h>.
 *  The length argument is now size_t instead of int, and the local
 *  bounded copy length uses size_t as well.
 *  No behavioural change intended.
 */

/* stubs for group-file access functions */

#define _KERNEL
#include "ixnet.h"
#include "kprintf.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

extern char *getenv(const char *);

int gethostname (char *name, int namelen)
{
    usetup;
    char *host;
    struct ixnet *p = (struct ixnet *)u.u_ixnet;

    switch (p->u_networkprotocol) {
	case IX_NETWORK_AMITCP:
	    return TCP_GetHostName(name,namelen);

	default: /* case IX_NETWORK_AS225: */
	    host = getenv("HOSTNAME");
	    if (host || (host = getenv("hostname")))
	    {
                char domain[257];

	        if (!strchr(host, '.'))
	        {
	            int len;
	          
	            strcpy(domain, host);
	            len = strlen(domain);
	            domain[len] = '.';
	            if (!getdomainname(domain + len + 1, sizeof(domain) - 1 - len))
	                host = domain;
	        }
		strncpy (name, host, namelen);
	    }
	    else
		strncpy (name, "localhost", namelen);
	    return 0;
    }
}

static char hostname[MAXHOSTNAMELEN] = "localhost";

int sethostname (const char *name, size_t namelen)
{
  usetup;
  struct ixnet *p = (struct ixnet *)u.u_ixnet;

  if (p->u_networkprotocol != IX_NETWORK_AMITCP) {
    size_t len = namelen < sizeof (hostname) - 1 ? namelen : sizeof (hostname) - 1;

    strncpy (hostname, name, len);
    hostname[len] = 0;
  }
  return 0;
}

char *crypt (const char *key, const char *setting)
{
  usetup;
  struct ixnet *p = (struct ixnet *)u.u_ixnet;

  if (p->u_networkprotocol == IX_NETWORK_AMITCP &&
      p->u_UserGroupBase)
    return UG_crypt(key,setting);
  return NULL;
}

mode_t
umask (mode_t mode)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP &&
        p->u_UserGroupBase) {
        return UG_umask(mode);
    }
    else {
        int md = (int)mode;
        mode_t res;
        if (md < 0 || (int)md > 0777) {
	    errno = EINVAL;
	    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	    return -1;
	}

	/* one day I'll use this field ;-)) */
	mode &= ((1<<9)-1);
	res = u.u_cmask;
	u.u_cmask = mode;
	return res;
    }
}
