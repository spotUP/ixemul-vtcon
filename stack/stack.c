#include "ixemul.h"

/* I wish I knew a more elegant way to do this:

   sstr(STACKSIZE) -> str((16384)) -> "(16384)"
*/
#define str(s) #s
#define sstr(s) str(s)

asm("\n\
	.data\n\
	.even\n\
	.globl	___stack\n\
	.ascii	\"StCk\"	| Magic cookie\n\
___stack:\n\
	.long	" sstr(STACKSIZE) "\n\
	.ascii	\"sTcK\"	| Magic cookie\n\
");
