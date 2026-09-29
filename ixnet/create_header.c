/* ix_internals.h for ixnet, from the target's struct layout: compiled to
 * assembly by the cross compiler, never run (see library/create_header.c). */
#define _KERNEL
#include "ixnet.h"
#include <stddef.h>

#define DEFINE_PLUS(sym, base, val) \
  asm volatile ("\n->" #sym " (" #base " + %c0)" : : "i" ((long)(val)))

void create_header(void)
{
  DEFINE_PLUS(IXNETBASE_SIZEOF, IXNETBASE_C_PRIVATE,
	      sizeof (struct ixnet_base) - offsetof (struct ixnet_base, ix_seg_list) - 4);
}
