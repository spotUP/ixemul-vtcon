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
 * set_errno.c,v
 *
 * Revision 1.1  2026/08/05  ChatGPT modifications (JJ)
 *
 *    Rebind errno for AmiTCP-compatible stacks even when the optional
 *    usergroup.library is unavailable, as with a normal Roadshow setup.
 *
 *    Guard usergroup and AS225 calls by their actual library bases and use
 *    the V4 SocketBase tag API for h_errno only when that API is available.
 */

/* routines that need -ffixed-a4 compiled for them */
#define _KERNEL
#include "ixnet.h"
#include <amitcp/socketbasetags.h>
#include <amitcp/usergroup.h>

void
set_errno(int *real_errno, int *real_h_errno)
{
    usetup;
    struct ixnet *p;

    p = (struct ixnet *)u.u_ixnet;
    if (!p || !real_errno)
      return;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP && p->u_TCPBase)
      {
        TCP_SetErrnoPtr(real_errno, sizeof(*real_errno));

        if (p->u_UserGroupBase)
          {
            struct TagItem ug_list[] = {
              { UGT_INTRMASK, SIGBREAKB_CTRL_C },
              { UGT_ERRNOPTR(sizeof(int)), (ULONG)real_errno },
              { TAG_END }
            };

            (void)ug_SetupContextTagList(p->u_progname, ug_list);
          }

        if (real_h_errno &&
            ((struct Library *)p->u_TCPBase)->lib_Version >= 4)
          {
            struct TagItem tcp_list[] = {
              { SBTM_SETVAL(SBTC_HERRNOLONGPTR),
                (ULONG)real_h_errno },
              { TAG_END }
            };

            (void)TCP_SocketBaseTagList(tcp_list);
          }
      }
    else if (p->u_networkprotocol == IX_NETWORK_AS225 && p->u_SockBase)
      {
        SOCK_setup_sockets(128, real_errno);
      }
}
