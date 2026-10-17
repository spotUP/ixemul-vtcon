/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
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
 * 
 *  Revision 1.2  2026/06/08  Copilot/ChatGPT modification  (JJ)
 *  Changed __make_link() to take const char * instead of char *.
 *  No functional change; __plock() copies the string internally.
 *
 *
 *  __make_link.c,v 1.1.1.1 1994/04/04 04:30:10 amiga Exp
 *
 *  __make_link.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:10  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"

#ifndef ACTION_MAKE_LINK
#define ACTION_MAKE_LINK 1021
#endif

struct link_vec {
  BPTR target;
  int  link_mode;
};

static int
__link_func (struct lockinfo *info, struct link_vec *lv, int *error)
{
  usetup;
  struct StandardPacket *sp = &info->sp;
  int before;

  /* the link's name must not be one the handler shortens (RAM: keeps 30
     characters; UP-Term) */
  before = __name_truncated (sp, info->handler, info->parent_lock,
                             info->bstr);
  if (before == 1)
    goto too_long;

  sp->sp_Pkt.dp_Port = __srwport;
  sp->sp_Pkt.dp_Type = ACTION_MAKE_LINK;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;
  sp->sp_Pkt.dp_Arg3 = lv->target;
  sp->sp_Pkt.dp_Arg4 = lv->link_mode;
  
  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;
  
  *error = info->result != -1;

  /* a soft link cannot be located (ERROR_IS_SOFT_LINK): -1, kept */
  if (!*error && before == -1
      && __name_truncated (sp, info->handler, info->parent_lock,
                           info->bstr) == 1)
    {
      __name_delete (sp, info->handler, info->parent_lock, info->bstr);
      goto too_long;
    }
  
  /* shouldn't be possible to get the ERR_SOFT_LINK here.. */
  return 0;

too_long:
  sp->sp_Pkt.dp_Res1 = 0;
  sp->sp_Pkt.dp_Res2 = IX_ERROR_NAME_TOO_LONG;
  info->result = 0;
  *error = 1;
  return 0;
}

int 
__make_link (const char *path, BPTR targ, int mode)
{
  struct link_vec lv;
  
  lv.target = targ;
  lv.link_mode = mode;
  
  return __plock (path, __link_func, &lv);
}
