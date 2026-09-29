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
  if (!(f->f_ttyflags & IXTTY_VTCON_KNOWN))
    {
      struct termios t;

      f->f_ttyflags |= IXTTY_VTCON_KNOWN;
      if (IsInteractive (CTOBPTR (f->f_fh)) &&
          __vtcon_packet (f, ACTION_VTCON_TCGETA, &t, 0))
        f->f_ttyflags |= IXTTY_VTCON;
    }
  return (f->f_ttyflags & IXTTY_VTCON) != 0;
}
