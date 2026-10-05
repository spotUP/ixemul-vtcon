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
asm("
        movl    sp@(12),d1              /* byte count */
        tstl    d1
        jmi     swab_done_060          /* negative count: no operation */
        lsrl    #1,d1                   /* complete 16-bit pairs */
        jeq     swab_done_060

        movl    sp@(4),a0               /* source */
        movl    sp@(8),a1               /* destination */

        cmpl    #8,d1
        blo     swab_tail_060

swab_loop8_060:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        subql   #8,d1
        cmpl    #8,d1
        jcc     swab_loop8_060

swab_tail_060:
        tstl    d1
        jeq     swab_done_060

swab_tail_loop_060:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+
        subql   #1,d1
        jne     swab_tail_loop_060

swab_done_060:
        rts
");


#elif SWAB_CPU_040

ENTRY(swab)
asm("
        movl    sp@(12),d1              /* byte count */
        tstl    d1
        jmi     swab_done_040          /* negative count: no operation */
        lsrl    #1,d1                   /* complete 16-bit pairs */
        jeq     swab_done_040

        movl    sp@(4),a0               /* source */
        movl    sp@(8),a1               /* destination */

        cmpl    #4,d1
        blo     swab_tail_040

swab_loop4_040:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        subql   #4,d1
        cmpl    #4,d1
        jcc     swab_loop4_040

swab_tail_040:
        tstl    d1
        jeq     swab_done_040

swab_tail_loop_040:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+
        subql   #1,d1
        jne     swab_tail_loop_040

swab_done_040:
        rts
");


#elif SWAB_CPU_020

ENTRY(swab)
asm("
        movl    sp@(12),d1              /* byte count */
        tstl    d1
        jmi     swab_done_020          /* negative count: no operation */
        lsrl    #1,d1                   /* complete 16-bit pairs */
        jeq     swab_done_020

        movl    sp@(4),a0               /* source */
        movl    sp@(8),a1               /* destination */

        cmpl    a1,a0
        jeq     swab_word_entry_020     /* MOVEP is not in-place safe */

        cmpl    #4,d1
        blo     swab_word_entry_020

swab_movep4_020:
        movepl  a0@(0),d0               /* source bytes 0,2,4,6 */
        movepl  d0,a1@(1)               /* -> destination 1,3,5,7 */
        movepl  a0@(1),d0               /* source bytes 1,3,5,7 */
        movepl  d0,a1@(0)               /* -> destination 0,2,4,6 */
        addql   #8,a0
        addql   #8,a1
        subql   #4,d1
        cmpl    #4,d1
        jcc     swab_movep4_020

swab_word_entry_020:
        tstl    d1
        jeq     swab_done_020

        cmpl    #4,d1
        blo     swab_word_tail_020

swab_word4_020:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        subql   #4,d1
        cmpl    #4,d1
        jcc     swab_word4_020

swab_word_tail_020:
        tstl    d1
        jeq     swab_done_020

swab_word_tail_loop_020:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+
        subql   #1,d1
        jne     swab_word_tail_loop_020

swab_done_020:
        rts
");


#else  /* SWAB_CPU_000 */

ENTRY(swab)
asm("
        movl    sp@(12),d1              /* byte count */
        tstl    d1
        jmi     swab_done_000          /* negative count: no operation */
        lsrl    #1,d1                   /* complete 16-bit pairs */
        jeq     swab_done_000

        movl    sp@(4),a0               /* source */
        movl    sp@(8),a1               /* destination */

        movl    a0,d0
        btst    #0,d0
        jne     swab_byte_loop_000
        movl    a1,d0
        btst    #0,d0
        jne     swab_byte_loop_000

        cmpl    #4,d1
        blo     swab_word_tail_000

swab_word4_000:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+

        subql   #4,d1
        cmpl    #4,d1
        jcc     swab_word4_000

swab_word_tail_000:
        tstl    d1
        jeq     swab_done_000

swab_word_tail_loop_000:
        movw    a0@+,d0
        rorw    #8,d0
        movw    d0,a1@+
        subql   #1,d1
        jne     swab_word_tail_loop_000
        bra     swab_done_000

swab_byte_loop_000:
        movb    a0@+,d0
        movb    a0@+,a1@+
        movb    d0,a1@+
        subql   #1,d1
        jne     swab_byte_loop_000

swab_done_000:
        rts
");

#endif
