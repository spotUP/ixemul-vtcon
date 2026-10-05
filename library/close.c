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
 *  close.c,v 1.1.1.1 1994/04/04 04:30:16 amiga Exp
 *
 *  close.c,v
 * 
 * Revision 1.4  2026/07/09  ChatGPT modifications (JJ)
 *
 *  Restore the historical ixemul close-handler ownership model and
 *  supersede the Revision 1.3 close()/f_count ownership change.
 *
 *  - close() detaches the descriptor from u.u_ofile[] but does not
 *    decrement f_count directly.
 *  - f_close handlers continue to own f_count decrement and final
 *    resource cleanup.
 *  - __close_file_ref() is kept as a helper, but it preserves the
 *    old model: it must be called while f_count still includes the
 *    descriptor reference being closed.
 *  - __flock_close() is called only when f_count == 1, before the
 *    close handler runs, so flock state is released only on final close.
 *
 *  This fixes the file-table-full regression caused by pre-decrementing
 *  f_count in generic close() before calling the existing f_close handler.
 * 
 * Revision 1.3  2026/07/01  ChatGPT modifications (JJ)
 * 
 *  - Call __flock_close() from __close_file_ref() before the file close
 *    handler runs, so advisory locks are released on final close.
 *  - Added __close_file_ref() helper.
 *  - Changed close() to drop one descriptor reference and only call
 *    the file close handler when the final struct file reference is gone.
 * 
 * Revision 1.2  2026/06/13  ChatGPT modifications  (JJ)
 *  Validate the descriptor before indexing u.u_ofile[] in close().
 *  This preserves the original close-handler ownership model while
 *  avoiding out-of-range descriptor table access on invalid fd values.
 *
 *  close.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:16  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

/*
 * Close one detached descriptor reference.
 *
 * IMPORTANT:
 * This helper preserves the historical ixemul ownership model.
 * The caller must already have removed the descriptor from u.u_ofile[],
 * but must NOT have decremented f_count.
 *
 * The f_close handler owns f_count decrement and final resource cleanup.
 * We only use f_count here to decide whether this descriptor reference is
 * the final reference, so that flock state can be released before the
 * handler performs the final close.
 */
int
__close_file_ref (struct file *f)
{
  usetup;

  if (!f)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  /*
   * f_count still includes the descriptor reference being closed.
   * Therefore f_count == 1 means the close handler is about to perform
   * the final close.
   */
  if (f->f_count == 1)
    __flock_close(f);

  if (f->f_close)
    return (*f->f_close)(f);

  errno = EIO;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
close (int fd)
{
  usetup;
  struct file *f;

  /*
   * Validate and detach the descriptor under ix_lock_base().
   * Do not touch f_count here.  The close handler owns that.
   */
  ix_lock_base ();

  if (fd < 0 || fd >= NOFILE || !(f = u.u_ofile[fd]))
    {
      ix_unlock_base ();
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  u.u_ofile[fd] = 0;

  ix_unlock_base ();

  return __close_file_ref(f);
}
