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

/*
 * A handler that answers a vtcon packet with ERROR_ACTION_NOT_KNOWN (an
 * older vtcon, or a console type that has no use for it: XCON: for the
 * window-size packet) is asked once. The refusal is remembered per handler
 * and packet, and the packet is not sent again: ACTION_VTCON_SWINSZ on every
 * TIOCSWINSZ and ACTION_VTCON_INTR on every interrupted read cost a packet
 * round trip each. Only the optional packets are remembered: the TCGETA
 * probe that finds out whether a console is vtcon's is itself such a refusal
 * and is kept in f_ttyflags.
 */
#define NREFUSED 8
static struct { struct MsgPort *port; long action; } refused[NREFUSED];

int __vtcon_refused(struct file *f, long action)
{
  int i;

  for (i = 0; i < NREFUSED; i++)
    if (refused[i].port == f->f_fh->fh_Type && refused[i].action == action)
      return 1;
  return 0;
}

void __vtcon_note_reply(struct file *f, long action, long res1, long res2)
{
  int i;

  if (res1 || res2 != ERROR_ACTION_NOT_KNOWN || __vtcon_refused (f, action))
    return;
  Forbid ();
  for (i = 0; i < NREFUSED; i++)
    if (!refused[i].port)
      {
        refused[i].action = action;
        refused[i].port = f->f_fh->fh_Type;
        break;
      }
  Permit ();
}

int __vtcon_packet(struct file *f, long action, void *arg, long arg3)
{
  usetup;
  int optional = action == ACTION_VTCON_SWINSZ;

  if (optional && __vtcon_refused (f, action))
    return 0;
  LastResult (f) = 0; LastError (f) = 0;
  SendPacket3 (f, __srwport, action, f->f_fh->fh_Arg1, (long)arg, arg3);
  __wait_sync_packet (&f->f_sp);
  if (optional)
    __vtcon_note_reply (f, action, LastResult (f), LastError (f));
  return LastResult (f) != 0;
}

/* FIONREAD on a vtcon console, PTY: or a PTY: pipe: the bytes a read would
   get now, or -1 when the handler does not know the packet (an older vtcon,
   PIPE:; asked once per handler) */
long __vtcon_nread(struct file *f)
{
  usetup;

  if (__vtcon_refused (f, ACTION_VTCON_NREAD))
    return -1;
  LastResult (f) = 0; LastError (f) = 0;
  SendPacket3 (f, __srwport, ACTION_VTCON_NREAD, f->f_fh->fh_Arg1, 0, 0);
  __wait_sync_packet (&f->f_sp);
  __vtcon_note_reply (f, ACTION_VTCON_NREAD, LastResult (f), LastError (f));
  return LastError (f) ? -1 : LastResult (f);
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

/* A process of a background group reads its terminal, or writes it with
 * TOSTOP set: SIGTTIN / SIGTTOU for its group, which stops it (BSD
 * ttread/ttwrite). The stop is taken when the task is switched in again
 * (machdep.c's launch code), so it yields a tick; after SIGCONT it looks
 * again, as it may still be in the background (bg). Returns 0 to go on
 * with the I/O, -1 (errno EIO) for a read whose signal is ignored or
 * blocked; such a write goes through, as on BSD. Call before the I/O
 * masks signals. */
int __vtcon_bg(struct file *f, int sig)
{
  usetup;

  for (;;)
    {
      if (!u.u_session || !u.u_session->pgrp || u.p_pgrp == u.u_session->pgrp)
        return 0;   /* no job control, or in the foreground */
      if (sig == SIGTTOU)
        {
          struct termios t;

          if (!__vtcon_packet (f, ACTION_VTCON_TCGETA, &t, 0) || !(t.c_lflag & TOSTOP))
            return 0;
        }
      if (((u.p_sigignore | u.p_sigmask) & sigmask(sig)) || (u.p_flag & SVFORK))
        {
          if (sig == SIGTTOU)
            return 0;
          errno = EIO;
          return -1;
        }
      _psignalgrp ((struct Process *)FindTask (0), sig);
      Delay (1);
    }
}

/* At program start: is the program on a vtcon terminal? Asked of fds 0-2
 * now, not at the first tty I/O: until u_vtcon is set, a sleep does not
 * wake for the ^Z and ^\ breaks, and a program sleeping before its first
 * read stopped only when the sleep ended (rig: ^Z to "sleep 2; read"
 * took 2 s, and what was typed meanwhile went to the job). */
void __vtcon_init(void)
{
  usetup;
  int fd;

  for (fd = 0; fd < 3; fd++)
    {
      struct file *f = u.u_ofile[fd];

      if (f && f->f_type == DTYPE_FILE && f->f_fh)
        __vtcon (f);
    }
}
