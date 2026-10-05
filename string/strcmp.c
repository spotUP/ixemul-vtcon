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
 * by Hiroshi Horimoto <horimoto@cs-aoi.cs.sist.ac.jp> and
 * by J.T. Conklin <jtc@NetBSD.org>.
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
 * strcmp.c,v
 *
 * Revision 1.2  2026/08/16  ChatGPT modifications (JJ)
 *
 *    Port the NetBSD m68k four-byte-unrolled comparison loop to ixemul.
 *    Use the condition codes from MOVB and SUBB directly, eliminating the
 *    separate TSTB used after each equal-byte comparison.
 *    Reconstruct the exact unsigned-char difference from the subtraction
 *    result and carry flag without re-reading the mismatching byte.
 *    Keep the implementation compatible with 68000 through 68060.
 *
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Unroll the equal-byte comparison loop four times using only
 *    68000-compatible instructions, reducing taken loop branches while
 *    preserving byte-exact access and stopping at the first NUL byte.
 *    Return mismatching bytes using unsigned-char strcmp semantics.
 */

ENTRY(strcmp)
asm("\n\
	movl	sp@(4),a0		/* string1 */\n\
	movl	sp@(8),a1		/* string2 */\n\
\n\
.Lstrcmp_loop:\n\
	movb	a0@+,d1\n\
	jeq	.Lstrcmp_zero\n\
	subb	a1@+,d1\n\
	jne	.Lstrcmp_diff\n\
\n\
	movb	a0@+,d1\n\
	jeq	.Lstrcmp_zero\n\
	subb	a1@+,d1\n\
	jne	.Lstrcmp_diff\n\
\n\
	movb	a0@+,d1\n\
	jeq	.Lstrcmp_zero\n\
	subb	a1@+,d1\n\
	jne	.Lstrcmp_diff\n\
\n\
	movb	a0@+,d1\n\
	jeq	.Lstrcmp_zero\n\
	subb	a1@+,d1\n\
	jeq	.Lstrcmp_loop\n\
\n\
.Lstrcmp_diff:\n\
	/*\n\
	 * SUBB leaves the low byte of the true unsigned-char difference\n\
	 * in d1 and sets carry on borrow.  Build the correct signed 32-bit\n\
	 * result without loading either string byte again.\n\
	 */\n\
	scs	d0\n\
	extw	d0\n\
	extl	d0\n\
	movb	d1,d0\n\
	rts\n\
\n\
.Lstrcmp_zero:\n\
	/*\n\
	 * string1 ended.  Return 0 - (unsigned char)*string2.\n\
	 * a1 has not yet been advanced for this byte.\n\
	 */\n\
	moveq	#0,d0\n\
	movb	a1@,d0\n\
	negl	d0\n\
	rts\n\
");
