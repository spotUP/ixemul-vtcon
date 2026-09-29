/* ix_internals.h from the target's own struct layouts. Built for a cross
 * build: the cross compiler compiles this to assembly (-S, it never runs);
 * every DEFINE leaves a line "->NAME value" there, which the Makefile
 * turns into "#define NAME value" (the asm-offsets technique). The
 * values are the ones the original create_header program printed. */
#define _KERNEL
#include "ixemul.h"
#include <stddef.h>

#define LIB struct Library
#define TASK struct Task

#define DEFINE(sym, val) \
  asm volatile ("\n->" #sym " %c0" : : "i" ((long)(val)))
#define DEFINE_PLUS(sym, base, val) \
  asm volatile ("\n->" #sym " (" #base " + %c0)" : : "i" ((long)(val)))

void create_header(void)
{
  int extra = 0;

  DEFINE(P_SIGMASK_OFFSET, offsetof (struct user, p_sigmask));
  DEFINE(U_ONSTACK_OFFSET, offsetof (struct user, u_onstack));
  DEFINE(P_FLAG_OFFSET, offsetof (struct user, p_flag));

  DEFINE(IXBASE_FLAGS, offsetof (LIB, lib_Flags));
  DEFINE(IXBASE_NEGSIZE, offsetof (LIB, lib_NegSize));
  DEFINE(IXBASE_POSSIZE, offsetof (LIB, lib_PosSize));
  DEFINE(IXBASE_VERSION, offsetof (LIB, lib_Version));
  DEFINE(IXBASE_REVISION, offsetof (LIB, lib_Revision));
  DEFINE(IXBASE_IDSTRING, offsetof (LIB, lib_IdString));
  DEFINE(IXBASE_SUM, offsetof (LIB, lib_Sum));
  DEFINE(IXBASE_OPENCNT, offsetof (LIB, lib_OpenCnt));
  DEFINE(IXBASE_LIBRARY, sizeof (LIB));

  DEFINE_PLUS(IXBASE_SIZEOF, IXBASE_C_PRIVATE,
	      (sizeof (struct ixemul_base) - offsetof (struct ixemul_base, ix_seg_list) - 4) + extra * 6);
  DEFINE(IXFAKEBASE_SIZE, extra * 6);

#ifdef NOTRAP
  DEFINE(USERPTR_OFFSET, offsetof (TASK, tc_UserData));
#else
  DEFINE(USERPTR_OFFSET, offsetof (TASK, tc_TrapData));
#endif
  DEFINE(IDNESTPTR_OFFSET, offsetof (TASK, tc_IDNestCnt));
  DEFINE(SPREGPTR_OFFSET, offsetof (TASK, tc_SPReg));
}
