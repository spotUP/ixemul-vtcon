/*
 *  The vtcon console (vtcon's XCON:, later its PTY:) runs a Unix line
 *  discipline of its own: termios, window size and signals are the
 *  console's there, not ixemul's (vtcon: handler/vtcon_packets.h,
 *  tty/ldisc.c). These are its packets. Any other console refuses them
 *  (dp_Res1 0), and ixemul keeps its old ways (SetMode, DISK_INFO).
 *
 *  Whether a file is on a vtcon console is asked once, with a TCGETA,
 *  and kept in f_ttyflags.
 */
#define _KERNEL
#include "ixemul.h"
#include <sys/termios.h>
#include "__vtcon.h"

int __vtcon_packet(struct file *f, long action, void *arg, long arg3)
{
  usetup;

  LastResult (f) = 0; LastError (f) = 0;
  SendPacket3 (f, __srwport, action, f->f_fh->fh_Arg1, (long)arg, arg3);
  __wait_sync_packet (&f->f_sp);
  return LastResult (f) != 0;
}

int __vtcon(struct file *f)
{
  usetup;

  if (!(f->f_ttyflags & IXTTY_VTCON_KNOWN))
    {
      struct termios t;

      f->f_ttyflags |= IXTTY_VTCON_KNOWN;
      if (IsInteractive (CTOBPTR (f->f_fh)) &&
          __vtcon_packet (f, ACTION_VTCON_TCGETA, &t, 0))
        {
          f->f_ttyflags |= IXTTY_VTCON;
          u.u_vtcon = 1;
        }
    }
  return (f->f_ttyflags & IXTTY_VTCON) != 0;
}

/* The console's ^\ and ^Z arrive as the CTRL_E and CTRL_F breaks (vtcon:
 * vtcon_packets.h): SIGQUIT and SIGTSTP for the foreground process group,
 * as CTRL_C is SIGINT. Only for a process on a vtcon console, and not when
 * it catches SIGMSG (then it wants the breaks themselves). Returns the
 * breaks it used. */
unsigned long __vtcon_breaks(unsigned long breaks)
{
  struct Process *proc;
  usetup;

  breaks &= SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F;
  if (!breaks || !u.u_vtcon || (u.p_sigcatch & sigmask(SIGMSG)))
    return 0;
  proc = (struct Process *)(u.u_session ? u.u_session->pgrp : (int)FindTask(0));
  /* whether a process ignores the signal is its own business (_psignal
     asks it): the shell that receives the break ignores SIGTSTP itself,
     and checking its mask here dropped ^Z for every job under tcsh */
  if (breaks & SIGBREAKF_CTRL_E)
    _psignalgrp(proc, SIGQUIT);
  if (breaks & SIGBREAKF_CTRL_F)
    _psignalgrp(proc, SIGTSTP);
  return breaks;
}

/* Does t still exist (running, ready or waiting)? Under Forbid. */
static int task_exists(struct Task *t)
{
  struct Node *n;

  if (t == SysBase->ThisTask)
    return 1;
  for (n = SysBase->TaskReady.lh_Head; n->ln_Succ; n = n->ln_Succ)
    if (n == &t->tc_Node)
      return 1;
  for (n = SysBase->TaskWait.lh_Head; n->ln_Succ; n = n->ln_Succ)
    if (n == &t->tc_Node)
      return 1;
  return 0;
}

/* SIGWINCH for a terminal whose size its pty master set: t is a process
 * on the slave (vtcon's PTY: names its break target), the signal goes to
 * its foreground process group, as a Unix tty sends it. A task that is
 * gone or no ixemul process gets nothing. */
void __vtcon_winch(struct Task *t)
{
  struct user *tu;

  if (!t)
    return;
  Forbid();
  if (task_exists(t) && t->tc_Node.ln_Type == NT_PROCESS && (tu = getuser(t)) != NULL)
    {
      struct Process *grp = tu->u_session ? (struct Process *)tu->u_session->pgrp : NULL;
      _psignalgrp(grp ? grp : (struct Process *)t, SIGWINCH);
    }
  Permit();
}
