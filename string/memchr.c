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
 * memchr.c,v
 *
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Replace the C byte loop with a 68000-compatible assembler
 *    implementation.  Search four bytes per main-loop iteration to reduce
 *    count-update and loop-branch overhead while preserving the exact length
 *    bound, unsigned-byte comparison semantics, and standard return value.
 */

#include "defs.h"

ENTRY(memchr)
asm("
	movl	sp@(4),a0	/* memory block */
	movb	sp@(11),d0	/* byte to look for */
	movl	sp@(12),d1	/* byte count */
	jeq	memchrnotfound

memchrloop:
	cmpl	#4,d1
	jcs	memchrtail

	cmpb	a0@+,d0		/* byte 1 */
	jeq	memchrfound
	cmpb	a0@+,d0		/* byte 2 */
	jeq	memchrfound
	cmpb	a0@+,d0		/* byte 3 */
	jeq	memchrfound
	cmpb	a0@+,d0		/* byte 4 */
	jeq	memchrfound

	subql	#4,d1
	jne	memchrloop
	bra	memchrnotfound

memchrtail:
	tstl	d1
	jeq	memchrnotfound
memchrtail_loop:
	cmpb	a0@+,d0
	jeq	memchrfound
	subql	#1,d1
	jne	memchrtail_loop

memchrnotfound:
	moveq	#0,d0
	rts

memchrfound:
	subql	#1,a0		/* a0 was post-incremented */
	movl	a0,d0
	rts
");
