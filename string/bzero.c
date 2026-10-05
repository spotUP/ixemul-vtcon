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

/* 1.1  2026/06/07  Copilot modifications (JJ)
 *      Add explicit zero-length check in bzero() to avoid stray write on
 *      odd-aligned destinations when count == 0.
 */

#include "defs.h"

/*
 * This is probably not the best we can do, but it is still much
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword loop (might not be good for a 68020)
 *	- longword, as opposed to word, align when possible (only on the 68020)
 *	- use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(bzero)
asm("\n\
	movl	sp@(4),a0	/* destination */\n\
	movl	sp@(8),d0	/* count */\n\
	tstl	d0			/* explicit zero-length check */\n\
	jeq		bzdone		/* avoid stray write on odd address */\n\
	movl	a0,d1\n\
	btst	#0,d1		/* address odd? */\n\
	jeq	bzeven		/* no, skip alignment */\n\
	clrb	a0@+		/* yes, clear a byte */\n\
	subql	#1,d0		/* adjust count */\n\
	jeq	bzdone		/* if zero, all done */\n\
bzeven:\n\
	movl	d0,d1\n\
	lsrl	#2,d1		/* convert to longword count */\n\
	jeq	bzbloop		/* no longwords, skip loop */\n\
bzlloop:\n\
	clrl	a0@+		/* clear a longword */\n\
	subql	#1,d1		/* adjust count */\n\
	jne	bzlloop		/* still more, keep going */\n\
	andl	#3,d0		/* what remains */\n\
	jeq	bzdone		/* nothing, all done */\n\
bzbloop:\n\
	clrb	a0@+		/* clear a byte */\n\
	subql	#1,d0		/* adjust count */\n\
	jne	bzbloop		/* still more, keep going */\n\
bzdone:\n\
	rts\n\
");
