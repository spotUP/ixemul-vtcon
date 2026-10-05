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
asm("
	movl	sp@(4),a0		/* string 1 */
	movl	sp@(8),a1		/* string 2 */
	movl	sp@(12),d1		/* length */
	tstl	d1
	jeq	.Lbc060_done

	/* Small blocks: avoid alignment setup. */
	cmpl	#8,d1
	jcs	.Lbc060_byte

	/* Word-align string 1.  68060 permits unaligned accesses to string 2. */
	movl	a0,d0
	btst	#0,d0
	jeq	.Lbc060_even
	cmpmb	a0@+,a1@+
	jne	.Lbc060_noteq
	subql	#1,d1
	addql	#1,d0

.Lbc060_even:
	/* Longword-align string 1. */
	btst	#1,d0
	jeq	.Lbc060_aligned
	cmpmw	a0@+,a1@+
	jne	.Lbc060_noteq
	subql	#2,d1

.Lbc060_aligned:
	/* Compare 32 bytes per main-loop iteration. */
	movl	d1,d0
	lsrl	#5,d0
	jeq	.Lbc060_long
	andl	#31,d1

.Lbc060_32loop:
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	subql	#1,d0
	jne	.Lbc060_32loop

.Lbc060_long:
	tstl	d1
	jeq	.Lbc060_done
	movl	d1,d0
	lsrl	#2,d0
	jeq	.Lbc060_byte
	andl	#3,d1

.Lbc060_longloop:
	cmpml	a0@+,a1@+
	jne	.Lbc060_noteq
	subql	#1,d0
	jne	.Lbc060_longloop

	tstl	d1
	jeq	.Lbc060_done

.Lbc060_byte:
	cmpmb	a0@+,a1@+
	jne	.Lbc060_noteq
	subql	#1,d1
	jne	.Lbc060_byte

.Lbc060_done:
	moveq	#0,d0
	rts

.Lbc060_noteq:
	moveq	#1,d0
	rts
");

#elif defined(__mc68040__)

/*
 * 68040 path.
 *
 * Use the same 32-byte NetBSD-derived data path as 68060, with a
 * full 32-bit SUBQ/BNE loop counter.
 */
ENTRY(bcmp)
asm("
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	sp@(12),d1
	tstl	d1
	jeq	.Lbc040_done

	cmpl	#8,d1
	jcs	.Lbc040_byte

	movl	a0,d0
	btst	#0,d0
	jeq	.Lbc040_even
	cmpmb	a0@+,a1@+
	jne	.Lbc040_noteq
	subql	#1,d1
	addql	#1,d0

.Lbc040_even:
	btst	#1,d0
	jeq	.Lbc040_aligned
	cmpmw	a0@+,a1@+
	jne	.Lbc040_noteq
	subql	#2,d1

.Lbc040_aligned:
	movl	d1,d0
	lsrl	#5,d0
	jeq	.Lbc040_long
	andl	#31,d1

.Lbc040_32loop:
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	subql	#1,d0
	jne	.Lbc040_32loop

.Lbc040_long:
	tstl	d1
	jeq	.Lbc040_done
	movl	d1,d0
	lsrl	#2,d0
	jeq	.Lbc040_byte
	andl	#3,d1

.Lbc040_longloop:
	cmpml	a0@+,a1@+
	jne	.Lbc040_noteq
	subql	#1,d0
	jne	.Lbc040_longloop

	tstl	d1
	jeq	.Lbc040_done

.Lbc040_byte:
	cmpmb	a0@+,a1@+
	jne	.Lbc040_noteq
	subql	#1,d1
	jne	.Lbc040_byte

.Lbc040_done:
	moveq	#0,d0
	rts

.Lbc040_noteq:
	moveq	#1,d0
	rts
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
asm("
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	sp@(12),d1
	tstl	d1
	jeq	.Lbc020_done

	cmpl	#8,d1
	jcs	.Lbc020_byte

	movl	a0,d0
	btst	#0,d0
	jeq	.Lbc020_even
	cmpmb	a0@+,a1@+
	jne	.Lbc020_noteq
	subql	#1,d1
	addql	#1,d0

.Lbc020_even:
	btst	#1,d0
	jeq	.Lbc020_aligned
	cmpmw	a0@+,a1@+
	jne	.Lbc020_noteq
	subql	#2,d1

.Lbc020_aligned:
	movl	d1,d0
	lsrl	#5,d0
	jeq	.Lbc020_long
	andl	#31,d1
	subql	#1,d0

.Lbc020_32loop:
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	dbf	d0,.Lbc020_32loop
	clrw	d0
	subql	#1,d0
	jcc	.Lbc020_32loop

.Lbc020_long:
	tstl	d1
	jeq	.Lbc020_done
	movl	d1,d0
	lsrl	#2,d0
	jeq	.Lbc020_byte
	subql	#1,d0

.Lbc020_longloop:
	cmpml	a0@+,a1@+
	jne	.Lbc020_noteq
	dbf	d0,.Lbc020_longloop
	clrw	d0
	subql	#1,d0
	jcc	.Lbc020_longloop

	andl	#3,d1
	jeq	.Lbc020_done

.Lbc020_byte:
	cmpmb	a0@+,a1@+
	jne	.Lbc020_noteq
	subql	#1,d1
	jne	.Lbc020_byte

.Lbc020_done:
	moveq	#0,d0
	rts

.Lbc020_noteq:
	moveq	#1,d0
	rts
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
asm("
	movl	sp@(4),a0	/* string 1 */
	movl	sp@(8),a1	/* string 2 */
	movl	sp@(12),d0	/* length */
	jeq	bcdone_bcmp	/* if zero, nothing to do */

	movl	a0,d1
	btst	#0,d1		/* string 1 address odd? */
	jeq	bceven		/* no, skip alignment */
	cmpmb	a0@+,a1@+	/* yes, compare a byte */
	jne	bcnoteq		/* not equal, return non-zero */
	subql	#1,d0		/* adjust count */
	jeq	bcdone_bcmp	/* count 0, return zero */

bceven:
	movl	a1,d1
	btst	#0,d1		/* string 2 address odd? */
	jne	bcbcheck		/* yes, compare bytes */

	/*
	 * Both addresses are even.  Compare 16 bytes per main-loop
	 * iteration, then handle remaining longwords and bytes.
	 */
	cmpl	#16,d0
	jcs	bclcheck

bcl16loop:
	cmpml	a0@+,a1@+
	jne	bcnoteq
	cmpml	a0@+,a1@+
	jne	bcnoteq
	cmpml	a0@+,a1@+
	jne	bcnoteq
	cmpml	a0@+,a1@+
	jne	bcnoteq
	subl	#16,d0
	jeq	bcdone_bcmp
	cmpl	#16,d0
	jcc	bcl16loop

bclcheck:
	cmpl	#4,d0
	jcs	bcbtail

bcltail:
	cmpml	a0@+,a1@+
	jne	bcnoteq
	subql	#4,d0
	jeq	bcdone_bcmp
	cmpl	#4,d0
	jcc	bcltail
	bra	bcbtail

	/*
	 * Different address parity: longword accesses cannot be used safely
	 * on 68000, so compare four bytes per main-loop iteration.
	 */
bcbcheck:
	cmpl	#4,d0
	jcs	bcbtail

bcb4loop:
	cmpmb	a0@+,a1@+
	jne	bcnoteq
	cmpmb	a0@+,a1@+
	jne	bcnoteq
	cmpmb	a0@+,a1@+
	jne	bcnoteq
	cmpmb	a0@+,a1@+
	jne	bcnoteq
	subql	#4,d0
	jeq	bcdone_bcmp
	cmpl	#4,d0
	jcc	bcb4loop

bcbtail:
	tstl	d0
	jeq	bcdone_bcmp

bcbloop:
	cmpmb	a0@+,a1@+
	jne	bcnoteq
	subql	#1,d0
	jne	bcbloop

bcdone_bcmp:
	rts

bcnoteq:
	moveq	#1,d0
	rts
");

#endif
