/*
 * This file is part of ixemul.library for the Amiga.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * swab.c,v
 *
 * Revision 1.2  2026/08/25  ChatGPT modifications (JJ)
 *
 *    Treat negative byte counts as a no-op before converting the count to
 *    complete byte pairs in every CPU-specific implementation.
 *
 * Revision 1.1  2026/08/16  ChatGPT (JJ)
 *
 *    Add a new CPU-selected m68k implementation of swab() while preserving
 *    its byte-count semantics.
 *
 *    Ignore an unpaired final byte, preserve zero-length operation, and
 *    support exact in-place conversion.
 *
 *    Use an eight-word unrolled path on 68060, a four-word unrolled path
 *    on 68040, a MOVEP-based four-word path for distinct buffers on
 *    68020/030, and an alignment-safe 68000/010 path with byte fallback.
 */

#include "defs.h"

/*
 * swab(from, to, len)
 *
 * len is a byte count.  Only complete byte pairs are processed, so an odd
 * final byte is ignored.  Exact in-place conversion is supported.
 * Partially overlapping source and destination ranges are not supported.
 */

#if defined(__mc68060__) || defined(mc68060) || \
    defined(__m68060__)  || defined(m68060)
#define SWAB_CPU_060 1

#elif defined(__mc68040__) || defined(mc68040) || \
      defined(__m68040__)  || defined(m68040)
#define SWAB_CPU_040 1

#elif defined(__mc68030__) || defined(mc68030) || \
      defined(__m68030__)  || defined(m68030)  || \
      defined(__mc68020__) || defined(mc68020) || \
      defined(__m68020__)  || defined(m68020)
#define SWAB_CPU_020 1

#else
#define SWAB_CPU_000 1
#endif


#if SWAB_CPU_060

ENTRY(swab)
asm("\n\
        movl    sp@(12),d1              /* byte count */\n\
        tstl    d1\n\
        jmi     swab_done_060          /* negative count: no operation */\n\
        lsrl    #1,d1                   /* complete 16-bit pairs */\n\
        jeq     swab_done_060\n\
\n\
        movl    sp@(4),a0               /* source */\n\
        movl    sp@(8),a1               /* destination */\n\
\n\
        cmpl    #8,d1\n\
        blo     swab_tail_060\n\
\n\
swab_loop8_060:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        subql   #8,d1\n\
        cmpl    #8,d1\n\
        jcc     swab_loop8_060\n\
\n\
swab_tail_060:\n\
        tstl    d1\n\
        jeq     swab_done_060\n\
\n\
swab_tail_loop_060:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
        subql   #1,d1\n\
        jne     swab_tail_loop_060\n\
\n\
swab_done_060:\n\
        rts\n\
");


#elif SWAB_CPU_040

ENTRY(swab)
asm("\n\
        movl    sp@(12),d1              /* byte count */\n\
        tstl    d1\n\
        jmi     swab_done_040          /* negative count: no operation */\n\
        lsrl    #1,d1                   /* complete 16-bit pairs */\n\
        jeq     swab_done_040\n\
\n\
        movl    sp@(4),a0               /* source */\n\
        movl    sp@(8),a1               /* destination */\n\
\n\
        cmpl    #4,d1\n\
        blo     swab_tail_040\n\
\n\
swab_loop4_040:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        subql   #4,d1\n\
        cmpl    #4,d1\n\
        jcc     swab_loop4_040\n\
\n\
swab_tail_040:\n\
        tstl    d1\n\
        jeq     swab_done_040\n\
\n\
swab_tail_loop_040:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
        subql   #1,d1\n\
        jne     swab_tail_loop_040\n\
\n\
swab_done_040:\n\
        rts\n\
");


#elif SWAB_CPU_020

ENTRY(swab)
asm("\n\
        movl    sp@(12),d1              /* byte count */\n\
        tstl    d1\n\
        jmi     swab_done_020          /* negative count: no operation */\n\
        lsrl    #1,d1                   /* complete 16-bit pairs */\n\
        jeq     swab_done_020\n\
\n\
        movl    sp@(4),a0               /* source */\n\
        movl    sp@(8),a1               /* destination */\n\
\n\
        cmpl    a1,a0\n\
        jeq     swab_word_entry_020     /* MOVEP is not in-place safe */\n\
\n\
        cmpl    #4,d1\n\
        blo     swab_word_entry_020\n\
\n\
swab_movep4_020:\n\
        movepl  a0@(0),d0               /* source bytes 0,2,4,6 */\n\
        movepl  d0,a1@(1)               /* -> destination 1,3,5,7 */\n\
        movepl  a0@(1),d0               /* source bytes 1,3,5,7 */\n\
        movepl  d0,a1@(0)               /* -> destination 0,2,4,6 */\n\
        addql   #8,a0\n\
        addql   #8,a1\n\
        subql   #4,d1\n\
        cmpl    #4,d1\n\
        jcc     swab_movep4_020\n\
\n\
swab_word_entry_020:\n\
        tstl    d1\n\
        jeq     swab_done_020\n\
\n\
        cmpl    #4,d1\n\
        blo     swab_word_tail_020\n\
\n\
swab_word4_020:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        subql   #4,d1\n\
        cmpl    #4,d1\n\
        jcc     swab_word4_020\n\
\n\
swab_word_tail_020:\n\
        tstl    d1\n\
        jeq     swab_done_020\n\
\n\
swab_word_tail_loop_020:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
        subql   #1,d1\n\
        jne     swab_word_tail_loop_020\n\
\n\
swab_done_020:\n\
        rts\n\
");


#else  /* SWAB_CPU_000 */

ENTRY(swab)
asm("\n\
        movl    sp@(12),d1              /* byte count */\n\
        tstl    d1\n\
        jmi     swab_done_000          /* negative count: no operation */\n\
        lsrl    #1,d1                   /* complete 16-bit pairs */\n\
        jeq     swab_done_000\n\
\n\
        movl    sp@(4),a0               /* source */\n\
        movl    sp@(8),a1               /* destination */\n\
\n\
        movl    a0,d0\n\
        btst    #0,d0\n\
        jne     swab_byte_loop_000\n\
        movl    a1,d0\n\
        btst    #0,d0\n\
        jne     swab_byte_loop_000\n\
\n\
        cmpl    #4,d1\n\
        blo     swab_word_tail_000\n\
\n\
swab_word4_000:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
\n\
        subql   #4,d1\n\
        cmpl    #4,d1\n\
        jcc     swab_word4_000\n\
\n\
swab_word_tail_000:\n\
        tstl    d1\n\
        jeq     swab_done_000\n\
\n\
swab_word_tail_loop_000:\n\
        movw    a0@+,d0\n\
        rorw    #8,d0\n\
        movw    d0,a1@+\n\
        subql   #1,d1\n\
        jne     swab_word_tail_loop_000\n\
        bra     swab_done_000\n\
\n\
swab_byte_loop_000:\n\
        movb    a0@+,d0\n\
        movb    a0@+,a1@+\n\
        movb    d0,a1@+\n\
        subql   #1,d1\n\
        jne     swab_byte_loop_000\n\
\n\
swab_done_000:\n\
        rts\n\
");

#endif
