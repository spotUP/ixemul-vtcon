/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "defs.h"

/*
 * strncpy.c,v
 *
 * Revision 1.1  2026/08/16  ChatGPT modifications (JJ)
 *
 *    Replace the byte-copy loop with a DBEQ-based m68k loop that combines
 *    NUL detection with low-word count control.
 *    Replace byte-padding loop control with DBRA and extend both loops
 *    correctly across the full 32-bit size_t range.
 */

ENTRY(strncpy)
asm("\n\
	movl	sp@(4),d0		/* return destination */\n\
	movl	sp@(12),d1		/* count */\n\
	jeq	.Lsn_done\n\
	movl	sp@(8),a0		/* source */\n\
	movl	d0,a1			/* destination */\n\
\n\
	/*\n\
	 * Bias the count for DBEQ.  Each DBEQ pass handles one 16-bit\n\
	 * count block while preserving the upper half of d1.\n\
	 */\n\
	subql	#1,d1\n\
\n\
.Lsn_copy:\n\
	movb	a0@+,a1@+\n\
	dbeq	d1,.Lsn_copy\n\
	jeq	.Lsn_pad_entry\n\
\n\
	/*\n\
	 * DBEQ exhausted the low word without copying NUL.\n\
	 * Advance to the next 16-bit block if the 32-bit count remains.\n\
	 */\n\
	clrw	d1\n\
	subql	#1,d1\n\
	jcc	.Lsn_copy\n\
	rts\n\
\n\
.Lsn_pad_entry:\n\
	/*\n\
	 * The terminating NUL has already consumed one byte of the count.\n\
	 */\n\
	subql	#1,d1\n\
	jcs	.Lsn_done\n\
\n\
.Lsn_pad:\n\
	clrb	a1@+\n\
	dbra	d1,.Lsn_pad\n\
\n\
	/*\n\
	 * Continue padding when a higher 16-bit count block remains.\n\
	 */\n\
	clrw	d1\n\
	subql	#1,d1\n\
	jcc	.Lsn_pad\n\
\n\
.Lsn_done:\n\
	rts\n\
");
