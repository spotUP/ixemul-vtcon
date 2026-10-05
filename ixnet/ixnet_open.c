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
 */

/*
 * Revision 1.2  2026/08/06  ChatGPT modifications (JJ)
 *
 *    Validate the current task and its ixemul user structure before use.
 *
 *    Do not retain cli_CommandName as a C-string pointer: it references
 *    a length-prefixed BCPL string. Use the task's NUL-terminated name,
 *    with "ixnet" as a fallback, for SocketBase and usergroup context.
 *
 * Revision 1.1  2026/08/05  ChatGPT modifications (JJ)
 *
 *    Accept an AmiTCP-compatible bsdsocket.library without requiring
 *    AmiTCP usergroup.library, allowing Roadshow to use the normal
 *    IX_NETWORK_AMITCP socket path.
 *
 *    Validate both allocated signal bits and fully roll back partial
 *    AmiTCP or AS225 initialization without leaving stale base pointers.
 *
 *    Configure the socket errno pointer and event signals through the
 *    classic AmiTCP API, and use SocketBaseTagList() only with V4 bases
 *    for the optional log tag and h_errno pointer.
 *
 *    Store the program-name pointer used by later usergroup context
 *    updates and clear u_ixnet when initialization fails.
 */

#define _KERNEL
#include "ixnet.h"
#include "kprintf.h"

#include <amitcp/socketbasetags.h>
#include <amitcp/usergroup.h>
#include <exec/memory.h>
#include <string.h>
#include "ixprotos.h"

extern struct ixemul_base *ixemulbase;

struct ixnet_base *
ixnet_open (struct ixnet_base *ixbase)
{
    struct ixnet *p;
    struct Task *me;
    struct user *ix_u;
    struct ix_settings *settings; /* Do not call this *ix; ix is a macro. */
    int network_type;

    me = FindTask(0);
    if (!me)
      return 0;

    ix_u = getuser(me); /* already initialized by ixemul.library */
    if (!ix_u)
      return 0;

    /* This must be set here because ixnet_init.c runs in ramlib. */
    ixemulbase = ix_u->u_ixbase;

    if (ixnetbase->ixnet_lib.lib_Version != ixemulbase->ix_lib.lib_Version ||
        ixnetbase->ixnet_lib.lib_Revision != ixemulbase->ix_lib.lib_Revision)
      {
        ix_panic(
"ixnet.library has version %ld.%ld while ixemul.library has version %ld.%ld.\n"
"Both libraries should have the same version, therefore ixnet.library\n"
"won't be used.", ixnetbase->ixnet_lib.lib_Version,
                 ixnetbase->ixnet_lib.lib_Revision,
                 ixemulbase->ix_lib.lib_Version,
                 ixemulbase->ix_lib.lib_Revision);
        settings = ix_get_settings();
        settings->network_type = IX_NETWORK_NONE;
        ix_set_settings(settings);
        return 0;
      }

    p = (struct ixnet *)AllocMem(sizeof(struct ixnet),
                                 MEMF_PUBLIC | MEMF_CLEAR);
    if (!p)
      return 0;

    ix_u->u_ixnet = p;
    p->u_sigurg = -1;
    p->u_sigio = -1;
    p->u_networkprotocol = IX_NETWORK_NONE;

    settings = ix_get_settings();
    network_type = settings->network_type;

    switch (network_type)
      {
      case IX_NETWORK_AUTO:
      case IX_NETWORK_AMITCP:
        /*
         * Roadshow, AmiTCP and Miami expose the AmiTCP-compatible
         * bsdsocket.library interface.  usergroup.library is optional and
         * must not decide whether the socket backend itself is usable.
         */
        p->u_TCPBase = OpenLibrary("bsdsocket.library", 3);
        if (p->u_TCPBase)
          {
            /*
             * tc_Node.ln_Name is a normal NUL-terminated C string.
             * cli_CommandName is a BCPL string and must not be retained
             * as a C-string pointer for later SocketBase/usergroup calls.
             */
            p->u_progname = me->tc_Node.ln_Name;
            if (!p->u_progname)
              p->u_progname = "ixnet";

            p->u_sigurg = AllocSignal(-1);
            if (p->u_sigurg >= 0)
              p->u_sigio = AllocSignal(-1);

            if (p->u_sigurg >= 0 && p->u_sigio >= 0)
              {
                ULONG sigio_mask;
                ULONG sigurg_mask;

                sigio_mask = 1UL << p->u_sigio;
                sigurg_mask = 1UL << p->u_sigurg;

                TCP_SetErrnoPtr(ix_u->u_errno, sizeof(*ix_u->u_errno));
                TCP_SetSocketSignals(SIGBREAKF_CTRL_C,
                                     sigio_mask, sigurg_mask);

                /*
                 * SocketBaseTagList() is an AmiTCP V4 API.  Keep V3
                 * compatibility by using it only for optional V4 settings.
                 */
                if (((struct Library *)p->u_TCPBase)->lib_Version >= 4)
                  {
                    struct TagItem list[] = {
                      { SBTM_SETVAL(SBTC_LOGTAGPTR),
                        (ULONG)p->u_progname },
                      { SBTM_SETVAL(SBTC_HERRNOLONGPTR),
                        (ULONG)ix_u->u_h_errno },
                      { TAG_END }
                    };

                    (void)TCP_SocketBaseTagList(list);
                  }

                p->u_UserGroupBase =
                    OpenLibrary("AmiTCP:libs/usergroup.library", 1);

                if (p->u_UserGroupBase)
                  {
                    struct TagItem ug_list[] = {
                      { UGT_INTRMASK, SIGBREAKB_CTRL_C },
                      { UGT_ERRNOPTR(sizeof(int)), (ULONG)ix_u->u_errno },
                      { TAG_END }
                    };

                    (void)ug_SetupContextTagList(p->u_progname, ug_list);
                  }

                p->u_networkprotocol = IX_NETWORK_AMITCP;
                break;
              }

            if (p->u_sigio >= 0)
              {
                FreeSignal(p->u_sigio);
                p->u_sigio = -1;
              }
            if (p->u_sigurg >= 0)
              {
                FreeSignal(p->u_sigurg);
                p->u_sigurg = -1;
              }
            CloseLibrary(p->u_TCPBase);
            p->u_TCPBase = NULL;
          }

        if (network_type != IX_NETWORK_AUTO)
          break;

        /* Fall through to AS225 auto-detection. */

      case IX_NETWORK_AS225:
        p->u_SockBase = OpenLibrary("socket.library", 3);
        if (p->u_SockBase)
          {
            p->u_sigurg = AllocSignal(-1);
            if (p->u_sigurg >= 0)
              p->u_sigio = AllocSignal(-1);

            if (p->u_sigurg >= 0 && p->u_sigio >= 0)
              {
                p->u_networkprotocol = IX_NETWORK_AS225;
                break;
              }

            if (p->u_sigio >= 0)
              {
                FreeSignal(p->u_sigio);
                p->u_sigio = -1;
              }
            if (p->u_sigurg >= 0)
              {
                FreeSignal(p->u_sigurg);
                p->u_sigurg = -1;
              }
            CloseLibrary(p->u_SockBase);
            p->u_SockBase = NULL;
          }
        break;
      }

    if (p->u_networkprotocol == IX_NETWORK_NONE)
      {
        ix_u->u_ixnet = NULL;
        FreeMem(p, sizeof(struct ixnet));
        return 0;
      }

    return ixbase;
}
