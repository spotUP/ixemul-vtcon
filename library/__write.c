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
 * Revision 1.3  2026/08/05  ChatGPT modifications (JJ)
 *
 *  Abort writes to regular files opened with O_APPEND when the required
 *  seek to EOF fails. Preserve the historical behavior for non-regular
 *  handlers, where a file position may be irrelevant or unsupported.
 *
 *
 * Revision 1.2  2026/05/31  ChatGPT modifications (JJ)
 *
 *  Refactored the synchronous and interactive write paths while preserving
 *  their established external behavior.
 *
 *  - Extracted interactive chunk calculation and NL-to-CRLF handling into
 *    dedicated helpers.
 *  - Use the precomputed IXTTY_MAP_NL_TO_CRLF flag so termios and sgtty
 *    output mappings follow the state established by __tioctl.c.
 *  - Added an early return for non-positive internal write lengths.
 *  - Made write-buffer parameters const-correct.
 *  - Added unlikely-branch hints to error, append and short-write paths.
 *  - Preserve the caller's errno after successful writes and translate
 *    IoErr() when Write() fails.
 *  - Restricted diagnostic output to debug builds.
 *  - Detect and report a failed append seek without yet changing the
 *    historical write-after-seek-failure behavior.
 *
 *  __swrite.c,v 1.1.1.1 1994/04/04 04:30:12 amiga Exp
 *
 *  __swrite.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:12  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

/* --- Includes ------------------------------------------------------------ */

#define _KERNEL
#include "ixemul.h"
#include "__vtcon.h"
#include "kprintf.h"

/* --- Internal helpers ---------------------------------------------------- */

/* Forward declaration so helpers can call the core writer. */
static int __do_sync_write(struct file *f, const char *buf, int len);

/*
 * Compute the number of bytes to write before encountering a newline or
 * reaching the 256-byte interactive chunk limit.
 *
 * This is a direct extraction of the original inline logic.
 * Semantics are unchanged.
 */
static inline int
tty_write_chunk_len(const char *p, int remaining)
{
  int bytes = 0;

  while (bytes < remaining &&
         bytes < 256 &&
         p[bytes] != '\n')
    {
      bytes++;
    }

  return bytes;
}

/*
 * NL -> CRLF translation helper.
 *
 * When RAW, OPOST, and ONLCR are all set, a newline must be translated
 * to CRLF. This helper performs the write and updates the caller's
 * consumed-byte count.
 *
 * Returns:
 *   -1  on write error
 *    0  if nothing was consumed
 *    1  if one byte of caller input was consumed
 *
 * Semantics are identical to the original inline logic.
 */
static inline int
tty_write_nlcr(struct file *f, int *res)
{
  int tmp;

  tmp = __do_sync_write(f, "\r\n", 2);
  if (tmp == -1)
    return -1;

  /*
   * tmp == 2 -> CRLF fully written -> consume 1 byte of caller input
   * tmp != 2 -> partial write -> consume 0 bytes
   */
  tmp = (tmp == 2 ? 1 : 0);
  *res += tmp;

  return tmp;
}

static int __do_sync_write(struct file *f, const char *buf, int len)
{
  usetup;

  /* --- Local state ------------------------------------------------------- */
  /*
   * saved_errno:
   *   We preserve the caller's errno unless Write() fails.
   *
   * omask:
   *   Signals are blocked during the write to avoid inconsistent state
   *   if a signal arrives while interacting with AmigaDOS.
   *
   * res:
   *   Number of bytes written or -1 on error.
   */

  int saved_errno = errno;
  int res = 0;
  int omask;

  /* Early return for zero-length writes: no I/O, no locks, no syscalls */
  if (__builtin_expect((len <= 0), 0))
    return 0;

  /* --- Enter critical section: block signals ----------------------------- */
  /*
   * We block all signals during the write operation to ensure that the
   * AmigaDOS Write() call cannot be interrupted in an inconsistent state.
   */

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);

  if (len > 0)
    {
      /* full append-mode means, before each write do an explicit
       * seek to eof */

      /* --- Append mode handling ------------------------------------------ */
      /*
       * When FAPPEND is set, POSIX requires that each write is appended
       * atomically. We emulate this by seeking to EOF before every write.
       *
       * This is required for correctness but may be slow on some handlers.
       * The behavior must remain unchanged for compatibility.
       */

      if (__builtin_expect((f->f_flags & FAPPEND), 0))
        {
          if (Seek(CTOBPTR(f->f_fh), 0, OFFSET_END) == -1)
            {
              /*
               * A regular-file append must not continue at an unknown
               * position.  Non-regular handlers retain the historical
               * behavior because their file position may be irrelevant.
               */
              if (S_ISREG(f->f_stb.st_mode))
                {
                  int append_errno;

                  append_errno = __ioerr_to_errno(IoErr());
                  __release_file(f);
                  syscall(SYS_sigsetmask, omask);
                  errno = append_errno;
                  return -1;
                }

              KPRINTF(("append seek failed: IoErr=%ld\n", IoErr()));
            }
        }

      res = Write(CTOBPTR(f->f_fh), (APTR)buf, len);

      /* --- Error handling ------------------------------------------------ */
      /*
       * If Write() fails, convert IoErr() to errno.
       * Otherwise restore the caller's errno.
       */

      if (__builtin_expect((res == -1), 0))
        errno = __ioerr_to_errno(IoErr());
      else
        errno = saved_errno;
    }

  __release_file (f);

  /* --- Restore signal mask ---------------------------------------------- */
  syscall (SYS_sigsetmask, omask);

  /* Debug trace (enabled only in debug builds) */
#ifdef DEBUG
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
#endif
  return res;
}

int __write(struct file *f, const char *buf, int len)
{
  char *p;
  int l, bytes, res = 0, tmp;

  /* --- Early exit for NIL handler --------------------------------------- */
  /*
   * Writing to NIL always succeeds and discards all data.
   * Returning len matches AmigaDOS semantics.
   */

  if (HANDLER_NIL(f))
    return len;

  /* --- Non-interactive path --------------------------------------------- */
  /*
   * If the file is not interactive, we bypass all TTY logic and perform
   * a synchronous write of the entire buffer.
   */

  if (!IsInteractive(CTOBPTR(f->f_fh))) /* if not interactive */
    return __do_sync_write(f, buf, len);

  /* a vtcon console does OPOST itself: the buffer goes in one packet
     (line by line, a full-screen redraw was a packet per line). A
     background job writing it with TOSTOP set stops first (SIGTTOU). */
  if (__vtcon(f))
    {
      __vtcon_bg(f, SIGTTOU);
      return __do_sync_write(f, buf, len);
    }

  /* ----------------------------------------------------------------------- */
  /*  Interactive write path (line-buffered, NL-CRLF, chunked writes)        */
  /* ----------------------------------------------------------------------- */

  /* write the buffer line by line, otherwise the user isn't able to stop
     the console output until the whole buffer was flushed to the console */
  /* also check the f->f_ttyflags */

  /* --- Interactive path: line-buffered writes --------------------------- */
  /*
   * For interactive consoles, we must write output in small chunks so the
   * user can interrupt output (Ctrl-C) between lines.
   *
   * Additionally, NL -> CRLF translation is applied when:
   *   RAW + OPOST + ONLCR are all set.
   *
   * This behavior is required for compatibility with shells and editors.
   */

  /* --- Interactive path -------------------------------------------------- */
  /*  Loop over caller buffer, processing newline and chunk boundaries       */
  /* ----------------------------------------------------------------------- */

  for (p = (char *)buf, l = 0; l < len; p += bytes, l += bytes)
    {
      if (__builtin_expect((p[0] == '\n'), 0))
        {
          bytes = 1;

          /* --- NL -> CRLF translation ------------------------------------- */
          /* ---------------------------------------------------------------- */
          /*  RAW + OPOST + ONLCR -> write CRLF and consume one caller byte   */
          /* (now driven by IXTTY_MAP_NL_TO_CRLF)                             */

          /* IXTTY_MAP_NL_TO_CRLF is derived by __tioctl.c from:
           *   termios: RAW + OPOST + ONLCR
           *   sgtty:   RAW/CBREAK + CRMOD
           *
           * When set, write "\r\n" and count one caller byte as consumed
           * only if both bytes were written successfully.
           */

          /*
           * When RAW, OPOST, and ONLCR are all set, a newline must be
           * translated to CRLF. We write "\r\n" and count only one byte
           * as consumed from the caller's perspective.
           *
           * This logic mirrors BSD TTY behavior and must remain unchanged.
           */

          if (f->f_ttyflags & IXTTY_MAP_NL_TO_CRLF)
            {
              tmp = tty_write_nlcr(f, &res);
              if (__builtin_expect((tmp == -1), 0))
                return -1;
              if (__builtin_expect((tmp != 1), 0))
                return res;
              continue;
            }
        }
      else

        /* --- Compute chunk length (no newline) --------------------------- */
        /* ---------------------------------------------------------------- */

        bytes = tty_write_chunk_len(p, len - l);

      /* --- Write chunk (no newline) ------------------------------------- */
      /*
       * Write up to 256 bytes or until a newline is encountered.
       * This ensures the console remains interruptible and responsive.
       */

      if (bytes)
        {
          tmp = __do_sync_write(f, p, bytes);
          if (__builtin_expect((tmp == -1), 0))
            return tmp;

          res += tmp;

          if (__builtin_expect((tmp != bytes), 0))
            return res;
        }
    }

  /* ----------------------------------------------------------------------- */
  /*  Return total bytes consumed from caller buffer                         */
  /* ----------------------------------------------------------------------- */

  /* --- Return total bytes written --------------------------------------- */
  /*
   * The return value matches POSIX semantics: number of bytes consumed
   * from the caller's buffer, not necessarily the number of bytes written
   * to the underlying device.
   */

  return res;
}
