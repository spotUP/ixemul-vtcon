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
 * Revision 1.4  2026/08/12  ChatGPT modifications (JJ)
 *
 *      Reject negative lengths and descriptors without FWRITE access.
 *      Treat a SetFileSize() result different from the requested length
 *      as an error, and stop before SetFileSize() if the preparatory
 *      Seek() fails.
 *
 * Revision 1.3  2026/08/01  ChatGPT modification (JJ)
 *      Added the FFS2-compatible SetFileSize() seek workaround from the
 *      later ixemul branch.  The current file position is saved by seeking
 *      to the beginning before SetFileSize(), then restored afterwards.
 *      If the file was shortened below the old position, the position is
 *      restored to the new end of file, matching the handler workaround.
 *
 * Revision 1.2  2026/06/07  Copilot/ChatGPT modifications (JJ)
 *      Normalized ftruncate() return value to POSIX semantics (0 on
 *      success, -1 on error), added early fd bounds check to avoid
 *      out-of-range indexing of u.u_ofile[], and simplified the open
 *      file test to 'if (f)' since fd is now validated.
 *
 *
 *  ftruncate.c,v 1.1.1.1 1994/04/04 04:30:18 amiga Exp
 *
 *  ftruncate.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:18  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#ifndef ACTION_SET_FILE_SIZE
#define ACTION_SET_FILE_SIZE 1022
#endif

int
ftruncate (int fd, off_t len)
{
  usetup;
  struct file *f;
  int err, res;
  int omask;

  /* validate fd before indexing u.u_ofile[] */
  if (fd < 0 || fd >= NOFILE)
    {
      errno = EBADF;
      return -1;
    }

  if (len < 0)
    {
      errno = EINVAL;
      return -1;
    }

  f = u.u_ofile[fd];

  /* if this is an open fd */
  if (f)
    {
      if (f->f_type == DTYPE_FILE)
	{
          BPTR fh;
          LONG old_pos;

          if ((f->f_flags & FWRITE) == 0)
            {
              errno = EBADF;
              KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
              return -1;
            }

          if (HANDLER_NIL (f))
	    {
	      errno = EINVAL;
	      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	      return -1;
	    }

	  err = 0;
	  omask = syscall (SYS_sigsetmask, ~0);
	  __get_file (f);

          /*
           * Some filesystems, notably FFS2 variants, require the file
           * position to be at the beginning for SetFileSize() to behave
           * correctly.  AmigaDOS Seek() returns the previous position.
           */
          fh = CTOBPTR(f->f_fh);
          old_pos = Seek(fh, 0, OFFSET_BEGINNING);
          if (old_pos == -1)
            {
              err = __ioerr_to_errno (IoErr ());
              if (err == 0)
                err = EIO;
              res = -1;
            }
          else
            {
	      res = SetFileSize(fh, len, OFFSET_BEGINNING);

	      if (res == -1)
	        {
	          err = __ioerr_to_errno (IoErr ());
	        }
              else
                {
                  if ((LONG)res < old_pos)
                    {
                      /* The old position no longer exists after truncation. */
                      old_pos = (LONG)res;
                    }

                  if ((off_t)res != len)
                    {
                      /* The handler did not reach the requested file size. */
                      err = EIO;
                      res = -1;
                    }
                }

              /*
               * Restore the previous position even when SetFileSize() failed,
               * since the preparatory Seek() may already have moved it.
               * Keep the historical workaround behavior if restoration fails.
               */
              Seek(fh, old_pos, OFFSET_BEGINNING);

	      if (res != -1)
	        {
	          err = 0;
	          res = 0;   /* POSIX: ftruncate() returns 0 on success */
	        }
            }
	  __release_file (f);
	  syscall (SYS_sigsetmask, omask);
	  errno = err;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return res;
	}
      else
	{
	  errno = ESPIPE;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return -1;
	}
    }

  errno = EBADF;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
