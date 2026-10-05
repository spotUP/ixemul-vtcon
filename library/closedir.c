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
 *  closedir.c,v 1.1.1.2  2026/06/01  Copilot modification (JJ)
 *
 *  Fix closedir() resource handling and error reporting:
 *
 *    - Always free DIR when dp != NULL; only close() when dd_fd is valid.
 *    - Set errno = EBADF and return -1 when called with a NULL DIR pointer.
 *    - Remove misleading dd_fd poisoning; DIR is freed unconditionally.
 *
 *  No functional changes for valid DIR handles; external API and observable
 *  behaviour remain unchanged.
 *
 *  closedir.c,v 1.1.1.1 1994/04/04 04:30:51 amiga Exp
 *
 *  closedir.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:51  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/stat.h>
#include <dirent.h>

int
closedir (DIR *dp)
{
  usetup;
  int res = 0;

  /*
   * POSIX: closedir() on a NULL DIR* is an error. Report EBADF and
   * return -1. No resources to free in this case.
   */
  if (dp == NULL)
    {
      errno = EBADF;
      return -1;
    }

  if (dp->dd_fd >= 0)
    res = syscall (SYS_close, dp->dd_fd);

  syscall (SYS_free, dp);
  return res;
}
