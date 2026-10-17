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
 *  mkdir.c,v 1.1.1.1 1994/04/04 04:30:30 amiga Exp
 *
 *  mkdir.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:30  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

static int
__mkdir_func (struct lockinfo *info, void *dummy, int *error)
{
  usetup;
  struct StandardPacket *sp = &info->sp;
  int before;

  /* the new name must not be one the handler shortens (RAM: keeps 30
     characters; UP-Term) */
  before = __name_truncated (sp, info->handler, info->parent_lock,
                             info->bstr);
  if (before == 1)
    goto too_long;

  sp->sp_Pkt.dp_Port = __srwport;
  sp->sp_Pkt.dp_Type = ACTION_CREATE_DIR;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result == 0;

  if (!*error && before == -1
      && __name_truncated (sp, info->handler, info->parent_lock,
                           info->bstr) == 1)
    {
      UnLock (info->result);
      __name_delete (sp, info->handler, info->parent_lock, info->bstr);
      goto too_long;
    }
    
  /* continue if we failed because of symlink - reference */
  return 1;

too_long:
  sp->sp_Pkt.dp_Res1 = 0;
  sp->sp_Pkt.dp_Res2 = IX_ERROR_NAME_TOO_LONG;
  info->result = 0;
  *error = 1;
  return 1;
}


int
mkdir (const char *path, mode_t perm)
{
  BPTR lock;
  int result = -1;
  int omask;
  usetup;
 
  /* protect the obtained lock, we have to free it later */
  omask = syscall (SYS_sigsetmask, ~0);  
  lock = __plock (path, __mkdir_func, 0);

  if (lock)
    {
      __unlock (lock);
      result = syscall (SYS_chmod, path, (int)(perm & ~u.u_cmask));

      if (!muBase)
        {
          uid_t uid = geteuid();
          gid_t gid = getegid();

          if (uid != (uid_t)(-2) || gid != (gid_t)(-2))
            syscall (SYS_chown, path, uid, gid);
        }
    }
  syscall (SYS_sigsetmask, omask);

  errno = __ioerr_to_errno (IoErr ());
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return result;
}
