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
 *  * Revision 1.4  2026/08/05  ChatGPT modifications (JJ)
 *
 * - Check FNDELAY before calling IsInteractive(), avoiding the historical
 *   IsInteractive() call on every normal blocking read.
 * - Snapshot f_ttyflags before releasing the file reference and use that
 *   snapshot for CR/LF post-processing, avoiding access to struct file after
 *   __release_file() without extending the reference across the buffer scan.
 *
 * Revision 1.3  2026/05/31  Copilot/ChatGPT modifications (JJ)
 *
 * Refactored internal TTY read datapath without changing the external API.
 *
 * - Extracted packet-mode pre/post logic into helpers.
 * - Extracted CR/LF translation into tty_apply_crlf_translation().
 * - Preserved RAW/cooked semantics, CR/LF mapping, errno propagation
 *   and normal interactive/non-interactive read behavior.
 * - Guard packet-mode pre-processing so a one-byte read buffer is not
 *   consumed by a packet header without room for payload.
 * - Avoid calling IsInteractive() on the normal read path unless FNDELAY
 *   is set.
 *
 *  __read.c,v 1.1.1.1 1994/04/04 04:30:12 amiga Exp
 *
 *  __read.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:12  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "__vtcon.h"
#include "kprintf.h"

/*
 * Starting with ixemul 42.0 we switched from asynchronous to synchronous
 * reads.  Apparently the ADOS CON handler uses the mp_SigTask field of the
 * MsgPort struct of an ACTION_READ package to find the Task which it should
 * signal when the user presses Ctrl-C.  However, async reads use a special
 * port where the mp_SigTask field points to an Interrupt struct.  The CON
 * handler seems to either use that address as the new Task pointer (I hope
 * not) or disable Ctrl-C signalling entirely.  I never noticed this until it
 * was pointed out to me, since I always use KingCON. KingCON seems to handle
 * Ctrl-C differently. Just to be sure I also changed the ACTION_WAIT_CHAR
 * to synchronous mode.
 *
 * Starting with 43.1 I no longer use async writes at all. The overhead in
 * handling async writes turned out to be bigger than the gain.
 */

/* --- Internal helpers ---------------------------------------------------- */

/*
 * Packet mode pre rocessing:
 *   - If IXTTY_PKT is set, prepend a zero byte and reduce the available
 *     read length. Semantics identical to the inline logic previously used.
 *
 * Returns non zero if packet mode was active, 0 otherwise.
 */
static inline int
tty_packetmode_pre(struct file *f, char **buf, int *len)
{
  /*
   * Packet mode requires space for both:
   *   - the packet header (1 byte)
   *   - at least one payload byte from Read()
   *
   * If len <= 1, there is no room for both header and payload.
   * Do not enter packet mode for this read; perform an ordinary read instead.
   */
  if ((f->f_ttyflags & IXTTY_PKT) && *len > 1)
    {
      (*len)--;
      *(*buf)++ = 0;
      return 1;
    }

  return 0;
}

/*
 * Packet mode post processing:
 *   - If packet mode was active and res > 0, increment the returned count
 *     to include the prepended zero byte.
 *
 * Returns the adjusted result.
 */
static inline int
tty_packetmode_post(int packet_mode_active, int res)
{
  if (packet_mode_active && res > 0)
    return res + 1;
  return res;
}

/*
 * CR/LF translation helper:
 *   - INLCR: NL CR
 *   - RAW + ICRNL: CR NL
 *
 * This is a direct extraction of the original inline logic.
 * Semantics are unchanged.
 */
static inline void
tty_apply_crlf_translation(int ttyflags, char *buf, int len)
{
  int i;
  char match, subst;

  /*
   * Level  CR/LF mapping:
   *   - IXTTY_MAP_NL_TO_CR : NL CR
   *   - IXTTY_MAP_CR_TO_NL : CR NL
   *
   * These flags are precomputed in __tioctl.c.
   * If neither is set, no translation is required.
   */

  if (!(ttyflags & (IXTTY_MAP_NL_TO_CR | IXTTY_MAP_CR_TO_NL)))
    return;

  if (ttyflags & IXTTY_MAP_NL_TO_CR)
    {
      match = '\n';
      subst = '\r';
    }
  else if (ttyflags & IXTTY_MAP_CR_TO_NL)
    {
      match = '\r';
      subst = '\n';
    }
  else
    return; /* defensive: should never happen */

  for (i = 0; i < len; i++)
    if (buf[i] == match)
      buf[i] = subst;
}

int __read(struct file *f, char *buf, int len)
{
  usetup;
  int err = errno, res = 0;
  int omask;
  int packet_mode_active = 0;
  int ttyflags;
  char *orig_buf;

  /* --- Early exit: NIL handler always returns EOF ------------------------ */
  /*
   * If the file handle corresponds to a NIL device, there is never any data
   * to read. Returning 0 matches AmigaDOS semantics and avoids unnecessary
   * processing. No locking or signal masking is needed in this case.
   */

  /* always return EOF */
  if (HANDLER_NIL(f)) return 0;

  /* Early exit: nothing to read */
  if (len <= 0)
    return 0;

  /* Normal path begins here */

  /* a background job reading its terminal stops (SIGTTIN) */
  if (__vtcon (f) && __vtcon_bg (f, SIGTTIN) < 0)
    return -1;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);
  
  /* ----------------------------------------------------------------------- */
  /*  Packet?mode pre processing                                             */
  /* ----------------------------------------------------------------------- */

  /* --- Packet mode: prepend zero byte ----------------------------------- */
  /*
   * Centralized packet mode pre processing.
   * Semantics identical to the original inline logic.
   */
  packet_mode_active = tty_packetmode_pre(f, &buf, &len);
  orig_buf = buf;
  ttyflags = 0;

  /* ----------------------------------------------------------------------- */
  /*  Main read logic                                                        */
  /* ----------------------------------------------------------------------- */

  /* --- Main read logic --------------------------------------------------- */
  /*
   * Two paths:
   *   1. Interactive + FNDELAY poll with WaitForChar(), read 1 byte at a time
   *   2. Normal blocking read  Read() for full length
   */

  if (len > 0)
    {
      /* FNDELAY slow path */
      if (__builtin_expect((f->f_flags & FNDELAY), 0) &&
          IsInteractive(CTOBPTR(f->f_fh)))
        {
          /*
           * Interactive + FNDELAY path:
           *   - Poll using WaitForChar()
           *   - Read 1 byte at a time
           */
	  while (res < len)
            {
              if (!WaitForChar(CTOBPTR(f->f_fh), 0))
              {
                if (!res)
                {
                  res = -1;
                  err = EAGAIN;
                }
                break;
              }
              if (Read(CTOBPTR(f->f_fh), buf, 1) != 1)
		{
		  err = __ioerr_to_errno(IoErr());
		  /* if there really was no character to read, we should
		   * have escaped already at the 'if WaitForChar' line */
		  res = -1;
		  break;
		}
	      buf++;
	      res++;
	    }
	}	  
      else
        {
          /* Normal blocking read (fast path) */
	  res = Read(CTOBPTR(f->f_fh), buf, len);
	  if (res == -1)
	    err = __ioerr_to_errno(IoErr());
	}
   }

  /* Do not access f after dropping the active file reference. A vtcon
     console runs the line discipline itself (ICRNL, INLCR): no
     translation here on top of it. */
  ttyflags = __vtcon (f) ? 0 : f->f_ttyflags;
  __release_file (f);
  syscall (SYS_sigsetmask, omask);
  errno = err;

  /* ----------------------------------------------------------------------- */
  /*  Post processing: CR/LF translation                                     */
  /* ----------------------------------------------------------------------- */
  /* Debug trace (enabled only in debug builds) */
#ifdef DEBUG
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
#endif
  
  /* --- CR/LF translation ------------------------------------------------- */
  /*
   * Centralized CR/LF translation.
   * Semantics identical to the original inline logic.
   */
  if (res > 0)
    tty_apply_crlf_translation(ttyflags, orig_buf, res);

  /* ----------------------------------------------------------------------- */
  /*  Packet mode post processing                                            */
  /* ----------------------------------------------------------------------- */

  /* --- Packet mode post processing -------------------------------------- */
  /*
   * Centralized packet?mode post?processing.
   * Semantics identical to the original inline logic.
   */
  return tty_packetmode_post(packet_mode_active, res);
}
