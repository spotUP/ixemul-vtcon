/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
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
 *
 *  getrlimit.c,v 1.1.1.1 1994/04/04 04:30:24 amiga Exp
 *
 *  getrlimit.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:24  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 * Revision 1.1.1.2  2026/06/16  JJ/ChatGPT
 *
 * Updated getrlimit() resource validation to match the RLIMIT_* values
 * exported by sys/resource.h.
 *
 * Replaced the RLIMIT_RSS upper-bound check with RLIM_NLIMITS-based
 * validation.
 * Added handling for RLIMIT_MEMLOCK, RLIMIT_NPROC and RLIMIT_OFILE,
 * returning RLIM_INFINITY like the other non-enforced limits.
 * Kept RLIMIT_STACK reporting based on the current task stack bounds,
 * using explicit address arithmetic for the stack-size calculation.
 * Preserved the existing no-enforcement semantics and ABI.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/time.h>
#include <sys/resource.h>

int
getrlimit(int resource, struct rlimit *rlp)
{
  struct Task *me = FindTask(0);
  struct user *u_ptr = getuser(me);

  if (!rlp || resource < 0 || resource >= RLIM_NLIMITS)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  switch (resource)
    {
      case RLIMIT_CPU:
      case RLIMIT_FSIZE:
      case RLIMIT_CORE:
      case RLIMIT_RSS:
      case RLIMIT_DATA:
      case RLIMIT_MEMLOCK:
      case RLIMIT_NPROC:
      case RLIMIT_OFILE:
	rlp->rlim_cur = rlp->rlim_max = RLIM_INFINITY;
	break;
      case RLIMIT_STACK:
        rlp->rlim_cur = rlp->rlim_max =
	  (long)((ULONG)me->tc_SPUpper - (ULONG)me->tc_SPLower);
	break;
      default:
	errno = EINVAL;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return -1;
    }

  return 0;
}
