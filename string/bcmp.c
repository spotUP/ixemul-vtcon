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

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "defs.h"

/*
 * bcmp.c,v
 *
 * Revision 1.3  2026/08/25  ChatGPT modifications (JJ)
 *
 *    Prevent the optimized 68020/030/040/060 paths from entering the byte
 *    tail with a zero remainder after a completed 32-byte block.
 *
 * Revision 1.2  2026/08/16  ChatGPT modifications (JJ)
 *
 *    Add CPU-specific m68k bcmp() implementations.
 *
 *    68060 and 68040 use the NetBSD m68k alignment strategy and compare
 *    32 bytes per main-loop iteration with eight longword comparisons,
 *    using a full 32-bit SUBQ/BNE loop counter.
 *
 *    68020/68030 use the NetBSD m68k alignment strategy and 32-byte
 *    eight-longword main loop with DBF-based loop handling.
 *
 *    68000/68010 retain the previous ixemul implementation, including
 *    its parity-safe byte fallback and full 32-bit byte-loop counters.
 *
 *    Use unsigned length comparisons in all optimized paths.
 *
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Unroll the aligned longword comparison loop four times, comparing
 *    16 bytes per main-loop iteration, and unroll the byte fallback four
 *    times for differently aligned inputs.  Preserve the original 68000-safe
 *    alignment handling and zero/non-zero bcmp() return semantics.
 */

/* bcmp(s1, s2, n) */

#if defined(__mc68060__)

/*
 * 68060 path.
 *
 * Use NetBSD's 4-byte alignment strategy and an eight-longword,
 * 32-byte main loop.  Keep the loop counter fully 32-bit.
 */
ENTRY(bcmp)
asm("\n\
	movl	sp@(4),a0		/* string 1 */\n\
	movl	sp@(8),a1		/* string 2 */\n\
	movl	sp@(12),d1		/* length */\n\
	tstl	d1\n\
	jeq	.Lbc060_done\n\
\n\
	/* Small blocks: avoid alignment setup. */\n\
	cmpl	#8,d1\n\
	jcs	.Lbc060_byte\n\
\n\
	/* Word-align string 1.  68060 permits unaligned accesses to string 2. */\n\
	movl	a0,d0\n\
	btst	#0,d0\n\
	jeq	.Lbc060_even\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	subql	#1,d1\n\
	addql	#1,d0\n\
\n\
.Lbc060_even:\n\
	/* Longword-align string 1. */\n\
	btst	#1,d0\n\
	jeq	.Lbc060_aligned\n\
	cmpmw	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	subql	#2,d1\n\
\n\
.Lbc060_aligned:\n\
	/* Compare 32 bytes per main-loop iteration. */\n\
	movl	d1,d0\n\
	lsrl	#5,d0\n\
	jeq	.Lbc060_long\n\
	andl	#31,d1\n\
\n\
.Lbc060_32loop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	subql	#1,d0\n\
	jne	.Lbc060_32loop\n\
\n\
.Lbc060_long:\n\
	tstl	d1\n\
	jeq	.Lbc060_done\n\
	movl	d1,d0\n\
	lsrl	#2,d0\n\
	jeq	.Lbc060_byte\n\
	andl	#3,d1\n\
\n\
.Lbc060_longloop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	subql	#1,d0\n\
	jne	.Lbc060_longloop\n\
\n\
	tstl	d1\n\
	jeq	.Lbc060_done\n\
\n\
.Lbc060_byte:\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc060_noteq\n\
	subql	#1,d1\n\
	jne	.Lbc060_byte\n\
\n\
.Lbc060_done:\n\
	moveq	#0,d0\n\
	rts\n\
\n\
.Lbc060_noteq:\n\
	moveq	#1,d0\n\
	rts\n\
");

#elif defined(__mc68040__)

/*
 * 68040 path.
 *
 * Use the same 32-byte NetBSD-derived data path as 68060, with a
 * full 32-bit SUBQ/BNE loop counter.
 */
ENTRY(bcmp)
asm("\n\
	movl	sp@(4),a0\n\
	movl	sp@(8),a1\n\
	movl	sp@(12),d1\n\
	tstl	d1\n\
	jeq	.Lbc040_done\n\
\n\
	cmpl	#8,d1\n\
	jcs	.Lbc040_byte\n\
\n\
	movl	a0,d0\n\
	btst	#0,d0\n\
	jeq	.Lbc040_even\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	subql	#1,d1\n\
	addql	#1,d0\n\
\n\
.Lbc040_even:\n\
	btst	#1,d0\n\
	jeq	.Lbc040_aligned\n\
	cmpmw	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	subql	#2,d1\n\
\n\
.Lbc040_aligned:\n\
	movl	d1,d0\n\
	lsrl	#5,d0\n\
	jeq	.Lbc040_long\n\
	andl	#31,d1\n\
\n\
.Lbc040_32loop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	subql	#1,d0\n\
	jne	.Lbc040_32loop\n\
\n\
.Lbc040_long:\n\
	tstl	d1\n\
	jeq	.Lbc040_done\n\
	movl	d1,d0\n\
	lsrl	#2,d0\n\
	jeq	.Lbc040_byte\n\
	andl	#3,d1\n\
\n\
.Lbc040_longloop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	subql	#1,d0\n\
	jne	.Lbc040_longloop\n\
\n\
	tstl	d1\n\
	jeq	.Lbc040_done\n\
\n\
.Lbc040_byte:\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc040_noteq\n\
	subql	#1,d1\n\
	jne	.Lbc040_byte\n\
\n\
.Lbc040_done:\n\
	moveq	#0,d0\n\
	rts\n\
\n\
.Lbc040_noteq:\n\
	moveq	#1,d0\n\
	rts\n\
");

#elif defined(__mc68020__) || defined(__mc68030__)

/*
 * 68020/68030 path.
 *
 * Closely follow the NetBSD m68k algorithm: align string 1 to four
 * bytes, compare 32 bytes per main iteration, and use DBF for the
 * long-running unrolled loop.  Byte tails use a full 32-bit count.
 */
ENTRY(bcmp)
asm("\n\
	movl	sp@(4),a0\n\
	movl	sp@(8),a1\n\
	movl	sp@(12),d1\n\
	tstl	d1\n\
	jeq	.Lbc020_done\n\
\n\
	cmpl	#8,d1\n\
	jcs	.Lbc020_byte\n\
\n\
	movl	a0,d0\n\
	btst	#0,d0\n\
	jeq	.Lbc020_even\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	subql	#1,d1\n\
	addql	#1,d0\n\
\n\
.Lbc020_even:\n\
	btst	#1,d0\n\
	jeq	.Lbc020_aligned\n\
	cmpmw	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	subql	#2,d1\n\
\n\
.Lbc020_aligned:\n\
	movl	d1,d0\n\
	lsrl	#5,d0\n\
	jeq	.Lbc020_long\n\
	andl	#31,d1\n\
	subql	#1,d0\n\
\n\
.Lbc020_32loop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	dbf	d0,.Lbc020_32loop\n\
	clrw	d0\n\
	subql	#1,d0\n\
	jcc	.Lbc020_32loop\n\
\n\
.Lbc020_long:\n\
	tstl	d1\n\
	jeq	.Lbc020_done\n\
	movl	d1,d0\n\
	lsrl	#2,d0\n\
	jeq	.Lbc020_byte\n\
	subql	#1,d0\n\
\n\
.Lbc020_longloop:\n\
	cmpml	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	dbf	d0,.Lbc020_longloop\n\
	clrw	d0\n\
	subql	#1,d0\n\
	jcc	.Lbc020_longloop\n\
\n\
	andl	#3,d1\n\
	jeq	.Lbc020_done\n\
\n\
.Lbc020_byte:\n\
	cmpmb	a0@+,a1@+\n\
	jne	.Lbc020_noteq\n\
	subql	#1,d1\n\
	jne	.Lbc020_byte\n\
\n\
.Lbc020_done:\n\
	moveq	#0,d0\n\
	rts\n\
\n\
.Lbc020_noteq:\n\
	moveq	#1,d0\n\
	rts\n\
");

#else

/*
 * 68000/68010/generic path.
 *
 * Retain the previous ixemul implementation unchanged in substance.
 * Differently aligned inputs stay on the byte path so word/longword
 * accesses remain safe on the oldest supported processors.
 */
ENTRY(bcmp)
asm("\n\
	movl	sp@(4),a0	/* string 1 */\n\
	movl	sp@(8),a1	/* string 2 */\n\
	movl	sp@(12),d0	/* length */\n\
	jeq	bcdone_bcmp	/* if zero, nothing to do */\n\
\n\
	movl	a0,d1\n\
	btst	#0,d1		/* string 1 address odd? */\n\
	jeq	bceven		/* no, skip alignment */\n\
	cmpmb	a0@+,a1@+	/* yes, compare a byte */\n\
	jne	bcnoteq		/* not equal, return non-zero */\n\
	subql	#1,d0		/* adjust count */\n\
	jeq	bcdone_bcmp	/* count 0, return zero */\n\
\n\
bceven:\n\
	movl	a1,d1\n\
	btst	#0,d1		/* string 2 address odd? */\n\
	jne	bcbcheck		/* yes, compare bytes */\n\
\n\
	/*\n\
	 * Both addresses are even.  Compare 16 bytes per main-loop\n\
	 * iteration, then handle remaining longwords and bytes.\n\
	 */\n\
	cmpl	#16,d0\n\
	jcs	bclcheck\n\
\n\
bcl16loop:\n\
	cmpml	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpml	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpml	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpml	a0@+,a1@+\n\
	jne	bcnoteq\n\
	subl	#16,d0\n\
	jeq	bcdone_bcmp\n\
	cmpl	#16,d0\n\
	jcc	bcl16loop\n\
\n\
bclcheck:\n\
	cmpl	#4,d0\n\
	jcs	bcbtail\n\
\n\
bcltail:\n\
	cmpml	a0@+,a1@+\n\
	jne	bcnoteq\n\
	subql	#4,d0\n\
	jeq	bcdone_bcmp\n\
	cmpl	#4,d0\n\
	jcc	bcltail\n\
	bra	bcbtail\n\
\n\
	/*\n\
	 * Different address parity: longword accesses cannot be used safely\n\
	 * on 68000, so compare four bytes per main-loop iteration.\n\
	 */\n\
bcbcheck:\n\
	cmpl	#4,d0\n\
	jcs	bcbtail\n\
\n\
bcb4loop:\n\
	cmpmb	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpmb	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpmb	a0@+,a1@+\n\
	jne	bcnoteq\n\
	cmpmb	a0@+,a1@+\n\
	jne	bcnoteq\n\
	subql	#4,d0\n\
	jeq	bcdone_bcmp\n\
	cmpl	#4,d0\n\
	jcc	bcb4loop\n\
\n\
bcbtail:\n\
	tstl	d0\n\
	jeq	bcdone_bcmp\n\
\n\
bcbloop:\n\
	cmpmb	a0@+,a1@+\n\
	jne	bcnoteq\n\
	subql	#1,d0\n\
	jne	bcbloop\n\
\n\
bcdone_bcmp:\n\
	rts\n\
\n\
bcnoteq:\n\
	moveq	#1,d0\n\
	rts\n\
");

#endif
