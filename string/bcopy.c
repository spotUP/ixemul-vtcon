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
asm("\n\
        movl    sp@(12),d1           /* length */\n\
        tstl    d1\n\
        jle     .done                /* len <= 0 -> nothing to do */\n\
\n\
        movl    sp@(4),a0            /* src */\n\
        movl    sp@(8),a1            /* dst */\n\
\n\
        cmpl    a1,a0\n\
        blo     .backward            /* copy backwards if src < dst */\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Forward copy                                                       */\n\
/* ------------------------------------------------------------------ */\n\
\n\
.forward:\n\
        cmpl    #32,d1               /* small block */\n\
        blo     .f_small\n\
\n\
.f_lw_entry:\n\
        movl    d1,d0\n\
        lsrl    #5,d0                /* 32-byte / 8-longword blocks */\n\
        beq     .f_lw_rem\n\
\n\
.f_lw_unroll:\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     .f_lw_unroll\n\
\n\
.f_lw_rem:\n\
        /*\n\
         * d1 still holds the byte count for this phase.\n\
         * Remaining longwords after 32-byte / 8-longword blocks:\n\
         *     (d1 >> 2) & 7\n\
         * d1 is not decremented by the 32-byte block loop.\n\
         */\n\
\n\
        movl    d1,d0\n\
        lsrl    #2,d0                /* longword count */\n\
        andl    #7,d0\n\
        beq     .f_tail\n\
\n\
.f_lw_loop:\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     .f_lw_loop\n\
\n\
.f_tail:\n\
        andl    #3,d1                /* remaining bytes */\n\
        beq     .done\n\
\n\
.f_b_loop:\n\
        movb    a0@+,a1@+\n\
        subql   #1,d1\n\
        bne     .f_b_loop\n\
        bra     .done\n\
\n\
.f_small:\n\
.f_small_loop:\n\
        movb    a0@+,a1@+\n\
        subql   #1,d1\n\
        bne     .f_small_loop\n\
        bra     .done\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Backward copy                                                      */\n\
/* ------------------------------------------------------------------ */\n\
\n\
.backward:\n\
        addl    d1,a0                /* src end */\n\
        addl    d1,a1                /* dst end */\n\
\n\
        cmpl    #32,d1\n\
        blo     .b_small\n\
\n\
.b_lw_entry:\n\
        movl    d1,d0\n\
        lsrl    #5,d0                /* 32-byte / 8-longword blocks */\n\
        beq     .b_lw_rem\n\
\n\
.b_lw_unroll:\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     .b_lw_unroll\n\
\n\
.b_lw_rem:\n\
        /*\n\
         * d1 still holds the byte count for this phase.\n\
         * Remaining backward longwords after 32-byte / 8-longword blocks:\n\
         *     (d1 >> 2) & 7\n\
         * d1 is not decremented by the 32-byte block loop.\n\
         */\n\
\n\
        movl    d1,d0\n\
        lsrl    #2,d0                /* longword count */\n\
        andl    #7,d0\n\
        beq     .b_tail\n\
\n\
.b_lw_loop:\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     .b_lw_loop\n\
\n\
.b_tail:\n\
        andl    #3,d1\n\
        beq     .done\n\
\n\
.b_b_loop:\n\
        movb    a0@-,a1@-\n\
        subql   #1,d1\n\
        bne     .b_b_loop\n\
        bra     .done\n\
\n\
.b_small:\n\
.b_small_loop:\n\
        movb    a0@-,a1@-\n\
        subql   #1,d1\n\
        bne     .b_small_loop\n\
\n\
.done:\n\
        rts\n\
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
asm("\n\
        movl    sp@(12),d1          /* d1 = len */\n\
        tstl    d1\n\
        jle     bcdone_bcopy_040    /* len <= 0 -> nothing to do */\n\
\n\
        movl    sp@(4),a0           /* a0 = src */\n\
        movl    sp@(8),a1           /* a1 = dst */\n\
\n\
        cmpl    a1,a0\n\
        blo     bcback_040          /* src < dst -> copy backwards */\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Forward copy: src >= dst                                           */\n\
/* ------------------------------------------------------------------ */\n\
\n\
bcforw_040:\n\
        cmpl    #16,d1              /* small block */\n\
        blo     bcf_small_040\n\
\n\
        /*\n\
         * Try to get 4-byte alignment when src and dst share alignment.\n\
         * If (a0 - a1) & 3 != 0, skip alignment and still use\n\
         * longword copy. 040 tolerates unaligned longwords.\n\
         */\n\
        movl    a0,d0\n\
        subl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcf_noalign_040     /* different alignment -> no pre-align */\n\
\n\
        /* src and dst share low 2 bits, align dst and thus src to 4 */\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        beq     bcf_lw_entry_040    /* already 4-byte aligned */\n\
\n\
bcf_align_loop_040:\n\
        movb    a0@+,a1@+           /* copy byte until dst 4-byte aligned */\n\
        subql   #1,d1\n\
        beq     bcdone_bcopy_040\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcf_align_loop_040\n\
        bra     bcf_lw_entry_040\n\
\n\
bcf_noalign_040:\n\
        /* No shared alignment: still do longwords, but without pre-align. */\n\
\n\
bcf_lw_entry_040:\n\
        movl    d1,d0\n\
        lsrl    #4,d0               /* 16-byte / 4-longword blocks */\n\
        beq     bcf_lw_rem_040\n\
\n\
bcf_lw_unroll_040:\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     bcf_lw_unroll_040\n\
\n\
bcf_lw_rem_040:\n\
        /*\n\
         * d1 still holds the original byte count.\n\
         * It is not decremented by the 16-byte block loop.\n\
         * Remaining forward longwords after 16-byte / 4-longword blocks:\n\
         *     (d1 >> 2) & 3\n\
         * Tail bytes are later computed as d1 & 3.\n\
         */\n\
        movl    d1,d0\n\
        lsrl    #2,d0               /* d0 = len / 4 */\n\
        andl    #3,d0               /* remaining longwords */\n\
        beq     bcf_tail_040\n\
\n\
bcf_lw_loop_040:\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     bcf_lw_loop_040\n\
\n\
bcf_tail_040:\n\
        /*\n\
         * d1 still holds the original byte count.\n\
         * Tail bytes are:\n\
         *     d1 & 3\n\
         */\n\
\n\
        andl    #3,d1               /* remaining bytes */\n\
        beq     bcdone_bcopy_040\n\
\n\
bcf_b_loop_040:\n\
        movb    a0@+,a1@+           /* copy byte */\n\
        subql   #1,d1\n\
        bne     bcf_b_loop_040\n\
        bra     bcdone_bcopy_040\n\
\n\
bcf_small_040:\n\
        movb    a0@+,a1@+           /* small forward copy */\n\
        subql   #1,d1\n\
        bne     bcf_small_040\n\
        bra     bcdone_bcopy_040\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Backward copy: src < dst                                           */\n\
/* ------------------------------------------------------------------ */\n\
\n\
bcback_040:\n\
        addl    d1,a0               /* a0 = src + len */\n\
        addl    d1,a1               /* a1 = dst + len */\n\
\n\
        cmpl    #16,d1              /* small block */\n\
        blo     bcb_small_040\n\
\n\
        /*\n\
         * Same alignment strategy as forward copy, but backwards.\n\
         */\n\
        movl    a0,d0\n\
        subl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcb_noalign_040     /* different alignment -> no pre-align */\n\
\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        beq     bcb_lw_entry_040\n\
\n\
bcb_align_loop_040:\n\
        movb    a0@-,a1@-           /* copy byte until dst 4-byte aligned */\n\
        subql   #1,d1\n\
        beq     bcdone_bcopy_040\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcb_align_loop_040\n\
        bra     bcb_lw_entry_040\n\
\n\
bcb_noalign_040:\n\
\n\
bcb_lw_entry_040:\n\
        movl    d1,d0\n\
        lsrl    #4,d0               /* 16-byte / 4-longword blocks */\n\
        beq     bcb_lw_rem_040\n\
\n\
bcb_lw_unroll_040:\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     bcb_lw_unroll_040\n\
\n\
bcb_lw_rem_040:\n\
        /*\n\
         * d1 still holds the original byte count.\n\
         * It is not decremented by the 16-byte block loop.\n\
         * Remaining backward longwords after 16-byte / 4-longword blocks:\n\
         *     (d1 >> 2) & 3\n\
         * Tail bytes are later computed as d1 & 3.\n\
         */\n\
        movl    d1,d0\n\
        lsrl    #2,d0               /* d0 = len / 4 */\n\
        andl    #3,d0               /* remaining longwords */\n\
        beq     bcb_tail_040\n\
\n\
bcb_lw_loop_040:\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     bcb_lw_loop_040\n\
\n\
bcb_tail_040:\n\
        /*\n\
         * d1 still holds the original byte count.\n\
         * Tail bytes are:\n\
         *     d1 & 3\n\
         */\n\
\n\
        andl    #3,d1               /* remaining bytes */\n\
        beq     bcdone_bcopy_040\n\
\n\
bcb_b_loop_040:\n\
        movb    a0@-,a1@-           /* copy byte backwards */\n\
        subql   #1,d1\n\
        bne     bcb_b_loop_040\n\
        bra     bcdone_bcopy_040\n\
\n\
bcb_small_040:\n\
        movb    a0@-,a1@-           /* small backward copy */\n\
        subql   #1,d1\n\
        bne     bcb_small_040\n\
\n\
bcdone_bcopy_040:\n\
        rts\n\
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
asm("\n\
        movl    sp@(12),d1           /* d1 = len */\n\
        tstl    d1\n\
        jle     bcdone_bcopy_020    /* len <= 0 -> nothing to do */\n\
\n\
        movl    sp@(4),a0           /* a0 = src */\n\
        movl    sp@(8),a1           /* a1 = dst */\n\
\n\
        cmpl    a1,a0\n\
        blo     bcback_020          /* src < dst -> copy backwards */\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Forward copy: src >= dst                                           */\n\
/* ------------------------------------------------------------------ */\n\
\n\
bcforw_020:\n\
        cmpl    #16,d1              /* small block? */\n\
        blo     bcf_small_020       /* len < 16 -> byte loop only */\n\
\n\
        /*\n\
         * Try to get 4-byte alignment when src and dst share alignment.\n\
         * If (a0 - a1) & 3 != 0, skip alignment and still use\n\
         * longword copy. 020+ tolerates unaligned longwords.\n\
         */\n\
        movl    a0,d0\n\
        subl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcf_noalign_020     /* different alignment -> no pre-align */\n\
\n\
        /* src and dst share low 2 bits, align dst and thus src to 4 */\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        beq     bcf_lw_entry_020    /* already 4-byte aligned */\n\
\n\
bcf_align_loop_020:\n\
        movb    a0@+,a1@+           /* copy byte until dst 4-byte aligned */\n\
        subql   #1,d1\n\
        beq     bcdone_bcopy_020\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcf_align_loop_020\n\
        bra     bcf_lw_entry_020\n\
\n\
bcf_noalign_020:\n\
        /* No shared alignment: still do longwords, but without pre-align. */\n\
\n\
bcf_lw_entry_020:\n\
        movl    d1,d0\n\
        lsrl    #4,d0               /* d0 = len / 16 (4-longword blocks) */\n\
        beq     bcf_lw_rem_020\n\
\n\
bcf_lw_unroll_020:\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     bcf_lw_unroll_020\n\
\n\
bcf_lw_rem_020:\n\
        /*\n\
         * d1 is the remaining byte count after any pre-align bytes.\n\
         * It is not decremented by the 16-byte block loop.\n\
         * Remaining forward longwords after 16-byte / 4-longword blocks:\n\
         *     (d1 >> 2) & 3\n\
         * Tail bytes are later computed as d1 & 3.\n\
         */\n\
\n\
        movl    d1,d0\n\
        lsrl    #2,d0               /* d0 = len / 4 (longwords) */\n\
        andl    #3,d0               /* remaining longwords (0..3) */\n\
        beq     bcf_tail_020\n\
\n\
bcf_lw_loop_020:\n\
        movl    a0@+,a1@+\n\
        subql   #1,d0\n\
        bne     bcf_lw_loop_020\n\
\n\
bcf_tail_020:\n\
        andl    #3,d1               /* remaining bytes */\n\
        beq     bcdone_bcopy_020\n\
\n\
bcf_b_loop_020:\n\
        movb    a0@+,a1@+           /* copy remaining bytes */\n\
        subql   #1,d1\n\
        bne     bcf_b_loop_020\n\
        bra     bcdone_bcopy_020\n\
\n\
bcf_small_020:\n\
        /* Small forward copy: pure byte loop */\n\
bcf_small_loop_020:\n\
        movb    a0@+,a1@+\n\
        subql   #1,d1\n\
        bne     bcf_small_loop_020\n\
        bra     bcdone_bcopy_020\n\
\n\
/* ------------------------------------------------------------------ */\n\
/* Backward copy: src < dst                                           */\n\
/* ------------------------------------------------------------------ */\n\
\n\
bcback_020:\n\
        addl    d1,a0               /* a0 = src + len */\n\
        addl    d1,a1               /* a1 = dst + len */\n\
\n\
        cmpl    #16,d1              /* small block? */\n\
        blo     bcb_small_020       /* len < 16 -> byte loop only */\n\
\n\
        /*\n\
         * Same alignment strategy as forward copy, but backwards.\n\
         */\n\
        movl    a0,d0\n\
        subl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcb_noalign_020     /* different alignment -> no pre-align */\n\
\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        beq     bcb_lw_entry_020\n\
\n\
bcb_align_loop_020:\n\
        movb    a0@-,a1@-           /* copy byte until dst 4-byte aligned */\n\
        subql   #1,d1\n\
        beq     bcdone_bcopy_020\n\
        movl    a1,d0\n\
        andl    #3,d0\n\
        bne     bcb_align_loop_020\n\
        bra     bcb_lw_entry_020\n\
\n\
bcb_noalign_020:\n\
\n\
bcb_lw_entry_020:\n\
        movl    d1,d0\n\
        lsrl    #4,d0               /* d0 = len / 16 (4-longword blocks) */\n\
        beq     bcb_lw_rem_020\n\
\n\
bcb_lw_unroll_020:\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     bcb_lw_unroll_020\n\
\n\
bcb_lw_rem_020:\n\
        /*\n\
         * d1 is the remaining byte count after any pre-align bytes.\n\
         * It is not decremented by the 16-byte block loop.\n\
         * Remaining backward longwords after 16-byte / 4-longword blocks:\n\
         *     (d1 >> 2) & 3\n\
         * Tail bytes are later computed as d1 & 3.\n\
         */\n\
\n\
        movl    d1,d0\n\
        lsrl    #2,d0               /* d0 = len / 4 */\n\
        andl    #3,d0\n\
        beq     bcb_tail_020\n\
\n\
bcb_lw_loop_020:\n\
        movl    a0@-,a1@-\n\
        subql   #1,d0\n\
        bne     bcb_lw_loop_020\n\
\n\
bcb_tail_020:\n\
        andl    #3,d1               /* remaining bytes */\n\
        beq     bcdone_bcopy_020\n\
\n\
bcb_b_loop_020:\n\
        movb    a0@-,a1@-           /* copy remaining bytes backwards */\n\
        subql   #1,d1\n\
        bne     bcb_b_loop_020\n\
        bra     bcdone_bcopy_020\n\
\n\
bcb_small_020:\n\
        /* Small backward copy: pure byte loop */\n\
bcb_small_loop_020:\n\
        movb    a0@-,a1@-\n\
        subql   #1,d1\n\
        bne     bcb_small_loop_020\n\
\n\
bcdone_bcopy_020:\n\
        rts\n\
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
asm("\n\
	movl	sp@(12),d1	/* check count */\n\
	tstl	d1\n\
	jle	bcdone_bcopy	/* count <= 0, don't do anything */\n\
	movl	sp@(4),a0	/* src address */\n\
	movl	sp@(8),a1	/* dest address */\n\
	cmpl	a1,a0		/* src before dest? */\n\
	blo	bcback		/* yes, must copy backwards */\n\
	movl	a0,d0\n\
	btst	#0,d0		/* src address odd? */\n\
	jeq	bcfeven		/* no, skip alignment */\n\
	movb	a0@+,a1@+	/* yes, copy a byte */\n\
	subql	#1,d1		/* adjust count */\n\
	jeq	bcdone_bcopy	/* count 0, all done  */\n\
bcfeven:\n\
	movl	a1,d0\n\
	btst	#0,d0		/* dest address odd? */\n\
	jne	bcfbloop	/* yes, no hope for alignment, copy bytes */\n\
	movl	d1,d0		/* no, both even */\n\
	lsrl	#2,d0		/* convert count to longword count */\n\
	jeq	bcfbloop	/* count 0, skip longword loop */\n\
bcflloop:\n\
	movl	a0@+,a1@+	/* copy a longword */\n\
	subql	#1,d0		/* adjust count */\n\
	jne	bcflloop	/* still more, keep copying */\n\
	andl	#3,d1		/* what remains */\n\
	jeq	bcdone_bcopy	/* nothing, all done */\n\
bcfbloop:\n\
	movb	a0@+,a1@+	/* copy a byte */\n\
	subql	#1,d1		/* adjust count */\n\
	jne	bcfbloop	/* still more, keep going */\n\
bcdone_bcopy:\n\
	rts\n\
bcback:\n\
	addl	d1,a0		/* src pointer to end */\n\
	addl	d1,a1		/* dest pointer to end */\n\
	movl	a0,d0\n\
	btst	#0,d0		/* src address odd? */\n\
	jeq	bcbeven		/* no, skip alignment */\n\
	movb	a0@-,a1@-	/* yes, copy a byte */\n\
	subql	#1,d1		/* adjust count */\n\
	jeq	bcdone_bcopy	/* count 0, all done  */\n\
bcbeven:\n\
	movl	a1,d0\n\
	btst	#0,d0		/* dest address odd? */\n\
	jne	bcbbloop	/* yes, no hope for alignment, copy bytes */\n\
	movl	d1,d0		/* no, both even */\n\
	lsrl	#2,d0		/* convert count to longword count */\n\
	jeq	bcbbloop	/* count 0, skip longword loop */\n\
bcblloop:\n\
	movl	a0@-,a1@-	/* copy a longword */\n\
	subql	#1,d0		/* adjust count */\n\
	jne	bcblloop	/* still more, keep copying */\n\
	andl	#3,d1		/* what remains */\n\
	jeq	bcdone_bcopy	/* nothing, all done */\n\
bcbbloop:\n\
	movb	a0@-,a1@-	/* copy a byte */\n\
	subql	#1,d1		/* adjust count */\n\
	jne	bcbbloop	/* still more, keep going */\n\
	rts\n\
");

#endif
