/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define START
#include <ixemul.h>
#include "version.h"

#include "ix_internals.h"

	.text

	.globl	Start
Start:
   movel   #-1,d0
   rts

initDDescrip:
               |STRUCTURE RT,0
     .word    RTC_MATCHWORD
     .long    initDDescrip
     .long    EndCode
     .byte    RTF_AUTOINIT
     .byte    IX_VERSION
     .byte    NT_LIBRARY
     .byte    IX_PRIORITY
     .long    ixName
     .long    idString 
     .long    Init

EndCode:

ixName:    .asciz IX_NAME

idString:
	.ascii IX_IDSTRING
	.byte 13
	.byte 10
	.byte 0

   .even

Init:
   .long   IXBASE_SIZEOF	
   .long   funcTable	
   .long   dataTable     
   .long   _initRoutine  


funcTable:

   .long   _libOpen
   .long   _libClose
   .long   _libExpunge
   .long   Null

#define ___must_recompile27  ___must_recompile
#define ___must_recompile28  ___must_recompile
#define ___must_recompile29  ___must_recompile
#define ___must_recompile30  ___must_recompile
#define ___must_recompile31  ___must_recompile
#define ___must_recompile32  ___must_recompile
#define ___must_recompile33  ___must_recompile
#define ___must_recompile34  ___must_recompile
#define ___must_recompile215 ___must_recompile
#define ___must_recompile216 ___must_recompile
#define ___must_recompile217 ___must_recompile
#define ___must_recompile220 ___must_recompile
#define ___must_recompile221 ___must_recompile
#define ___must_recompile222 ___must_recompile
#define ___must_recompile223 ___must_recompile
#define ___must_recompile224 ___must_recompile
#define ___must_recompile226 ___must_recompile
#define ___must_recompile229 ___must_recompile
#define ___must_recompile235 ___must_recompile
#define ___must_recompile237 ___must_recompile
#define ___must_recompile239 ___must_recompile
#define ___must_recompile240 ___must_recompile
#define ___must_recompile241 ___must_recompile
#define ___must_recompile242 ___must_recompile
#define ___must_recompile243 ___must_recompile
#define ___must_recompile244 ___must_recompile
#define ___must_recompile253 ___must_recompile
#define ___must_recompile254 ___must_recompile
#define ___must_recompile255 ___must_recompile
#define ___must_recompile256 ___must_recompile
#define ___must_recompile259 ___must_recompile
#define ___must_recompile262 ___must_recompile
#define ___must_recompile267 ___must_recompile
#define ___must_recompile273 ___must_recompile
#define ___must_recompile274 ___must_recompile
#define ___must_recompile275 ___must_recompile
#define ___must_recompile277 ___must_recompile
#define ___must_recompile279 ___must_recompile
#define ___must_recompile386 ___must_recompile
#define ___must_recompile387 ___must_recompile
#define ___must_recompile388 ___must_recompile
#define ___must_recompile389 ___must_recompile
#define ___must_recompile390 ___must_recompile
#define ___must_recompile447 ___must_recompile
#define ___must_recompile448 ___must_recompile
#define ___must_recompile449 ___must_recompile
#define ___must_recompile450 ___must_recompile
#define ___must_recompile502 ___must_recompile
#define ___must_recompile503 ___must_recompile
#define ___must_recompile504 ___must_recompile
#define ___must_recompile505 ___must_recompile
#define ___must_recompile506 ___must_recompile
#define ___must_recompile507 ___must_recompile
#define ___must_recompile520 ___must_recompile
#define ___must_recompile521 ___must_recompile
#define ___must_recompile523 ___must_recompile
#define ___must_recompile524 ___must_recompile
#define ___must_recompile525 ___must_recompile
#define ___must_recompile526 ___must_recompile
#define ___must_recompile527 ___must_recompile
#define ___must_recompile528 ___must_recompile
#define ___must_recompile529 ___must_recompile
#define ___must_recompile530 ___must_recompile
#define ___must_recompile570 ___must_recompile


#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec) .long trace_##func
#else
#define SYSTEM_CALL(func, vec) .long _##func
#endif
#include <sys/syscall.def>
#undef SYSTEM_CALL

   .long   -1

#ifdef TRACE_LIBRARY

#define SYSTEM_CALL(func, vec) \
trace_##func: ;                                                   \
        moveml d0/d1/a0/a1,sp@-;                                 \
        /* Create a stable symbol reference without CPP concat bugs */ \
        .set __ix_target, _##func ;                              \
        pea __ix_target ;                                        \
        pea vec ;                                                \
        jsr _trace_entry ;                                       \
        lea sp@(8),sp ;                                          \
        tstl d0 ;                                                \
        beq 1f ;                                                 \
        moveml sp@+,d0/d1/a0/a1 ;                                \
        movel sp@(-20),d0 ;                                      \
        rts ;                                                    \
1:      moveml sp@+,d0/d1/a0/a1 ;                                \
        jmp _##func ;

#include <sys/syscall.def>
#undef SYSTEM_CALL
#endif

dataTable:
	INITBYTE (LN_TYPE, 		NT_LIBRARY)
	INITLONG (LN_NAME, 		ixName)
	INITBYTE (IXBASE_FLAGS, 	0x6) |LIBF_CHANGED_SUMUSED
	INITWORD (IXBASE_VERSION, 	IX_VERSION)
	INITWORD (IXBASE_REVISION, 	IX_REVISION)
	INITLONG (IXBASE_IDSTRING, 	idString)
	.long   0

_initRoutine:

   movel   a5,sp@-
   movel   d0,a5

   movel   a0,a5@(IXBASE_SEGLIST)

   pea	   a5@
   jsr	   _ix_init_glue
   addqw   #4,sp

   movel   sp@+,a5
   rts

_libOpen:

   addqw   #1,a6@(IXBASE_OPENCNT)

   bclr    #LIBB_DELEXP,a6@(IXBASE_FLAGS)

   pea	   a6@
   jsr	   _ix_open
   addqw   #4,sp
   beq     OpenFailed

   rts

OpenFailed:
   subqw  #1,a6@(IXBASE_OPENCNT)
   rts

_libClose:

   pea	   a6@
   jsr	   _ix_close
   addqw   #4,sp

   subqw   #1,a6@(IXBASE_OPENCNT)

   bne     Null

   btst    #LIBB_DELEXP,a6@(IXBASE_FLAGS)
   bne     _libExpunge

Null:
   moveq   #0,d0
   rts

_libExpunge:
   moveml  a2/a5/a6,sp@-
   movel   a6,a5

   subal   a2,a2
   bset    #LIBB_DELEXP,a5@(IXBASE_FLAGS)

   tstw    a5@(IXBASE_OPENCNT)
   bne     L21

   movel   a5@(IXBASE_SEGLIST),a2
   movel   4:w,a6

   movel   a5,a1
   jsr	   a6@(_LVORemove)

   pea	   a5@
   jsr	   _ix_expunge
   addqw   #4,sp

   movel   a5,a1
   moveq   #0,d0
   movew   a5@(IXBASE_NEGSIZE),d0
   subl    d0,a1
   addw    a5@(IXBASE_POSSIZE),d0
   jsr	   a6@(_LVOFreeMem)

L21:
   movel   a2,d0

   moveml  sp@+,a2/a5/a6
   rts
