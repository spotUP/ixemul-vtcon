#include "a4.h"		/* for the A4 macro */

asm("\n\
	.text\n\
	.even\n\
	.globl	___sub_d0_sp\n\
	.globl	___move_d0_sp\n\
	.globl	___unlk_a5_rts\n\
\n\
___sub_d0_sp:\n\
	movel	sp@+,a0\n\
	movel	sp,d1\n\
	subl	d0,d1\n\
	cmpl	"A4(___stk_limit)",d1\n\
	jcc	l0\n\
	jbsr	___stkext\n\
l0:	subl	d0,sp\n\
	jmp	a0@\n\
\n\
___move_d0_sp:\n\
	jra	___stkrst\n\
\n\
___unlk_a5_rts:\n\
	movel	d0,a0\n\
	movel	a5,d0\n\
	jbsr	___stkrst\n\
	movel	a0,d0\n\
	movel	sp@+,a5\n\
	rts\n\
");
