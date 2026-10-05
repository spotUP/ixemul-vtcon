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
 *  Move asynchronous ACTION_WAIT_CHAR packets from shared struct file
 *  storage to process-private state keyed by struct file.
 *  Add helpers to drain and release that state on local close and process
 *  teardown, preventing reply-port ownership from crossing vfork/exec.
 *  Preserve POLL readiness and idempotent SELCMD_CANCEL semantics.
 *
 * Revision 1.5  2026/08/07  ChatGPT modifications  (JJ)
 *
 *  Cache the converted file handle across IsInteractive() and WaitForChar()
 *  in the synchronous POLL path, avoiding a repeated struct-file load and
 *  BPTR conversion.  Build the select-port signal mask from unsigned long
 *  1 so the highest Exec signal bit is represented without a signed shift.
 *  Select state-machine and readiness semantics are unchanged.
 *
 * Revision 1.4  2026/07/28  ChatGPT modifications  (JJ)
 *
 *  Added explicit SELCMD_CANCEL handling.  Normal AmigaOS filehandles do
 *  not store a task registration in this backend, and an outstanding
 *  ACTION_WAIT_CHAR packet cannot be generically aborted here, so CANCEL
 *  is an intentional idempotent no-op.
 *
 * Revision 1.3  2026/05/31  Copilot modifications (JJ)
 *  Reviewed and modernized select() backend for normal AmigaOS handlers.
 *  No behavioral changes introduced.
 *
 *  - Added DEBUG_FSELECT infrastructure for optional diagnostics.
 *  - Improved documentation of SELCMD_PREPARE / SELCMD_CHECK /
 *      SELCMD_POLL semantics.
 *  - Clarified fallback behavior for handlers lacking ACTION_WAIT_CHAR.
 *  - No changes to:
 *      * readiness semantics (PREPARE vs CHECK/POLL)
 *      * WaitForChar() usage
 *      * PTY/FIFO select?packet interactions
 *
 *  API and observable behavior remain unchanged.
 * 
 *
 * Revision 1.6  2026/08/29  ChatGPT modifications (JJ)
 *
 *  Move asynchronous ACTION_WAIT_CHAR packets from shared struct file
 *  storage to process-private state keyed by struct file.
 *  Add helpers to drain and release that state on local close and process
 *  teardown, preventing reply-port ownership from crossing vfork/exec.
 *  Preserve POLL readiness and idempotent SELCMD_CANCEL semantics.
 *
 * Revision 1.5  2026/08/07  ChatGPT modifications  (JJ)
 *
 *  Cache the converted file handle across IsInteractive() and WaitForChar()
 *  in the synchronous POLL path, avoiding a repeated struct-file load and
 *  BPTR conversion.  Build the select-port signal mask from unsigned long
 *  1 so the highest Exec signal bit is represented without a signed shift.
 *  Select state-machine and readiness semantics are unchanged.
 *
 * Revision 1.4  2026/07/28  ChatGPT modifications  (JJ)
 *
 *  Added explicit SELCMD_CANCEL handling.  Normal AmigaOS filehandles do
 *  not store a task registration in this backend, and an outstanding
 *  ACTION_WAIT_CHAR packet cannot be generically aborted here, so CANCEL
 *  is an intentional idempotent no-op.
 *
 * Revision 1.3  2026/05/31  Copilot modifications (JJ)
 *  Reviewed and modernized select() backend for normal AmigaOS handlers.
 *  No behavioral changes introduced.
 *
 *  - Added DEBUG_FSELECT infrastructure for optional diagnostics.
 *  - Improved documentation of SELCMD_PREPARE / SELCMD_CHECK /
 *      SELCMD_POLL semantics.
 *  - Clarified fallback behavior for handlers lacking ACTION_WAIT_CHAR.
 *  - No changes to:
 *      * readiness semantics (PREPARE vs CHECK/POLL)
 *      * WaitForChar() usage
 *      * PTY/FIFO select?packet interactions
 *
 *  API and observable behavior remain unchanged.
 */

/*
 *  This file implements the select() backend for non-TTY, non-socket
 *  AmigaOS filehandles. It provides:
 *
 *    SELCMD_PREPARE: issue ACTION_WAIT_CHAR for read readiness
 *                    (write/exception modes are not armed here)
 *
 *    SELCMD_CHECK:   interpret WaitChar result or fallback on errors;
 *                    write/exception modes are treated as always ready
 *
 *    SELCMD_POLL:    synchronous readiness check via WaitForChar();
 *                    write/exception modes are treated as always ready
 *
 *    SELCMD_CANCEL:  explicit no-op; outstanding asynchronous packets
 *                    remain process-private and complete normally
 *
 *  Limitations:
 *    Only SELMODE_IN (read) is armed in PREPARE.
 *    SELCMD_CHECK/POLL treat write readiness as always ready.
 *    Exceptional conditions are treated as read?ready to avoid deadlock.
 *
 *  This logic must remain consistent with:
 *    - __read.c (interactive vs non interactive behavior)
 *    - __tioctl.c (IsInteractive gating)
 *    - PTY/FIFO select packet handling in __close.c
 * ---------------------------------------------------------------------------
 */




#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"


#include "select.h"

/* don't use the `normal' packet port for select. We need synchronous
 * notification to be able to Wait() for multiple replies */

/* --- Overview ------------------------------------------------------------ */
/*
 * For normal AmigaOS filehandles, select() is implemented using:
 *   - ACTION_WAIT_CHAR packets for asynchronous readiness
 *   - WaitForChar() for synchronous polling
 *   - Minimal interpretation of handler return values
 */

/*
 * select operation on a "normal" AmigaOS filehandle. Normal means,
 * we only have the WaitForChar() call available to find out, whether
 * a read would block. Write() cannot be caught, so we always allow it.
 * Request for exceptional data is routed to read.
 */

struct ix_fselect_state {
  struct ix_fselect_state *next;
  struct file *file;
  struct StandardPacket packet;
};

static struct ix_fselect_state *
fselect_find_state(struct user *p, struct file *f)
{
  struct ix_fselect_state *st;

  st = p->u_fselect_states;
  while (st)
    {
      if (st->file == f)
        return st;
      st = st->next;
    }

  return NULL;
}

static struct ix_fselect_state *
fselect_get_state(struct user *p, struct file *f)
{
  struct ix_fselect_state *st;

  st = fselect_find_state(p, f);
  if (st)
    return st;

  st = (struct ix_fselect_state *)kmalloc(sizeof(*st));
  if (!st)
    return NULL;

  bzero(st, sizeof(*st));
  __init_std_packet(&st->packet);
  st->file = f;
  st->next = p->u_fselect_states;
  p->u_fselect_states = st;

  return st;
}

struct StandardPacket *
__fselect_get_packet(struct file *f)
{
  usetup;
  struct ix_fselect_state *st;

  st = fselect_find_state(u_ptr, f);
  return st ? &st->packet : NULL;
}

void
__fselect_release_file(struct file *f)
{
  usetup;
  struct ix_fselect_state **link;
  struct ix_fselect_state *st;

  link = &u.u_fselect_states;
  while (*link && (*link)->file != f)
    link = &(*link)->next;

  st = *link;
  if (!st)
    return;

  /* Detach first so cleanup cannot encounter the state a second time. */
  *link = st->next;
  st->next = NULL;

  __wait_select_packet(&st->packet);

  st->file = NULL;
  kfree(st);
}

void
__fselect_cleanup_process(void)
{
  usetup;
  struct ix_fselect_state *st;

  while ((st = u.u_fselect_states) != NULL)
    {
      /* Do not dereference st->file here; final close may already have run. */
      u.u_fselect_states = st->next;
      st->next = NULL;

      __wait_select_packet(&st->packet);

      st->file = NULL;
      kfree(st);
    }
}

int
__fselect (struct file *f, int select_cmd, int io_mode,
	   fd_set *ignored, u_long *also_ignored)
{
  usetup;
  int result;
  struct ix_fselect_state *st;

  /* --- SELCMD_CANCEL ---------------------------------------------------- */
  /*
   * ACTION_WAIT_CHAR cannot be generically aborted here.  Keeping CANCEL
   * as a no-op is safe because each process now owns its own packet state;
   * that state is reused by later select() calls and drained on local close.
   */
  if (select_cmd == SELCMD_CANCEL)
    return 0;

  /* --- SELCMD_PREPARE --------------------------------------------------- */
  /*
   * Issue ACTION_WAIT_CHAR with a 10-second timeout.
   * Only SELMODE_IN (read) is meaningful; other modes return "not ready".
   * Returns the signal bit mask for this process's select packet port.
   */
  if (select_cmd == SELCMD_PREPARE)
    {
      if (io_mode != SELMODE_IN)
        return 0;

      st = fselect_get_state(u_ptr, f);
      if (!st)
        {
          errno = ENOMEM;
          return -1;
        }

      st->packet.sp_Pkt.dp_Res2 = 0;

      /* 10 seconds waittime */
      if (!st->packet.sp_Pkt.dp_Port)
        {
          st->packet.sp_Pkt.dp_Port = __selport;
          st->packet.sp_Pkt.dp_Type = ACTION_WAIT_CHAR;
          st->packet.sp_Pkt.dp_Arg1 = 10 * 1000000;
          PutPacket(f->f_fh->fh_Type, &st->packet);
        }

      return 1UL << __selport->mp_SigBit;
    }
  else if (select_cmd == SELCMD_CHECK)
    {
      /* only read is supported, other modes default to `ok' */
      if (io_mode != SELMODE_IN)
        return 1;

      st = fselect_find_state(u_ptr, f);
      if (!st)
        return 0;

      if (st->packet.sp_Pkt.dp_Port)
        return 0;

      /*
       * There are two possible answers: error (packet not supported) and
       * the real answer.  An error is treated as ready so select() cannot
       * block indefinitely.  & 1 converts DOS true (-1) to normal true (1).
       */
      result = st->packet.sp_Pkt.dp_Res2
             ? 1
             : (st->packet.sp_Pkt.dp_Res1 & 1);

      /* Do not make __write() think its last packet failed. */
      st->packet.sp_Pkt.dp_Res2 = 0;
      return result;
    }
  else if (select_cmd == SELCMD_POLL)
    {
      BPTR fh;

      /* only read is supported, other modes default to `ok' */
      if (io_mode != SELMODE_IN)
        return 1;

      __get_file (f);

      fh = CTOBPTR(f->f_fh);
      result = !IsInteractive(fh) || WaitForChar(fh, 0);
      __release_file (f);
      return result;
    }
  else
    return 0;
}
