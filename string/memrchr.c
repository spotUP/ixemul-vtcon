/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * memrchr.c,v
 *
 * Revision 1.1  2026/09/10  ChatGPT modifications (JJ)
 *
 *    Add a 68000-compatible assembler implementation of memrchr().
 *    Search four bytes per main-loop iteration from the end of the memory
 *    block while preserving the exact length bound, unsigned-byte comparison
 *    semantics, and standard return value.
 */

#include "defs.h"

ENTRY(memrchr)
asm("\n\
	movl	sp@(4),a0	/* memory block */\n\
	movb	sp@(11),d0	/* byte to look for */\n\
	movl	sp@(12),d1	/* byte count */\n\
	jeq	memrchrnotfound\n\
\n\
	addl	d1,a0		/* one byte past the search area */\n\
\n\
memrchrloop:\n\
	cmpl	#4,d1\n\
	jcs	memrchrtail\n\
\n\
	cmpb	a0@-,d0		/* byte 4 */\n\
	jeq	memrchrfound\n\
	cmpb	a0@-,d0		/* byte 3 */\n\
	jeq	memrchrfound\n\
	cmpb	a0@-,d0		/* byte 2 */\n\
	jeq	memrchrfound\n\
	cmpb	a0@-,d0		/* byte 1 */\n\
	jeq	memrchrfound\n\
\n\
	subql	#4,d1\n\
	jne	memrchrloop\n\
	bra	memrchrnotfound\n\
\n\
memrchrtail:\n\
	tstl	d1\n\
	jeq	memrchrnotfound\n\
memrchrtail_loop:\n\
	cmpb	a0@-,d0\n\
	jeq	memrchrfound\n\
	subql	#1,d1\n\
	jne	memrchrtail_loop\n\
\n\
memrchrnotfound:\n\
	moveq	#0,d0\n\
	rts\n\
\n\
memrchrfound:\n\
	movl	a0,d0		/* a0 was pre-decremented */\n\
	rts\n\
");
