/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Ray Burr
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
 */

/* Written and Copyright by Ray Burr. 
 * Put under the GNU Library General Public License.
 * Thanks Ray !
 */

/*
 * Revision 1.1.1.1  2026/06/16  JJ/ChatGPT
 *
 * Updated getwd.c/getcwd() for safer and slightly faster current
 * directory resolution.
 *
 * - Moved signal masking from _GetPathFromLock() to _get_pwd(), so
 *   GetCurrentDirName(), NameFromLock() and the fallback lock-walk all
 *   run under the same signal mask; _GetPathFromLock() no longer keeps
 *   a private omask.
 * - Added ERANGE handling for too-small buffers in root, non-process
 *   and fallback path-construction cases.
 * - Fixed slash-translation buffer accounting by reducing buffer_length
 *   after reserving one byte for the leading slash.
 * - Added getcwd(..., 0) rejection with EINVAL.
 * - Fixed getcwd(NULL, size) error cleanup so internally allocated
 *   buffers are freed if _get_pwd() fails.
 * - Guarded use of pr_CurrentDir to avoid passing a NULL current-directory
 *   lock to NameFromLock() or the fallback _GetPathFromLock() path.
 * - Added DupLock() failure handling in _GetPathFromLock() to avoid
 *   dereferencing a NULL path pointer when the fallback lock-walk cannot start.
 * - Limited ERROR_LINE_TOO_LONG handling to the no-current-lock case so
 *   explicit errno values from the fallback lock-walk are not overwritten.
 *
 * ABI unchanged.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <string.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* _GetPathFromLock - Given a pointer to an AmigaOS Lock structure, build
   a rooted path string in a buffer of a given size.  The buffer should be
   at least 'buffer_length' bytes.  Returns a pointer to the result
   or NULL on an error.  'errno' will be set in the case of an error.  This
   function returns the path string based at 'buffer[0]' but it can alter
   any part of the buffer.  */

static char *
_GetPathFromLock (BPTR lock, char *buffer, int buffer_length)
{
  usetup;
  char *p;
  BPTR fl, next_fl;
  int length;
  int ioerr;
  struct FileInfoBlock *fib;
  char *result = 0;

  /* Allocate space on stack for fib. */

  BYTE fib_Block[sizeof (struct FileInfoBlock) + 2];

  /* Make sure fib is longword aligned. */

  fib = (struct FileInfoBlock *) fib_Block;
  if (((ULONG) fib & 0x02) != 0)
    fib = (struct FileInfoBlock *) ((ULONG) fib + 2);

  p = 0L;

  /* Duplicate the lock so that the directory structure can't change
     while we're doing this. */

  fl = DupLock (lock);
  if (fl == 0L)
    {
      ioerr = IoErr();
      errno = ioerr ? __ioerr_to_errno(ioerr) : ENOENT;
      KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
      goto ret;
    }

  /* Follow the chain of directories and build the name in 'buffer' */

  while (fl != 0L)
    {
      if (Examine (fl, fib) == DOSFALSE)
	{
	  errno = __ioerr_to_errno(IoErr());
	  KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));

	  UnLock (fl);
	  goto ret;
	}
      next_fl = ParentDir (fl);
      UnLock (fl);
      if (p != 0L)
	{
	  if (next_fl != 0L)
	    *--p = '/';
	  else
	    *--p = ':';
	}
      else
	{
	  p = buffer + buffer_length - 1;	/* fix 20-jan-92 ## mw */
	  *p = '\0';
	  if (next_fl == 0L)
	    *--p = ':';
	}
      length = strlen (fib->fib_FileName);
      p -= length;
      if (p <= buffer)
	{
	  if (next_fl != 0L)
	    UnLock (next_fl);
	  errno = ERANGE;
	  goto ret;
	}
      memcpy (p, fib->fib_FileName, length);
      fl = next_fl;
    }

  /* Move the pathname so that it starts at buffer[0]. */

  bcopy (p, buffer, strlen (p) + 1);

  result = buffer;

ret:
  return result;
}


static char *
_get_pwd (char *buffer, int buffer_length)
{
  usetup;
  struct Process *proc;
  char *result, *colon;
  int omask;
  BPTR cwdlock;

  if (u.u_is_root)
    {
      if (buffer_length < 2)
	{
	  errno = ERANGE;
	  return 0L;
	}
      strcpy(buffer, "/");
      return buffer;
    }

  proc = (struct Process *) FindTask(0L);

  /* Just return an empty string if this is not a process. */

  if (proc == 0L || proc->pr_Task.tc_Node.ln_Type != NT_PROCESS)
    {
      if (buffer_length < 1)
	{
	  errno = ERANGE;
	  return 0L;
	}
      buffer[0] = '\0';
      return buffer;
    }

  /* make room for slash */
  if (ix.ix_flags & ix_translate_slash)
    {
      if (buffer_length < 2)
	{
	  errno = ERANGE;
	  return 0L;
	}

      buffer++;
      buffer_length--;
    }

  omask = syscall (SYS_sigsetmask, ~0);
  cwdlock = (BPTR)proc->pr_CurrentDir;

  if (GetCurrentDirName (buffer, buffer_length) ||
      (cwdlock != 0L && NameFromLock(cwdlock, buffer, buffer_length)))
    {
      result = buffer;
    }
  else if (cwdlock != 0L)
    {
      /* and as the last chance resort to the 1.3 algorithm */

      result = _GetPathFromLock(cwdlock, buffer, buffer_length);
    }
  else
    {
      if (IoErr() == ERROR_LINE_TOO_LONG)
	errno = ERANGE;
      else
	errno = ENOENT;
      result = 0L;
    }

  syscall (SYS_sigsetmask, omask);

  if ((ix.ix_flags & ix_translate_slash) && result)
    {
      colon = strchr (result, ':');
      if (colon)
        {
	  *colon = '/';
          result--;
          result[0] = '/';
	  return result;
        }

       bcopy (result, result - 1, strlen (result) + 1);
       return result - 1;
    }

  return result;
}


char *
getwd (char *buffer)
{
  usetup;
  char *path;

  path = _get_pwd (buffer, MAXPATHLEN);
  if (path == 0L)
    {
      strcpy (buffer, "getwd - ");
      strcat (buffer, strerror (errno));
      return 0L;
    }

  return path;
}

char *
getcwd (char *buffer, size_t buffer_length)
{
  int allocated = 0;
  char *res;
  usetup;

  if (buffer_length == 0)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0L;
    }

  if (buffer == 0L)
    {
      buffer = (char *) syscall (SYS_malloc, buffer_length + 2);
      if (buffer == 0L) { errno = ENOMEM; return 0L; }
      allocated = 1;
    }

  res = _get_pwd (buffer, buffer_length);

  if (res == 0L && allocated)
    {
      int saved_errno = errno;

      syscall (SYS_free, buffer);
      errno = saved_errno;
    }

  return res;
}