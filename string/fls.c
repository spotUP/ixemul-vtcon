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
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * $Id: fls.c,v 1.2 2026/07/20 jj Exp $
 *
 * Register-only m68k implementation with help from ChatGPT:
 *  - compatible with 68000 and later processors
 *  - no static data references
 *  - returns the 1-based index of the highest set bit
 *  - returns 0 when value is 0
 */

#include "defs.h"

/* bit = fls(value) */

ENTRY(fls)
asm("\n\
	moveq	#32,d0\n\
	movl	sp@(4),d1\n\
	beq	fls_zero\n\
fls_again:\n\
	subql	#1,d0\n\
	btst	d0,d1\n\
	beq	fls_again\n\
	addql	#1,d0\n\
	rts\n\
fls_zero:\n\
	moveq	#0,d0\n\
	rts\n\
");

