/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* The vtcon console's packets (see __vtcon.c). The numbers and layouts are
 * vtcon's handler/vtcon_packets.h; struct termios and struct winsize are
 * sent as ixemul defines them. */
#ifndef __VTCON_H__
#define __VTCON_H__

#define ACTION_VTCON_TCGETA 0x7655  /* Arg2 struct termios * to fill */
#define ACTION_VTCON_TCSETA 0x7656  /* Arg2 struct termios *, Arg3 TCSANOW/DRAIN/FLUSH */
#define ACTION_VTCON_GWINSZ 0x7657  /* Arg2 struct winsize * to fill */
#define ACTION_VTCON_SWINSZ 0x7658  /* Arg2 struct winsize * (a pty master's) */
#define ACTION_VTCON_NREAD  0x765A  /* Res1: bytes a Read would get now */
#define ACTION_VTCON_INTR   0x765C  /* Arg2 a waiting ACTION_READ: answer it now, -1/ERROR_BREAK */

int __vtcon_packet(struct file *f, long action, void *arg, long arg3);
int __vtcon(struct file *f);
int __vtcon_refused(struct file *f, long action);
void __vtcon_note_reply(struct file *f, long action, long res1, long res2);
long __vtcon_nread(struct file *f);
unsigned long __vtcon_breaks(unsigned long breaks);
void __sleep_breaks(unsigned long res);
void __vtcon_winch(struct Task *t);
int __vtcon_bg(struct file *f, int sig);
void __vtcon_init(void);
int __vtcon_pty_mounted(void);

#endif
