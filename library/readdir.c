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
 *
 *
 *  * Revision 1.1.1.3  2026/06/01  Copilot modification (JJ)
 *
 * Fix three directory entry validation bugs:
 *   - Reject name length 0 to avoid d_name[-1] undefined behavior.
 *   - Reject name length >= sizeof(d_name) to prevent buffer overflow.
 *   - Require full read of name field; partial reads are treated as error.
 *
 * No functional changes for valid directory entries.
 *
 * Revision 1.1.1.2  2026/05/31  ChatGPT moddification (JJ)
 *
 * Harden directory entry parsing.
 *
 *   - Validate directory entry name length before use.
 *   - Reject malformed or oversized directory records.
 *   - Require complete reads of fixed-size directory fields.
 *   - Prevent out-of-bounds access when processing entry names.
 * Reduce syscall overhead in readdir().
 *
 *   -Read the fixed-size directory entry header with a single
 *    SYS_read() call instead of three separate 4-byte reads.
 *
 * No functional changes for valid directory entries.
 *
 *  readdir.c,v 1.1.1.1 1994/04/04 04:30:51 amiga Exp
 *
 *  readdir.c,v
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

struct dirent *
readdir (DIR *dp)
{
  usetup;
  u_int hdr[3];
  u_int l;
  int n;

  if (! dp || dp->dd_fd < 0) return 0;

  /* Read fileno, typeflag, namelen in one syscall */
  n = syscall (SYS_read, dp->dd_fd, hdr, sizeof hdr);
  if (n != sizeof hdr)
    {
      /* n == 0 -> EOF, normal */
      if (n > 0)
        errno = EIO;   /* partial header -> corrupt entry */
      return 0;
    }

  dp->dd_ent.d_fileno = hdr[0];
  dp->dd_ent.d_type   = (hdr[1] ? DT_DIR : DT_REG);
  l = hdr[2];

  /*
   * BUGFIX 1+2:
   *  - Prevent UB when l == 0 (would access d_name[-1])
   *  - Prevent overflow when l >= sizeof(d_name)
   */
  if (l == 0 || l >= sizeof(dp->dd_ent.d_name)) {
      errno = EIO;
      return 0;
  }

  /* read name */
  /*
   * BUGFIX 3:
   * Require full read of name field.
   */
  n = syscall (SYS_read, dp->dd_fd, dp->dd_ent.d_name, l);
  if (n != (int) l)
    {
      /*
       * n == -1 ? errno already set by SYS_read
       * n >= 0  ? partial read -> corrupt entry
       */
      if (n >= 0)
        errno = EIO;
      return 0;
    }

  dp->dd_ent.d_name[l] = 0;

  /* Compute namlen: exclude trailing NUL if present */
  dp->dd_ent.d_namlen = l;
  if (dp->dd_ent.d_name[l - 1] == '\0')
    dp->dd_ent.d_namlen--;

  dp->dd_ent.d_reclen = l + 8;
  return & dp->dd_ent;
}
