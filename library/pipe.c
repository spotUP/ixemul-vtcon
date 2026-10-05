/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 * Revision 1.6  2026/08/04  ChatGPT modifications (JJ)
 *
 *  Drop the pipe owner reference inside __pclose() while its existing
 *  Forbid()/Permit() section already protects sock_stream::refs.  Final
 *  memory release remains outside Forbid(), and the separate owner-drop
 *  helper remains available for pipe() allocation rollback.
 *
 * Revision 1.5  2026-08-01  ChatGPT modifications  (JJ)
 *
 *  Stage 2 pipe lifetime and allocation fixes.  sock_stream now retains one
 *  owner reference until both endpoints are closed, while active operations
 *  hold temporary references in unp.c.  This prevents close from freeing a
 *  stream still used as an I/O object or ix_sleep() wait channel.
 *
 *  Also preserve falloc() error codes, return ENOMEM for stream allocation
 *  failure, validate pv, roll back the first descriptor safely if the second
 *  allocation fails, and signal a waiting task with its own pipe signal bit.
 *
 *  $Id: pipe.c,v 1.4 1994/06/19 15:14:19 rluebbert Exp $
 *
 *  $Log: pipe.c,v $
 *  Revision 1.4  1994/06/19  15:14:19  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/04  19:21:08  mwild
 *  (finally..) fix the bug which could cause pipe readers/writers to deadlock
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/ioctl.h>
#include <string.h>
#include "select.h"
#include "unp.h"

/* information for the temporary implementation of pipes.
   PIPE: has the big disadvantage that it blocks in the most unpleasent
   situations, and doesn't send SIGPIPE to processes that write on
   readerless pipes. Unacceptable for this library ;-)) */

static int __pclose(struct file *f);

static void
pipe_drop_stream_owner(struct sock_stream *ss)
{
  int free_stream;

  if (ss == NULL)
    return;

  free_stream = 0;
  Forbid();
  if (ss->refs > 0)
    {
      ss->refs--;
      if (ss->refs == 0)
        free_stream = 1;
    }
  Permit();

  if (free_stream)
    kfree(ss);
}

static void
pipe_rollback_file(int fd, struct file *fp)
{
  usetup;

  ix_lock_base();
  if (fd >= 0 && fd < NOFILE && u.u_ofile[fd] == fp)
    {
      u.u_ofile[fd] = NULL;
      u.u_pofile[fd] = 0;
      fp->f_count = 0;
      fp->f_ss = NULL;

    }
  ix_unlock_base();
}

int
pipe(int pv[2])
{
  struct file *f1;
  struct file *f2;
  struct sock_stream *ss;
  int fd1;
  int fd2;
  int err;
  int omask;
  usetup;

  if (pv == NULL)
    errno_return(EFAULT, -1);

  pv[0] = -1;
  pv[1] = -1;
  f1 = NULL;
  f2 = NULL;
  ss = NULL;
  fd1 = -1;
  fd2 = -1;
  err = 0;

  omask = syscall(SYS_sigsetmask, ~0);

  ss = init_stream();
  if (ss == NULL)
    {
      err = ENOMEM;
      goto error;
    }

  err = falloc(&f1, &fd1);
  if (err != 0)
    goto error;

  err = falloc(&f2, &fd2);
  if (err != 0)
    {
      pipe_rollback_file(fd1, f1);
      f1 = NULL;
      fd1 = -1;
      goto error;
    }

  f1->f_ss = ss;
  f1->f_stb.st_mode = 0666 | S_IFCHR;
  f1->f_stb.st_size = UNIX_SOCKET_SIZE;
  f1->f_stb.st_blksize = 512;
  f1->f_flags = FREAD;
  f1->f_type = DTYPE_PIPE;
  f1->f_read = stream_read;
  f1->f_write = 0;
  f1->f_ioctl = unp_ioctl;
  f1->f_close = __pclose;
  f1->f_select = unp_select;

  f2->f_ss = ss;
  f2->f_stb.st_mode = 0666 | S_IFCHR;
  f2->f_stb.st_size = UNIX_SOCKET_SIZE;
  f2->f_stb.st_blksize = 512;
  f2->f_flags = FWRITE;
  f2->f_type = DTYPE_PIPE;
  f2->f_read = 0;
  f2->f_write = stream_write;
  f2->f_ioctl = unp_ioctl;
  f2->f_close = __pclose;
  f2->f_select = unp_select;

  pv[0] = fd1;
  pv[1] = fd2;
  syscall(SYS_sigsetmask, omask);
  return 0;

error:
  if (ss != NULL)
    pipe_drop_stream_owner(ss);

  syscall(SYS_sigsetmask, omask);
  errno = err;
  KPRINTF_DISABLED(("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

static int
__pclose(struct file *f)
{
  struct sock_stream *ss;
  int free_stream;
  usetup;

  ss = f->f_ss;
  if (ss == NULL)
    return 0;

  free_stream = 0;

  Forbid();

  f->f_count--;
  if (f->f_count == 0)
    {
      f->f_ss = NULL;

      if (f->f_read != NULL)
        ss->flags |= UNF_NO_READER;
      else
        ss->flags |= UNF_NO_WRITER;

      if (ss->task != NULL)
        Signal(ss->task,
               1UL << getuser(ss->task)->u_pipe_sig);
      ss->task = NULL;
      ix_wakeup((u_int)ss);

      if ((ss->flags & (UNF_NO_READER | UNF_NO_WRITER)) ==
          (UNF_NO_READER | UNF_NO_WRITER))
        {
          /*
           * Drop the owning pipe reference while the current critical
           * section already protects ss->refs.  Active operations retain
           * their temporary references and perform the eventual release.
           */
          if (ss->refs > 0)
            {
              ss->refs--;
              if (ss->refs == 0)
                free_stream = 1;
            }
        }
    }

  Permit();

  if (free_stream)
    kfree(ss);

  return 0;
}
