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
 * Revision 1.6  2026/08/29  ChatGPT modifications (JJ)
 *
 *   Release process-private ACTION_WAIT_CHAR state when the current process
 *   closes its last descriptor alias for a shared struct file.
 *   Preserve the historical PTY ACTION_STACK wakeup on global final close.
 *   Leave f_count ownership and final file-resource cleanup unchanged.
 *
 * Revision 1.5  2026/06/25  ChatGPT modifications  (JJ)
 *
 * Centralized PTY pathname detection:
 *
 *   - Added is_pty_name() helper for guarded /fifo/pty* pathname checks.
 *   - Replaced duplicated PTY prefix/length checks with the helper.
 *   - Avoids strlen() on normal close paths by testing the PTY prefix first.
 *   - Ensures consistent guarded PTY name handling in both:
 *       * select()/ACTION_STACK cleanup path
 *       * PTY state update on final close
 *
 * No change to PTY lifecycle semantics or normal close behavior; improves
 * robustness, readability and cost of PTY name matching.
 *
 * Revision 1.4  2026/06/07  Copilot/ChatGPT modifications  (JJ)
 *
 * Hardened __close() against missing or too short f_name.
 *
 *   - Added NULL and length guards before PTY name matching and
 *     PTY index extraction.
 *   - Guarded deferred unlink/chmod/chown/utimes calls so pathname
 *     operations are skipped when f_name is unavailable.
 *   - Added f_fh guard before ACTION_STACK PTY select cleanup.
 *
 * Normal behavior is unchanged when f_name is valid.
 *
 * Revision 1.3  2026/05/31  Copilot modifications  (JJ)
 *
 * Updated filename storage management.
 *
 *   - Added support for per-file inline filename storage.
 *   - Added struct file::f_name_buf and f_name_inline handling.
 *   - Short pathnames are stored inline in struct file.
 *   - Longer pathnames continue to use dynamic allocation.
 *   - Added filename ownership cleanup during final close.
 *   - Added optional DEBUG logging for filename release.
 *
 * No changes to:
 *   - PTY lifecycle semantics
 *   - f_count logic
 *   - FUNLINK behavior
 *   - chmod/chown/utimes deferred update behavior
 *   - TTY dataplane behavior
 * 
 *  *  $Id: __close.c,v 1.2 1994/06/19 15:18:48 rluebbert Exp $
 *
 *  $Log: __close.c,v $
 *  Revision 1.2  1994/06/19  15:18:48  rluebbert
 *  *** empty log message ***
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "select.h"
#include <string.h>

#ifndef ACTION_STACK
#define ACTION_STACK 2002L
#endif

/* --- Close path ---------------------------------------------------------- */

static int
is_pty_name(const char *name)
{
  if (!name)
    return 0;

  if (strncmp(name, "/fifo/pty", 9) != 0)
    return 0;

  return strlen(name) >= 18;
}

/*
 * __close() is responsible for:
 *   - Decrementing f_count and only closing when it reaches zero
 *   - Cleaning up select()/FIFO state for PTYs
 *   - Updating filesystem metadata if marked dirty
 *   - Handling deferred unlink (FUNLINK)
 *   - Releasing filename storage correctly
 */

int
__close(struct file *f)
{
  usetup;
  struct StandardPacket *select_sp;
  int local_ref;
  int i;

  /*
   * close()/dup2() has already detached or replaced the descriptor that
   * caused this call.  A process-private select packet belongs to this task,
   * not to the globally shared struct file, so release it as soon as this
   * process no longer has another descriptor alias for f.
   */
  select_sp = __fselect_get_packet(f);
  if (select_sp)
    {
      local_ref = 0;
      for (i = 0; i < NOFILE; i++)
        {
          if (u.u_ofile[i] == f)
            {
              local_ref = 1;
              break;
            }
        }

      if (!local_ref)
        {
          /*
           * Preserve the historical PTY wakeup only for the global final
           * close.  On a non-final local close, let ACTION_WAIT_CHAR finish
           * normally rather than injecting input into a still-shared PTY.
           */
          if (f->f_count == 1 && f->f_fh && is_pty_name(f->f_name) &&
              select_sp->sp_Pkt.dp_Port)
            {
              SendPacket3(f, __srwport, ACTION_STACK,
                          f->f_fh->fh_Arg1, (int)"\n", 1);
              __wait_sync_packet(&f->f_sp);
            }

          __fselect_release_file(f);
        }
    }

  ix_lock_base();

  f->f_count--;

  KPRINTF(("__close: closing file $%lx, f_count now %ld.\n",
           f, f->f_count));

  if (f->f_count > 0)
    goto unlock;

  if (!(f->f_flags & FEXTOPEN))
    {
      __Close(CTOBPTR(f->f_fh));

      if (is_pty_name(f->f_name))
        {
          int i = (f->f_name[9] - 'p') * 16 +
                  f->f_name[10] -
                  (f->f_name[10] >= 'a' ? 'a' - 10 : '0');

          char mask = (f->f_name[17] == 'm'
                       ? IX_PTY_MASTER
                       : IX_PTY_SLAVE);

          ix.ix_ptys[i] &= ~(mask & IX_PTY_OPEN);
          ix.ix_ptys[i] |=  (mask & IX_PTY_CLOSE);

          if (!(ix.ix_ptys[i] & IX_PTY_OPEN))
            ix.ix_ptys[i] = 0;
        }
    }

  if (f->f_flags & FUNLINK)
    {
      if (f->f_name)
        syscall(SYS_unlink, f->f_name);
      else
        KPRINTF(("__close: FUNLINK set but f->f_name == NULL, skipping unlink\n"));
    }
  else
    {
      if ((f->f_stb_dirty & FSDF_MODE) && f->f_name)
        syscall(SYS_chmod, f->f_name, f->f_stb.st_mode);

      if ((f->f_stb_dirty & FSDF_OWNER) && f->f_name)
        syscall(SYS_chown, f->f_name,
                f->f_stb.st_uid, f->f_stb.st_gid);

      if (f->f_write &&
          (f->f_stb_dirty & FSDF_UTIME) &&
          f->f_name)
        syscall(SYS_utimes, f->f_name, NULL);
    }

  if (!(f->f_flags & FEXTNAME) && f->f_name)
    {
#ifdef DEBUG
      KPRINTF(("f->f_name(%s) ", f->f_name));
#endif

      if (!f->f_name_inline)
        kfree(f->f_name);

      f->f_name = 0;
      f->f_name_inline = 0;
    }

unlock:
  ix_unlock_base();
  return 0;
}
