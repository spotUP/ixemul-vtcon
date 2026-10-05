#include "a4.h"		/* for the A4 macro */

/*
 * Special glue that doesn't clobber any registers.
 */
asm("\n\
  	.globl	___stkovf\n\
___stkovf:\n\
	movel	"A4(_ixemulbase)",sp@-\n\
	addl	#-6*481-24,sp@\n\
	rts\n\
\n\
	.globl	___stkext\n\
___stkext:\n\
	movel	"A4(_ixemulbase)",sp@-\n\
	addl	#-6*482-24,sp@\n\
	rts\n\
\n\
  	.globl	___stkext_f\n\
___stkext_f:\n\
	movel	"A4(_ixemulbase)",sp@-\n\
	addl	#-6*483-24,sp@\n\
  	rts\n\
  \n\
  	.globl	___stkrst\n\
___stkrst:\n\
	movel	"A4(_ixemulbase)",sp@-\n\
	addl	#-6*484-24,sp@\n\
	rts\n\
\n\
	.globl	___stkext_startup\n\
___stkext_startup:\n\
	movel	"A4(_ixemulbase)",sp@-\n\
	addl	#-6*571-24,sp@\n\
	rts\n\
");
