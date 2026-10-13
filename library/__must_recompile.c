/* Modified 2026-09-29 by spotUP for UP-Term (ixemul-vtcon); see the git history. */
#define _KERNEL
#include <ixemul.h>
#include <stdlib.h>

void __must_recompile (void)
{
  ix_panic ("Obsolete ixemul.library syscall number used.\n\
Relink program with current ixemul crt0.o and libc.a.");
  exit (EXIT_FAILURE);
}
