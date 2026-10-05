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
 *  $Id:$
 *
 *  $Log:$
 *
 */

/*
 * ixnet_close.c,v
 *
 * Revision 1.1  2026/08/05  ChatGPT modifications (JJ)
 *
 *    Make per-process network teardown safe for partial initialization.
 *
 *    Disable AmiTCP-compatible socket signals before closing the private
 *    SocketBase, close each library at most once, and free signal bits only
 *    after the network libraries can no longer deliver them.
 *
 *    Clear u_ixnet before releasing the per-process ixnet structure.
 */

#define _KERNEL
#include "ixnet.h"
#include "kprintf.h"

extern struct ExecBase	*SysBase;

void
ixnet_close (struct ixnet_base *ixbase)
{
    usetup;
    struct ixnet *p;

    (void)ixbase;
    p = (struct ixnet *)u.u_ixnet;
    if (!p)
      return;

    if (p->u_SockBase)
      {
        SOCK_cleanup_sockets();
        CloseLibrary(p->u_SockBase);
        p->u_SockBase = NULL;
      }

    if (p->u_UserGroupBase)
      {
        CloseLibrary(p->u_UserGroupBase);
        p->u_UserGroupBase = NULL;
      }

    if (p->u_TCPBase)
      {
        /* Prevent delivery to signal bits after they are released. */
        TCP_SetSocketSignals(0, 0, 0);
        CloseLibrary(p->u_TCPBase);
        p->u_TCPBase = NULL;
      }

    if (p->u_sigurg >= 0)
      {
        FreeSignal(p->u_sigurg);
        p->u_sigurg = -1;
      }

    if (p->u_sigio >= 0)
      {
        FreeSignal(p->u_sigio);
        p->u_sigio = -1;
      }

    u.u_ixnet = NULL;
    FreeMem(p, sizeof(struct ixnet));
}
