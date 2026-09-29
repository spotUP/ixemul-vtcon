/* The vtcon console's packets (see __vtcon.c). The numbers and layouts are
 * vtcon's handler/vtcon_packets.h; struct termios and struct winsize are
 * sent as ixemul defines them. */
#ifndef __VTCON_H__
#define __VTCON_H__

#define ACTION_VTCON_TCGETA 0x7655  /* Arg2 struct termios * to fill */
#define ACTION_VTCON_TCSETA 0x7656  /* Arg2 struct termios *, Arg3 TCSANOW/DRAIN/FLUSH */
#define ACTION_VTCON_GWINSZ 0x7657  /* Arg2 struct winsize * to fill */
#define ACTION_VTCON_SWINSZ 0x7658  /* Arg2 struct winsize * (a pty master's) */

int __vtcon_packet(struct file *f, long action, void *arg, long arg3);
int __vtcon(struct file *f);
unsigned long __vtcon_breaks(unsigned long breaks);
void __vtcon_winch(struct Task *t);

#endif
