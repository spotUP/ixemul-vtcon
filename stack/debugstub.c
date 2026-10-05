/*
 *  This file is part of the ixemul package for the Amiga.
 *  Copyright (C) 1994 Rafael W. Luebbert
 *  Copyright (C) 1997 Hans Verkuil
 *
 *  This source is placed in the public domain.
 */

asm("\n\
	.globl	_KPrintF\n\
\n\
KPutChar:\n\
	movel	a6,sp@-\n\
	movel	4:W,a6\n\
	jsr	a6@(-516:W)\n\
	movel	sp@+,a6\n\
	rts\n\
\n\
KDoFmt:\n\
	movel	a6,sp@-\n\
	movel	4:W,a6\n\
	jsr	a6@(-522:W)\n\
	movel	sp@+,a6\n\
	rts\n\
\n\
_KPrintF:\n\
	lea	sp@(4),a1\n\
	movel	a1@+,a0\n\
	movel	a2,sp@-\n\
	lea	KPutChar,a2\n\
	jbsr	KDoFmt\n\
	movel	sp@+,a2\n\
	rts\n\
");
