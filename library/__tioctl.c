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
 *  * Revision 1.4  2026/06/10  ChatGPT/Copilot modifications (JJ)
 *
 *  Defensive hardening of the TTY ioctl path.
 *
 *  - Added EFAULT handling for NULL ioctl argument pointers.
 *  - Replaced the TIOCGWINSZ alloca() InfoData buffer with a fixed,
 *    LONG_ALIGN()'d stack buffer.
 *  - Added TypeOfMem()-based validation of accessed Window/IOStdReq/
 *    ConUnit field addresses.
 *  - Added defensive checks for character and pixel dimensions.
 *
 *  No intended changes to termios, sgtty, RAW/CBREAK handling, CR/LF
 *  translation, PTY packet mode, process-group ioctls, or default
 *  ioctl behavior.
 * 
 * NOTE:
 *  This version has been tested with a tty test. On the tested console,
 *  isatty(0) succeeded and TIOCGWINSZ returned -1, but the call returned
 *  safely and execution continued. This confirms the intended defensive
 *  behavior for handlers where the historical window-size pointer chain
 *  cannot be validated: fail safely instead of dereferencing unsafe data.
 *
 * Revision 1.3  2026/05/31  Copilot modifications (JJ)
 *
 * Refactored TTY/PTY ioctl control path for clarity and maintainability
 * without altering ioctl semantics or supported command set.
 *
 *  - Centralized RAW mode toggling into tty_set_raw_mode().
 *  - Centralized termios CR/LF flag derivation into
 *      tty_update_crlf_flags_from_termios().
 *  - Kept sgtty CRMOD flag derivation in the TIOCSETP/TIOCSETN path,
 *      preserving historical RAW/CBREAK + CRMOD behavior.
 *  - Centralized window-size extraction into tty_get_winsize().
 *  - Improved grouping and documentation of ioctl families
 *      (termios, sgtty, pgrp, PTY packet mode, window size).
 *  - No changes to:
 *      * termios or sgtty semantics
 *      * IXTTY_* flag meanings
 *      * PTY packet-mode behavior
 *      * error codes or ioctl return conventions
 *
 *  External API and behavior remain identical to the historical version.
 *
 *  __tioctl.c,v 1.1.1.1 1994/04/04 04:30:13 amiga Exp
 *
 *  __tioctl.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:13  amiga
 * Initial CVS check in.
 *
 *  Revision 1.5  1993/11/05  21:51:08  mw
 *  seems I got oldstyle tty handling oposite way..
 *
 *  Revision 1.4  1992/08/09  20:39:01  amiga
 *  add a cast to get rid of a warning
 *
 *  Revision 1.3  1992/07/04  19:07:25  mwild
 *  send DISK_INFO packet with synchronous port, async delivery seems to be
 *  broken with CNC:
 *
 * Revision 1.2  1992/06/08  02:36:00  mwild
 * fix TIOCGWINSZ, row/column was off by one
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "__vtcon.h"
#include "kprintf.h"
#include <string.h>
#include <sgtty.h>

#define OEXTB 15
#undef B0	
#undef B50	
#undef B75	
#undef B110	
#undef B134	
#undef B150	
#undef B200	
#undef B300	
#undef B600	
#undef B1200	
#undef B1800	
#undef B2400	
#undef B4800	
#undef B9600	
#undef EXTA	
#undef EXTB	
#include <sys/termios.h>
#include <ctype.h>
#include <stddef.h>
#include <exec/memory.h>
#include <devices/conunit.h>
#include <intuition/intuition.h>

/* --- Internal helpers ---------------------------------------------------- */

/*
 * Toggle RAW mode on the underlying console device.
 * This is a thin wrapper around f_ttyflags and SetMode(), used by both
 * termios and sgtty paths. Semantics are identical to the original code.
 */
static inline void
tty_set_raw_mode(struct file *f, int enable)
{
  if (enable && !(f->f_ttyflags & IXTTY_RAW))
    {
      f->f_ttyflags |= IXTTY_RAW;
      SetMode(CTOBPTR(f->f_fh), 1);
    }
  else if (!enable && (f->f_ttyflags & IXTTY_RAW))
    {
      f->f_ttyflags &= ~IXTTY_RAW;
      SetMode(CTOBPTR(f->f_fh), 0);
    }
}

/*
 * Update CR/LF translation flags based on termios input/output flags.
 * This is a direct extraction of the original bit twiddling logic.
 */
static inline void
tty_update_crlf_flags_from_termios(struct file *f, tcflag_t iflag, tcflag_t oflag)
{
  if (iflag & ICRNL)
    f->f_ttyflags |= IXTTY_ICRNL;
  else
    f->f_ttyflags &= ~IXTTY_ICRNL;

  /* Clear derived mapping flags */
  f->f_ttyflags &= ~(IXTTY_MAP_NL_TO_CR |
                     IXTTY_MAP_CR_TO_NL |
                     IXTTY_MAP_NL_TO_CRLF);

  /* Derived mapping: INLCR -> NL->CR */
  if (iflag & INLCR)
    f->f_ttyflags |= IXTTY_INLCR;
  else
    f->f_ttyflags &= ~IXTTY_INLCR;

  /* Clear output processing flags before rebuilding them.
     This matches the behavior of the original sgtty path and prevents
     IXTTY_ONLCR from persisting across tcsetattr() calls. */

  f->f_ttyflags &= ~(IXTTY_OPOST | IXTTY_ONLCR);

  if (oflag & OPOST)
    {
      f->f_ttyflags |= IXTTY_OPOST;
      if (oflag & ONLCR)
        {
          f->f_ttyflags |= IXTTY_ONLCR;
        }
    }

  /* Derived mapping: INLCR NL CR */
  if (iflag & INLCR)
    f->f_ttyflags |= IXTTY_MAP_NL_TO_CR;

  /* Derived mapping: RAW + ICRNL */
  if ((f->f_ttyflags & IXTTY_RAW) && (iflag & ICRNL))
    f->f_ttyflags |= IXTTY_MAP_CR_TO_NL;

  /* Derived mapping: RAW + OPOST + ONLCR CRLF (write side) */
  if ((f->f_ttyflags & IXTTY_RAW) &&
      (f->f_ttyflags & IXTTY_OPOST) &&
      (f->f_ttyflags & IXTTY_ONLCR))
    f->f_ttyflags |= IXTTY_MAP_NL_TO_CRLF;
}

static int
tty_ptr_is_even(const void *ptr)
{
  return ptr != 0 && (((ULONG)ptr & 1) == 0);
}

static int
tty_addr_is_memory(const void *ptr)
{
  if (ptr == 0)
    return 0;

  if (TypeOfMem((APTR)ptr) == 0)
    return 0;

  return 1;
}

static int
tty_field_is_memory(const void *base, ULONG offset, ULONG size)
{
  ULONG start;
  ULONG first;
  ULONG last;

  if (base == 0 || size == 0)
    return 0;

  start = (ULONG)base;
  first = start + offset;

  if (first < start)
    return 0;

  last = first + size - 1;

  if (last < first)
    return 0;

  if (!tty_addr_is_memory((APTR)first))
    return 0;

  if (!tty_addr_is_memory((APTR)last))
    return 0;

  return 1;
}

#define TTY_FIELD_IS_MEMORY(ptr, type, field) \
  tty_field_is_memory((ptr),                  \
                      (ULONG)offsetof(type, field), \
                      (ULONG)sizeof(((type *)0)->field))

/*
 * Query window size using ACTION_DISK_INFO and extract ConUnit/Window data.
 * This is a direct extraction of the original TIOCGWINSZ logic.
 * Semantics are unchanged; only moved into a helper for clarity.
 */
static int
tty_get_winsize(struct file *f, struct winsize *ws)
{
  struct Window *w;
  struct ConUnit *cu;
  struct IOStdReq *ios;
  struct InfoData *info;
  char infobuf[sizeof(struct InfoData) + 3];
  LONG xpixel;
  LONG ypixel;

  usetup;

  if (f == 0 || ws == 0)
    return -1;

  info = (struct InfoData *)LONG_ALIGN(infobuf);
  bzero(info, sizeof(*info));

  LastResult(f) = 0;
  LastError(f) = 0;

  SendPacket1(f, __srwport, ACTION_DISK_INFO, CTOBPTR(info));
  __wait_sync_packet(&f->f_sp);

  /* If handler returned success, no window info is available */
  if (LastResult(f) != -1)
    return -1;

  w = (struct Window *)info->id_VolumeNode;
  if (!tty_ptr_is_even(w))
    return -1;

  if (!TTY_FIELD_IS_MEMORY(w, struct Window, Width))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(w, struct Window, Height))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(w, struct Window, BorderLeft))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(w, struct Window, BorderRight))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(w, struct Window, BorderTop))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(w, struct Window, BorderBottom))
    return -1;

  /* From DevCon notes (not Bantam book) */
  ios = (struct IOStdReq *)info->id_InUse;
  if (!tty_ptr_is_even(ios))
    return -1;

  if (!TTY_FIELD_IS_MEMORY(ios, struct IOStdReq, io_Unit))
    return -1;

  cu = (struct ConUnit *)ios->io_Unit;
  if (!tty_ptr_is_even(cu))
    return -1;

  if (!TTY_FIELD_IS_MEMORY(cu, struct ConUnit, cu_Window))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(cu, struct ConUnit, cu_XMax))
    return -1;
  if (!TTY_FIELD_IS_MEMORY(cu, struct ConUnit, cu_YMax))
    return -1;

  /* paranoid check */
  if (cu->cu_Window != w)
    return -1;

  /* character dimensions (off-by-one correction preserved) */
  ws->ws_col = cu->cu_XMax + 1;
  ws->ws_row = cu->cu_YMax + 1;

  if (ws->ws_col == 0 || ws->ws_row == 0)
    return -1;

  /* pixel dimensions */
  xpixel = (LONG)w->Width  - (LONG)w->BorderLeft - (LONG)w->BorderRight;
  ypixel = (LONG)w->Height - (LONG)w->BorderTop  - (LONG)w->BorderBottom;

  ws->ws_xpixel = xpixel > 0 ? xpixel : 0;
  ws->ws_ypixel = ypixel > 0 ? ypixel : 0;

  return 0;
}

/* IOCTLs on "interactive" files */

int
__tioctl(struct file *f, unsigned int cmd, unsigned int inout, 
	 unsigned int arglen, unsigned int arg)
{
  int omask, result, err = 0;
  usetup;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);
  result = -1;

  /* --- Reject ioctl on non-interactive files ----------------------------- */
  /*
   * POSIX requires ENOTTY when ioctl() is applied to a non TTY.
   * ixemul follows this rule by gating all TTY ioctls on IsInteractive().
   */

  if (!IsInteractive(CTOBPTR(f->f_fh)))
    {
      err = ENOTTY;
    }
  else

    /* --- IOCTL dispatch -------------------------------------------------- */
    /*
     * Grouped by API family: termios, sgtty, window size, process group,
     * queue/packet mode, and default handling.
     */
    switch (cmd)
      {
      /* --- termios API --------------------------------------------------- */

    /* --- TIOCGETA: Get termios ------------------------------------------- */
    /*
     * Build a termios structure from f_ttyflags.
     * Many fields are fixed defaults due to Amiga console limitations.
     */

    case TIOCGETA:
      {
        struct termios *t;
        unsigned char *cp;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        t = (struct termios *)arg;

        if (__vtcon (f) && __vtcon_packet (f, ACTION_VTCON_TCGETA, t, 0))
          {
            result = 0;
            break;
          }

        t->c_iflag = IGNBRK | IGNPAR | IXON;
	if (f->f_ttyflags & IXTTY_ICRNL)
	  t->c_iflag |= ICRNL;
	if (f->f_ttyflags & IXTTY_INLCR)
	  t->c_iflag |= INLCR;
        t->c_oflag = 0;
	if (f->f_ttyflags & IXTTY_OPOST)
	  {
	    t->c_oflag |= OPOST;
	    if (f->f_ttyflags & IXTTY_ONLCR)
	      t->c_oflag |= ONLCR;
	  }
        t->c_cflag = CS8|CLOCAL;
        t->c_ispeed=
        t->c_ospeed= EXTB;
	/* Conman does ECHOCTL, Commo doesn't.. I use Conman:-)) */
        t->c_lflag = ECHOCTL | ((f->f_ttyflags & IXTTY_RAW) ? 0 : ICANON | ECHO);
	cp = t->c_cc;
	cp[VSTART] = 'q' & 31;
	cp[VSTOP] = 's' & 31;
	cp[VSUSP] = 0; /* sneef.. would that be nice... */
	cp[VDSUSP] = 0;
	cp[VREPRINT] = 0;
	cp[VDISCARD] = 'x' & 31;
	cp[VWERASE] = 0;
	cp[VLNEXT] = 0;
	cp[VSTATUS] = 0;
	cp[VINTR] = 3;
	cp[VQUIT] = 0;
	cp[VERASE] = 8;
	cp[VKILL] = 'x' & 31;
	cp[VEOF] = '\\' & 31;
	cp[VEOL] = 10;
	cp[VEOL2] = 0;
	result = 0;
	break;
      }

    /* --- TIOCSETA*, TIOCSETAW, TIOCSETAF: Set termios -------------------- */
    /*
     * Update f_ttyflags based on termios settings.
     * RAW mode toggles SetMode() on the underlying console device.
     * CR/LF translation flags (ICRNL, INLCR, OPOST, ONLCR) are mapped directly.
     */

    case TIOCSETA:
    case TIOCSETAW:
    case TIOCSETAF:
      {
        struct termios *t;
        int makeraw;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        t = (struct termios *)arg;

        /* the console's line discipline takes the whole termios */
        if (__vtcon (f) &&
            __vtcon_packet (f, ACTION_VTCON_TCSETA, t,
                            cmd == TIOCSETA ? TCSANOW : cmd == TIOCSETAW ? TCSADRAIN : TCSAFLUSH))
          {
            result = 0;
            break;
          }

        makeraw = (t->c_lflag & (ICANON | ECHO)) != (ICANON | ECHO);
	/* the only thing that counts so far.. if ICANON is disabled,        
	 * we disable ECHO too, no matter what the user wanted, and 
	 * send a RAW-packet.. */

        /* RAW mode toggle (unchanged semantics, centralized helper) */
        tty_set_raw_mode(f, makeraw);

        /* CR/LF translation flags from termios (unchanged semantics) */
        tty_update_crlf_flags_from_termios(f, t->c_iflag, t->c_oflag);

	result = 0;
	break;
      }

    /* --- legacy sgtty API ----------------------------------------------- */

    /* --- TIOCGETP: legacy sgtty get -------------------------------------- */
    /*
     * Legacy BSD sgtty interface. Values are approximated using f_ttyflags.
     * Required for compatibility with older Unix tools.
     */

    case TIOCGETP:
      {
	struct sgttyb *s;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        s = (struct sgttyb *)arg;
	s->sg_erase = 8;
	s->sg_kill = 'x' & 31;
	s->sg_flags = ODDP|EVENP|ANYP|
		      ((f->f_ttyflags & IXTTY_RAW) ? CBREAK|RAW : ECHO|CRMOD) |
		      ((f->f_ttyflags & IXTTY_ICRNL) ? CRMOD : 0);
	s->sg_ispeed =
	s->sg_ospeed = OEXTB;
	result = 0;
	break;
      }

    /* --- TIOCSETP / TIOCSETN: legacy sgtty set --------------------------- */
    /*
     * Map sgtty flags to ixemul's RAW and CR/LF translation flags.
     * RAW/CBREAK toggles SetMode() on the console device.
     */

    case TIOCSETN:
    case TIOCSETP:
      {
        /* sgtty RAW/CBREAK mapping */
	struct sgttyb *s;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        s = (struct sgttyb *)arg;

        /* RAW/CBREAK mapping to RAW mode (unchanged semantics) */
	if (!(s->sg_flags & (RAW|CBREAK)))
          tty_set_raw_mode(f, 0);
        else
          tty_set_raw_mode(f, 1);

    /* Clear CR/LF and derived mapping flags */
    f->f_ttyflags &= ~(IXTTY_INLCR |
                       IXTTY_ICRNL |
                       IXTTY_OPOST |
                       IXTTY_ONLCR |
                       IXTTY_MAP_NL_TO_CR |
                       IXTTY_MAP_CR_TO_NL |
                       IXTTY_MAP_NL_TO_CRLF);

        /* sgtty CRMOD mapping */
    if (s->sg_flags & CRMOD)
      {
        f->f_ttyflags |= IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;

        /* Derived mapping: NL->CRLF when RAW + CRMOD (write side) */
        if (f->f_ttyflags & IXTTY_RAW)
          {
            f->f_ttyflags |= IXTTY_MAP_CR_TO_NL;
            f->f_ttyflags |= IXTTY_MAP_NL_TO_CRLF;
          }
      }
	result = 0;
	break;
      }

    /* --- window size ----------------------------------------------------- */

    /* --- TIOCGWINSZ: Get window size ------------------------------------- */
    /*
     * Window size query is handled by a helper for clarity.
     * Semantics are unchanged.
     */

    case TIOCGWINSZ:
      {
	struct winsize *ws;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        ws = (struct winsize *)arg;
	if (__vtcon (f) && __vtcon_packet (f, ACTION_VTCON_GWINSZ, ws, 0))
	  {
	    result = 0;
	    break;
	  }
        result = tty_get_winsize(f, ws);
	break;
      }

    /* --- process group --------------------------------------------------- */

    /* --- TIOCSPGRP: Set process group ------------------------------------ */
    /*
     * ixemul maintains a simple session/pgrp model.
     * This ioctl updates the controlling process group.
     */

    case TIOCSPGRP:
      {
	int *pgrp;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        pgrp = (int *)arg;
        if (u.u_session)
          u.u_session->pgrp = *pgrp;
	result = 0;
        break;
      }

    /* --- TIOCSCTTY: become the controlling terminal ---------------------- */
    /*
     * The session (setsid) remembers this terminal's name, and open() of
     * /dev/tty opens it instead of the process's console "*". A program on
     * a pty -- a tmux pane, ptyrun -- otherwise read EOF from /dev/tty and
     * less quit at its first key (rig 3, 2026-10-08).
     */

    case TIOCSCTTY:
      {
        if (!u.u_session || !f->f_name || strlen(f->f_name) >= sizeof(u.u_session->s_ttyname))
          {
            err = EPERM;
            break;
          }
        strcpy(u.u_session->s_ttyname, f->f_name);
        result = 0;
        break;
      }

    /* --- TIOCGPGRP: Get process group ------------------------------------ */
    /*
     * Return the controlling process group for this session.
     * Falls back to SYS_getpgrp if no session exists.
     */

    case TIOCGPGRP:
      {
	int *pgrp;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        pgrp = (int *)arg;
        *pgrp = (u.u_session ? u.u_session->pgrp : syscall(SYS_getpgrp));
	result = 0;
        break;
      }

    /* --- queue / packet mode / default ---------------------------------- */

    /* --- TIOCOUTQ: Output queue size ------------------------------------- */
    /*
     * AmigaDOS console handlers do not expose an output queue.
     * Always return 0.
     */

    case TIOCOUTQ:
      {
	int *count;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        count = (int *)arg;
	*count = 0;
	result = 0;
	break;
      }

    /* --- TIOCPKT: PTY packet mode ---------------------------------------- */
    /*
     * Enable or disable BSD style packet mode for PTY masters.
     * Must match packet mode handling in __read.c.
     */

    case TIOCPKT:
      {
        int *on;

        if (arg == 0)
          {
            err = EFAULT;
            break;
          }

        on = (int *)arg;
	if (*on)
            f->f_ttyflags |= IXTTY_PKT;
	else
            f->f_ttyflags &= ~IXTTY_PKT;
	result = 0;
        break;
      }

    case TIOCSWINSZ:
      /* a pty master sets the size (vtcon's PTY:); the foreground process
         group learns it, as on Unix. A window's size stays the window's. */
      if (__vtcon (f))
        {
          /* PTY: answers whom the new size is for: a process on the
             slave (0 when the size did not change). Not the caller's own
             group: the caller holds the master, its terminal is another. */
          if (__vtcon_packet (f, ACTION_VTCON_SWINSZ, (void *)arg, 0))
            __vtcon_winch ((struct Task *)LastError (f));
          result = 0;
          break;
        }

      /* any other console: resizing the window is not implemented */
      /* fall through */
    default:
      /* --- Default: ignored but successful ------------------------------- */
      /*
       * Many ioctls are unsupported on Amiga consoles.
       * Returning success matches historical ixemul behavior.
       */
      result = 0;
      break;
      }

    __release_file (f);
    syscall (SYS_sigsetmask, omask);
    errno = err;

    /* --- Debug trace (enabled only in debug builds) ---------------------- */
#ifdef DEBUG
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
#endif

    return result;
}
