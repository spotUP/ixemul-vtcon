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

/*
 * Revision 1.2  2026/06/23  ChatGPT modifications (JJ)
 *
 *   Restored the original bcopy() len <= 0 no-op guard in all
 *   CPU-specific implementations.  This preserves the historical
 *   behavior for zero or negative counts while leaving the copy loops
 *   and CPU dispatch unchanged.
 *
 * Revision 1.1  2026/06/23  ChatGPT modifications (JJ)
 *
 *   Added compile-time selection of CPU-specific bcopy()
 *   implementations for 68000/68010, 68020/68030, 68040 and 68060.
 *
 *   Preserved the original alignment-safe 68000/68010 implementation.
 *   Added 68020/68030 and 68040 paths using four-longword unrolled
 *   copy loops, with an alignment pre-pass when the source and
 *   destination have matching alignment.
 *
 *   Added a 68060 path using eight-longword unrolled copy loops without
 *   an alignment pre-pass.
 *
 *   All implementations preserve overlap-safe forward and backward
 *   copying and use only caller-saved registers.
 */

/*
 * CPU-selected bcopy for ixemul.
 *
 * Selection is done at compile time from the CPU macros emitted by gcc
 * for the selected -m680xx option.
 *
 * 68000/010:
 *      original 68000-safe routine; avoids odd longword accesses.
 *
 * 68020/030:
 *      020+ routine; permits unaligned longword accesses, but aligns
 *      when src/dst share modulo-4 alignment; 4-longword unroll.
 *
 * 68040:
 *      040 routine; permits unaligned longword accesses, but aligns
 *      when src/dst share modulo-4 alignment; 4-longword unroll.
 *
 * 68060:
 *      060 routine; no byte-alignment pre-pass; 8-longword unroll.
 *
 * All variants:
 *      - overlap-safe, like memmove
 *      - conservatively copy backward if src < dst
 *      - use only caller-saved registers: d0/d1/a0/a1
 */

#include "defs.h"

/*
 * CPU selection.
 *
 * Common gcc/m68k defines include forms such as:
 *      __mc68020__, mc68020
 *      __mc68030__, mc68030
 *      __mc68040__, mc68040
 *      __mc68060__, mc68060
 *
 * Order matters: test 68060 and 68040 before 68020/030.
 */

#if defined(__mc68060__) || defined(mc68060) || \
    defined(__m68060__)  || defined(m68060)
#define BCOPY_CPU_060 1

#elif defined(__mc68040__) || defined(mc68040) || \
      defined(__m68040__)  || defined(m68040)
#define BCOPY_CPU_040 1

#elif defined(__mc68030__) || defined(mc68030) || \
      defined(__m68030__)  || defined(m68030)  || \
      defined(__mc68020__) || defined(mc68020) || \
      defined(__m68020__)  || defined(m68020)
#define BCOPY_CPU_020 1

#else
#define BCOPY_CPU_000 1
#endif

/*
 * void bcopy(const void *src, void *dst, size_t len);
 *
 * stack layout:
 *   sp+4  = src
 *   sp+8  = dst
 *   sp+12 = len
 */

#if BCOPY_CPU_060

/*
 * 68060-optimized bcopy.
 *
 * Notes:
 *      - 68060-only
 *      - intentionally does not byte-align before longword copies
 *      - uses 8-longword unrolled loops for large blocks
 *      - uses only caller-saved registers: d0/d1/a0/a1
 */

ENTRY(bcopy)
asm("
        movl    sp@(12),d1           /* length */
        tstl    d1
        jle     .done                /* len <= 0 -> nothing to do */

        movl    sp@(4),a0            /* src */
        movl    sp@(8),a1            /* dst */

        cmpl    a1,a0
        blo     .backward            /* copy backwards if src < dst */

/* ------------------------------------------------------------------ */
/* Forward copy                                                       */
/* ------------------------------------------------------------------ */

.forward:
        cmpl    #32,d1               /* small block */
        blo     .f_small

.f_lw_entry:
        movl    d1,d0
        lsrl    #5,d0                /* 32-byte / 8-longword blocks */
        beq     .f_lw_rem

.f_lw_unroll:
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        subql   #1,d0
        bne     .f_lw_unroll

.f_lw_rem:
        /*
         * d1 still holds the byte count for this phase.
         * Remaining longwords after 32-byte / 8-longword blocks:
         *     (d1 >> 2) & 7
         * d1 is not decremented by the 32-byte block loop.
         */

        movl    d1,d0
        lsrl    #2,d0                /* longword count */
        andl    #7,d0
        beq     .f_tail

.f_lw_loop:
        movl    a0@+,a1@+
        subql   #1,d0
        bne     .f_lw_loop

.f_tail:
        andl    #3,d1                /* remaining bytes */
        beq     .done

.f_b_loop:
        movb    a0@+,a1@+
        subql   #1,d1
        bne     .f_b_loop
        bra     .done

.f_small:
.f_small_loop:
        movb    a0@+,a1@+
        subql   #1,d1
        bne     .f_small_loop
        bra     .done

/* ------------------------------------------------------------------ */
/* Backward copy                                                      */
/* ------------------------------------------------------------------ */

.backward:
        addl    d1,a0                /* src end */
        addl    d1,a1                /* dst end */

        cmpl    #32,d1
        blo     .b_small

.b_lw_entry:
        movl    d1,d0
        lsrl    #5,d0                /* 32-byte / 8-longword blocks */
        beq     .b_lw_rem

.b_lw_unroll:
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        subql   #1,d0
        bne     .b_lw_unroll

.b_lw_rem:
        /*
         * d1 still holds the byte count for this phase.
         * Remaining backward longwords after 32-byte / 8-longword blocks:
         *     (d1 >> 2) & 7
         * d1 is not decremented by the 32-byte block loop.
         */

        movl    d1,d0
        lsrl    #2,d0                /* longword count */
        andl    #7,d0
        beq     .b_tail

.b_lw_loop:
        movl    a0@-,a1@-
        subql   #1,d0
        bne     .b_lw_loop

.b_tail:
        andl    #3,d1
        beq     .done

.b_b_loop:
        movb    a0@-,a1@-
        subql   #1,d1
        bne     .b_b_loop
        bra     .done

.b_small:
.b_small_loop:
        movb    a0@-,a1@-
        subql   #1,d1
        bne     .b_small_loop

.done:
        rts
");

#elif BCOPY_CPU_040

/*
 * 68040-optimized bcopy.
 *
 * Notes:
 *      - 68040-only
 *      - permits unaligned longword accesses
 *      - aligns when src and dst share modulo-4 alignment
 *      - byte loop for small blocks
 *      - 4-longword unrolled loops for larger blocks
 *      - uses only caller-saved registers: d0/d1/a0/a1
 */

ENTRY(bcopy)
asm("
        movl    sp@(12),d1          /* d1 = len */
        tstl    d1
        jle     bcdone_bcopy_040    /* len <= 0 -> nothing to do */

        movl    sp@(4),a0           /* a0 = src */
        movl    sp@(8),a1           /* a1 = dst */

        cmpl    a1,a0
        blo     bcback_040          /* src < dst -> copy backwards */

/* ------------------------------------------------------------------ */
/* Forward copy: src >= dst                                           */
/* ------------------------------------------------------------------ */

bcforw_040:
        cmpl    #16,d1              /* small block */
        blo     bcf_small_040

        /*
         * Try to get 4-byte alignment when src and dst share alignment.
         * If (a0 - a1) & 3 != 0, skip alignment and still use
         * longword copy. 040 tolerates unaligned longwords.
         */
        movl    a0,d0
        subl    a1,d0
        andl    #3,d0
        bne     bcf_noalign_040     /* different alignment -> no pre-align */

        /* src and dst share low 2 bits, align dst and thus src to 4 */
        movl    a1,d0
        andl    #3,d0
        beq     bcf_lw_entry_040    /* already 4-byte aligned */

bcf_align_loop_040:
        movb    a0@+,a1@+           /* copy byte until dst 4-byte aligned */
        subql   #1,d1
        beq     bcdone_bcopy_040
        movl    a1,d0
        andl    #3,d0
        bne     bcf_align_loop_040
        bra     bcf_lw_entry_040

bcf_noalign_040:
        /* No shared alignment: still do longwords, but without pre-align. */

bcf_lw_entry_040:
        movl    d1,d0
        lsrl    #4,d0               /* 16-byte / 4-longword blocks */
        beq     bcf_lw_rem_040

bcf_lw_unroll_040:
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        subql   #1,d0
        bne     bcf_lw_unroll_040

bcf_lw_rem_040:
        /*
         * d1 still holds the original byte count.
         * It is not decremented by the 16-byte block loop.
         * Remaining forward longwords after 16-byte / 4-longword blocks:
         *     (d1 >> 2) & 3
         * Tail bytes are later computed as d1 & 3.
         */
        movl    d1,d0
        lsrl    #2,d0               /* d0 = len / 4 */
        andl    #3,d0               /* remaining longwords */
        beq     bcf_tail_040

bcf_lw_loop_040:
        movl    a0@+,a1@+
        subql   #1,d0
        bne     bcf_lw_loop_040

bcf_tail_040:
        /*
         * d1 still holds the original byte count.
         * Tail bytes are:
         *     d1 & 3
         */

        andl    #3,d1               /* remaining bytes */
        beq     bcdone_bcopy_040

bcf_b_loop_040:
        movb    a0@+,a1@+           /* copy byte */
        subql   #1,d1
        bne     bcf_b_loop_040
        bra     bcdone_bcopy_040

bcf_small_040:
        movb    a0@+,a1@+           /* small forward copy */
        subql   #1,d1
        bne     bcf_small_040
        bra     bcdone_bcopy_040

/* ------------------------------------------------------------------ */
/* Backward copy: src < dst                                           */
/* ------------------------------------------------------------------ */

bcback_040:
        addl    d1,a0               /* a0 = src + len */
        addl    d1,a1               /* a1 = dst + len */

        cmpl    #16,d1              /* small block */
        blo     bcb_small_040

        /*
         * Same alignment strategy as forward copy, but backwards.
         */
        movl    a0,d0
        subl    a1,d0
        andl    #3,d0
        bne     bcb_noalign_040     /* different alignment -> no pre-align */

        movl    a1,d0
        andl    #3,d0
        beq     bcb_lw_entry_040

bcb_align_loop_040:
        movb    a0@-,a1@-           /* copy byte until dst 4-byte aligned */
        subql   #1,d1
        beq     bcdone_bcopy_040
        movl    a1,d0
        andl    #3,d0
        bne     bcb_align_loop_040
        bra     bcb_lw_entry_040

bcb_noalign_040:

bcb_lw_entry_040:
        movl    d1,d0
        lsrl    #4,d0               /* 16-byte / 4-longword blocks */
        beq     bcb_lw_rem_040

bcb_lw_unroll_040:
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        subql   #1,d0
        bne     bcb_lw_unroll_040

bcb_lw_rem_040:
        /*
         * d1 still holds the original byte count.
         * It is not decremented by the 16-byte block loop.
         * Remaining backward longwords after 16-byte / 4-longword blocks:
         *     (d1 >> 2) & 3
         * Tail bytes are later computed as d1 & 3.
         */
        movl    d1,d0
        lsrl    #2,d0               /* d0 = len / 4 */
        andl    #3,d0               /* remaining longwords */
        beq     bcb_tail_040

bcb_lw_loop_040:
        movl    a0@-,a1@-
        subql   #1,d0
        bne     bcb_lw_loop_040

bcb_tail_040:
        /*
         * d1 still holds the original byte count.
         * Tail bytes are:
         *     d1 & 3
         */

        andl    #3,d1               /* remaining bytes */
        beq     bcdone_bcopy_040

bcb_b_loop_040:
        movb    a0@-,a1@-           /* copy byte backwards */
        subql   #1,d1
        bne     bcb_b_loop_040
        bra     bcdone_bcopy_040

bcb_small_040:
        movb    a0@-,a1@-           /* small backward copy */
        subql   #1,d1
        bne     bcb_small_040

bcdone_bcopy_040:
        rts
");

#elif BCOPY_CPU_020

/*
 * 68020/030-optimized bcopy.
 *
 * Notes:
 *      - 68020/030 baseline
 *      - also safe as generic 020+ fallback
 *      - permits unaligned longword accesses
 *      - aligns when src and dst share modulo-4 alignment
 *      - uses 4-longword unrolled loops
 *      - uses only caller-saved registers: d0/d1/a0/a1
 */

ENTRY(bcopy)
asm("
        movl    sp@(12),d1           /* d1 = len */
        tstl    d1
        jle     bcdone_bcopy_020    /* len <= 0 -> nothing to do */

        movl    sp@(4),a0           /* a0 = src */
        movl    sp@(8),a1           /* a1 = dst */

        cmpl    a1,a0
        blo     bcback_020          /* src < dst -> copy backwards */

/* ------------------------------------------------------------------ */
/* Forward copy: src >= dst                                           */
/* ------------------------------------------------------------------ */

bcforw_020:
        cmpl    #16,d1              /* small block? */
        blo     bcf_small_020       /* len < 16 -> byte loop only */

        /*
         * Try to get 4-byte alignment when src and dst share alignment.
         * If (a0 - a1) & 3 != 0, skip alignment and still use
         * longword copy. 020+ tolerates unaligned longwords.
         */
        movl    a0,d0
        subl    a1,d0
        andl    #3,d0
        bne     bcf_noalign_020     /* different alignment -> no pre-align */

        /* src and dst share low 2 bits, align dst and thus src to 4 */
        movl    a1,d0
        andl    #3,d0
        beq     bcf_lw_entry_020    /* already 4-byte aligned */

bcf_align_loop_020:
        movb    a0@+,a1@+           /* copy byte until dst 4-byte aligned */
        subql   #1,d1
        beq     bcdone_bcopy_020
        movl    a1,d0
        andl    #3,d0
        bne     bcf_align_loop_020
        bra     bcf_lw_entry_020

bcf_noalign_020:
        /* No shared alignment: still do longwords, but without pre-align. */

bcf_lw_entry_020:
        movl    d1,d0
        lsrl    #4,d0               /* d0 = len / 16 (4-longword blocks) */
        beq     bcf_lw_rem_020

bcf_lw_unroll_020:
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        movl    a0@+,a1@+
        subql   #1,d0
        bne     bcf_lw_unroll_020

bcf_lw_rem_020:
        /*
         * d1 is the remaining byte count after any pre-align bytes.
         * It is not decremented by the 16-byte block loop.
         * Remaining forward longwords after 16-byte / 4-longword blocks:
         *     (d1 >> 2) & 3
         * Tail bytes are later computed as d1 & 3.
         */

        movl    d1,d0
        lsrl    #2,d0               /* d0 = len / 4 (longwords) */
        andl    #3,d0               /* remaining longwords (0..3) */
        beq     bcf_tail_020

bcf_lw_loop_020:
        movl    a0@+,a1@+
        subql   #1,d0
        bne     bcf_lw_loop_020

bcf_tail_020:
        andl    #3,d1               /* remaining bytes */
        beq     bcdone_bcopy_020

bcf_b_loop_020:
        movb    a0@+,a1@+           /* copy remaining bytes */
        subql   #1,d1
        bne     bcf_b_loop_020
        bra     bcdone_bcopy_020

bcf_small_020:
        /* Small forward copy: pure byte loop */
bcf_small_loop_020:
        movb    a0@+,a1@+
        subql   #1,d1
        bne     bcf_small_loop_020
        bra     bcdone_bcopy_020

/* ------------------------------------------------------------------ */
/* Backward copy: src < dst                                           */
/* ------------------------------------------------------------------ */

bcback_020:
        addl    d1,a0               /* a0 = src + len */
        addl    d1,a1               /* a1 = dst + len */

        cmpl    #16,d1              /* small block? */
        blo     bcb_small_020       /* len < 16 -> byte loop only */

        /*
         * Same alignment strategy as forward copy, but backwards.
         */
        movl    a0,d0
        subl    a1,d0
        andl    #3,d0
        bne     bcb_noalign_020     /* different alignment -> no pre-align */

        movl    a1,d0
        andl    #3,d0
        beq     bcb_lw_entry_020

bcb_align_loop_020:
        movb    a0@-,a1@-           /* copy byte until dst 4-byte aligned */
        subql   #1,d1
        beq     bcdone_bcopy_020
        movl    a1,d0
        andl    #3,d0
        bne     bcb_align_loop_020
        bra     bcb_lw_entry_020

bcb_noalign_020:

bcb_lw_entry_020:
        movl    d1,d0
        lsrl    #4,d0               /* d0 = len / 16 (4-longword blocks) */
        beq     bcb_lw_rem_020

bcb_lw_unroll_020:
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        movl    a0@-,a1@-
        subql   #1,d0
        bne     bcb_lw_unroll_020

bcb_lw_rem_020:
        /*
         * d1 is the remaining byte count after any pre-align bytes.
         * It is not decremented by the 16-byte block loop.
         * Remaining backward longwords after 16-byte / 4-longword blocks:
         *     (d1 >> 2) & 3
         * Tail bytes are later computed as d1 & 3.
         */

        movl    d1,d0
        lsrl    #2,d0               /* d0 = len / 4 */
        andl    #3,d0
        beq     bcb_tail_020

bcb_lw_loop_020:
        movl    a0@-,a1@-
        subql   #1,d0
        bne     bcb_lw_loop_020

bcb_tail_020:
        andl    #3,d1               /* remaining bytes */
        beq     bcdone_bcopy_020

bcb_b_loop_020:
        movb    a0@-,a1@-           /* copy remaining bytes backwards */
        subql   #1,d1
        bne     bcb_b_loop_020
        bra     bcdone_bcopy_020

bcb_small_020:
        /* Small backward copy: pure byte loop */
bcb_small_loop_020:
        movb    a0@-,a1@-
        subql   #1,d1
        bne     bcb_small_loop_020

bcdone_bcopy_020:
        rts
");

#else  /* BCOPY_CPU_000 */

/*
 * 68000/68010-safe bcopy.
 *
 * Notes:
 *      - preserves original 68000-safe behavior
 *      - avoids odd longword accesses
 *      - uses only caller-saved registers: d0/d1/a0/a1
 */

ENTRY(bcopy)
asm("
	movl	sp@(12),d1	/* check count */
	tstl	d1
	jle	bcdone_bcopy	/* count <= 0, don't do anything */
	movl	sp@(4),a0	/* src address */
	movl	sp@(8),a1	/* dest address */
	cmpl	a1,a0		/* src before dest? */
	blo	bcback		/* yes, must copy backwards */
	movl	a0,d0
	btst	#0,d0		/* src address odd? */
	jeq	bcfeven		/* no, skip alignment */
	movb	a0@+,a1@+	/* yes, copy a byte */
	subql	#1,d1		/* adjust count */
	jeq	bcdone_bcopy	/* count 0, all done  */
bcfeven:
	movl	a1,d0
	btst	#0,d0		/* dest address odd? */
	jne	bcfbloop	/* yes, no hope for alignment, copy bytes */
	movl	d1,d0		/* no, both even */
	lsrl	#2,d0		/* convert count to longword count */
	jeq	bcfbloop	/* count 0, skip longword loop */
bcflloop:
	movl	a0@+,a1@+	/* copy a longword */
	subql	#1,d0		/* adjust count */
	jne	bcflloop	/* still more, keep copying */
	andl	#3,d1		/* what remains */
	jeq	bcdone_bcopy	/* nothing, all done */
bcfbloop:
	movb	a0@+,a1@+	/* copy a byte */
	subql	#1,d1		/* adjust count */
	jne	bcfbloop	/* still more, keep going */
bcdone_bcopy:
	rts
bcback:
	addl	d1,a0		/* src pointer to end */
	addl	d1,a1		/* dest pointer to end */
	movl	a0,d0
	btst	#0,d0		/* src address odd? */
	jeq	bcbeven		/* no, skip alignment */
	movb	a0@-,a1@-	/* yes, copy a byte */
	subql	#1,d1		/* adjust count */
	jeq	bcdone_bcopy	/* count 0, all done  */
bcbeven:
	movl	a1,d0
	btst	#0,d0		/* dest address odd? */
	jne	bcbbloop	/* yes, no hope for alignment, copy bytes */
	movl	d1,d0		/* no, both even */
	lsrl	#2,d0		/* convert count to longword count */
	jeq	bcbbloop	/* count 0, skip longword loop */
bcblloop:
	movl	a0@-,a1@-	/* copy a longword */
	subql	#1,d0		/* adjust count */
	jne	bcblloop	/* still more, keep copying */
	andl	#3,d1		/* what remains */
	jeq	bcdone_bcopy	/* nothing, all done */
bcbbloop:
	movb	a0@-,a1@-	/* copy a byte */
	subql	#1,d1		/* adjust count */
	jne	bcbbloop	/* still more, keep going */
	rts
");

#endif
