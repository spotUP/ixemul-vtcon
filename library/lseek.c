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
 *  lseek.c,v 1.1.1.1 1994/04/04 04:30:29 amiga Exp
 * 
 *  Revision 1.1.1.4  2026/09/13  ChatGPT modifications (JJ)
 *
 *    Fix DTYPE_MEM lseek() return value to report the resulting file
 *    position instead of the previous position.
 *
 *  Revision 1.1.1.3  2026/06/23  ChatGPT modifications (JJ)
 *
 *    Validate DTYPE_FILE whence values before HANDLER_NIL handling.
 *    Added a fast path for lseek(fd, 0, SEEK_CUR), avoiding a redundant
 *    AmigaOS Seek() call when only the current file position is requested.
 *
 *  File extension semantics and EOF handling are unchanged.
 *
 *  Revision 1.1.1.2  2026/06/17  ChatGPT modifications (JJ)
 *
 *  Harden lseek() argument validation.
 *
 *  - Validate fd before indexing u.u_ofile[] to avoid out-of-bounds
 *    access for negative or too-large file descriptors.
 *  - Reject invalid whence values with EINVAL for both regular files
 *    and memory-backed file descriptors.
 *  - Initialize internal error/result state defensively.
 *  - Guard seek-position arithmetic against signed overflow.
 *  - Treat zero-length Write() progress while extending files as EIO to
 *    avoid an infinite loop.
 *  - Set extension errno only from failing Seek() calls; clear it after
 *    successful final positioning.
 *
 *  No functional change for valid file descriptors and valid whence values.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <limits.h>
#include <string.h>

/* errno for a failed Seek(): a stream that cannot seek (FH_SEEK in
   packets.h) is ESPIPE on Unix, not the ENODEV ERROR_ACTION_NOT_KNOWN maps to */
static inline int
__seek_errno (void)
{
  long e = IoErr ();
  return e == ERROR_ACTION_NOT_KNOWN ? ESPIPE : __ioerr_to_errno (e);
}

static inline int
__extend_file (struct file *f, int add_to_eof, int *err)
{
  int res = 0;
  int buf_size, written;
  char *buf;

  buf_size = add_to_eof > 32*1024 ? 32*1024 : add_to_eof;
  while (!(buf = (char *) kmalloc(buf_size)) && buf_size)
    buf_size >>= 1;

  if (buf)
    {
      bzero(buf, buf_size);

      for (written = 0; written < add_to_eof; )
        {
          res = Write(CTOBPTR(f->f_fh), buf, buf_size);
          if (res <= 0)
            {
              if (res < 0)
                *err = __ioerr_to_errno(IoErr());
              else
                *err = EIO;
              res = -1;
              break;
            }
          written += res;
          buf_size = add_to_eof - written > buf_size ?
                     buf_size : add_to_eof - written;
        }
      kfree(buf);
    }
  else
    {
      *err = ENOMEM;
      res = -1;
    }

  if (res >= 0)
    {
      res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
      if (res == -1)
        *err = __ioerr_to_errno(IoErr());
      else
        {
          res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
          if (res == -1)
            *err = __ioerr_to_errno(IoErr());
          else
            *err = 0;
        }
    }

  return res;
}

off_t
lseek (int fd, off_t off, int dir)
{
  usetup;
  struct file *f;
  int omask;
  int err = 0;
  int res = -1;
  int previous_pos = 0;
  int shouldbe_pos;

  if (fd >= 0 && fd < NOFILE && (f = u.u_ofile[fd]))
    {
      if (f->f_type == DTYPE_FILE)
        {
          /* NEW: validate whence before HANDLER_NIL */
          if (dir != SEEK_SET && dir != SEEK_CUR && dir != SEEK_END)
            {
              errno = EINVAL;
              KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
              return -1;
            }

          if (HANDLER_NIL(f))
            {
              if (off == 0)
                return 0;
              errno = ESPIPE;
              KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
              return -1;
            }

          omask = syscall(SYS_sigsetmask, ~0);
          __get_file(f);

          /* NEW: fast path for SEEK_CUR + off == 0 */
          if (dir == SEEK_CUR && off == 0)
            {
              res = FH_SEEK(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
              if (res == -1)
                err = __seek_errno();
              else
                err = 0;
              goto done_file;
            }

          switch (dir)
            {
            case SEEK_SET:
              previous_pos = 0;
              break;

            case SEEK_CUR:
              previous_pos = FH_SEEK(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
              if (previous_pos == -1)
                err = __seek_errno();
              break;

            case SEEK_END:
              Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
              previous_pos = FH_SEEK(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
              if (previous_pos == -1)
                err = __seek_errno();
              break;
            }

          if (previous_pos < 0)
            res = -1;
          else if (off < 0 && off < -(off_t)previous_pos)
            {
              err = EINVAL;
              res = -1;
            }
          else if (off > 0 && off > (off_t)INT_MAX - previous_pos)
            {
#ifdef EOVERFLOW
              err = EOVERFLOW;
#else
              err = EINVAL;
#endif
              res = -1;
            }
          else
            {
              shouldbe_pos = (int)(previous_pos + off);

              res = FH_SEEK(CTOBPTR(f->f_fh), off, dir - 1);
              if (res == -1 && IoErr() == ERROR_SEEK_ERROR)
                res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);

              if (res == -1)
                err = __seek_errno();
              else
                {
                  err = 0;

                  if (previous_pos != shouldbe_pos)
                    {
                      res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
                      if (res == -1)
                        err = __seek_errno();

                      if (res >= 0 && res < shouldbe_pos &&
                          (f->f_flags & FWRITE))
                        res = __extend_file(f, shouldbe_pos - res, &err);
                    }
                  else
                    res = shouldbe_pos;
                }
            }

done_file:
          __release_file(f);
          syscall(SYS_sigsetmask, omask);
          errno = err;
          KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
          return res;
        }

      else if (f->f_type == DTYPE_MEM)
        {
          int real_off;
          int old_off;
          int mem_size;

          omask = syscall(SYS_sigsetmask, ~0);
          __get_file(f);
          old_off = f->f_mf.mf_offset;
          mem_size = f->f_stb.st_size;

          switch (dir)
            {
            case L_SET:
              if (off < 0)
                real_off = 0;
              else if (off > (off_t)mem_size)
                real_off = mem_size;
              else
                real_off = (int)off;
              break;

            case L_INCR:
              if (off < 0 && off < -(off_t)old_off)
                real_off = 0;
              else if (off > 0 && off > (off_t)mem_size - old_off)
                real_off = mem_size;
              else
                real_off = (int)(old_off + off);
              break;

            case L_XTND:
              if (off < 0 && off < -(off_t)mem_size)
                real_off = 0;
              else if (off > 0)
                real_off = mem_size;
              else
                real_off = (int)(mem_size + off);
              break;

            default:
              __release_file(f);
              syscall(SYS_sigsetmask, omask);
              errno = EINVAL;
              KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
              return -1;
            }

          if (real_off < 0)
            real_off = 0;
          else if (real_off > f->f_stb.st_size)
            real_off = f->f_stb.st_size;

          f->f_mf.mf_offset = real_off;
          __release_file(f);
          syscall(SYS_sigsetmask, omask);
          return real_off;
        }

      else
        {
          errno = ESPIPE;
          KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
        }
    }
  else
    {
      errno = EBADF;
      KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
    }

  return -1;
}
