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
 * memcmp.c,v
 *
 * Revision 1.2  2026/08/15  ChatGPT modifications (JJ)
 *
 *    Add a short-input fast path.  Counts below eight bytes now bypass the
 *    alignment and longword-path setup and go directly to the byte loop.
 *    Keep the existing aligned longword implementation for counts of eight
 *    bytes or more.  This targets the measured short-comparison regression
 *    without discarding the measured gains at 15 bytes and above.
 * 
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Replace the C byte loop with a 68000-compatible assembler
 *    implementation.  Use an aligned longword fast path when both input
 *    pointers have compatible alignment, with a byte fallback and tail for
 *    arbitrary addresses and lengths.  On a differing longword, locate the
 *    first differing byte and preserve the original unsigned-byte result.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)memcmp.c	5.6 (Berkeley) 1/26/91";
#endif /* LIBC_SCCS and not lint */

#include "defs.h"

ENTRY(memcmp)
asm("
	movl	sp@(4),a0	/* first memory block */
	movl	sp@(8),a1	/* second memory block */
	movl	sp@(12),d0	/* byte count */
	beq	memcmpequal

	/* Avoid alignment/longword setup for very short comparisons. */
	cmpl	#8,d0
	bcs	memcmp_byteloop

	/* Longword accesses on 68000 require even addresses. */
	movl	a0,d1
	btst	#0,d1
	beq	memcmp_a0_even

	/* a0 is odd: use longwords only if a1 is also odd. */
	movl	a1,d1
	btst	#0,d1
	beq	memcmp_byteloop

	/* Both are odd; compare one byte to make both even. */
	movb	a0@+,d1
	cmpb	a1@+,d1
	bne	memcmp_byte_diff
	subql	#1,d0
	beq	memcmpequal
	bra	memcmp_longcheck

memcmp_a0_even:
	/* a0 is even; a1 must also be even for the longword path. */
	movl	a1,d1
	btst	#0,d1
	bne	memcmp_byteloop

memcmp_longcheck:
	cmpl	#4,d0
	bcs	memcmp_tail

memcmp_longloop:
	movl	a0@+,d1
	cmpl	a1@+,d1
	bne	memcmp_long_diff
	subql	#4,d0
	beq	memcmpequal
	cmpl	#4,d0
	bcc	memcmp_longloop

memcmp_tail:
	/* One to three bytes remain. */
	movb	a0@+,d1
	cmpb	a1@+,d1
	bne	memcmp_byte_diff
	subql	#1,d0
	bne	memcmp_tail
	bra	memcmpequal

memcmp_byteloop:
	/* Different pointer parity: byte-safe fallback for the full region. */
	movb	a0@+,d1
	cmpb	a1@+,d1
	bne	memcmp_byte_diff
	subql	#1,d0
	bne	memcmp_byteloop

memcmpequal:
	moveq	#0,d0
	rts

memcmp_byte_diff:
	/* d1 holds byte from s1; a1 points one byte past byte from s2. */
	andl	#255,d1
	moveq	#0,d0
	movb	a1@-,d0
	subl	d0,d1
	movl	d1,d0
	rts

memcmp_long_diff:
	/* Recheck the differing longword byte by byte to preserve memcmp order. */
	subql	#4,a0
	subql	#4,a1
	moveq	#0,d0
	moveq	#0,d1

	movb	a0@+,d1
	movb	a1@+,d0
	cmpb	d0,d1
	bne	memcmp_return_diff

	movb	a0@+,d1
	movb	a1@+,d0
	cmpb	d0,d1
	bne	memcmp_return_diff

	movb	a0@+,d1
	movb	a1@+,d0
	cmpb	d0,d1
	bne	memcmp_return_diff

	movb	a0@+,d1
	movb	a1@+,d0

memcmp_return_diff:
	subl	d0,d1
	movl	d1,d0
	rts
");
