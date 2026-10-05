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
 */

/* 1.3  2026/06/13  ChatGPT modifications  (JJ)
 *      Fixed chroot() so the target directory is locked and resolved
 *      before chdir(dir).  This avoids resolving a relative chroot path
 *      again after the current directory has already changed.
 *      Use S_ISDIR() when checking the stat mode in chdir(),
 *      instead of testing S_IFDIR bits directly.
 *      Added defensive errno fallbacks in chdir() error paths so
 *      failures cannot return -1 with errno left as zero.
 *      Bounded the copy into u.u_root_directory after successful chroot().
 *
 *  1.2  2026/06/07  Copilot modifications (JJ)
 *      Added missing __unlock(newlock) in chdir() chroot-protection
 *      error paths, and guarded __unlock(rootlock) in chroot() so
 *      rootlock is only released when __lock(dir,ACCESS_READ) succeeds.
 *
 *  1.1  2026/05/31  Copilot modifications (JJ)
 *      Updated chdir() and helpers with improved path handling and
 *      directory resolution. Added per-process path buffer usage,
 *      enhanced dirisparent() validity checks, early NULL/empty path
 *      rejection, and consistent error/IoErr handling.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

/* this one is for Mike B. Smith ;-) */

void set_dir_name_from_lock(BPTR lock)
{
  char *buf;
  usetup;

  /* Prefer per-process path buffer if free, else fallback to kmalloc(). */
  if (!u.u_path_buf_in_use) {
    buf = u.u_path_buf;
    u.u_path_buf_in_use = 1;
  } else {
    buf = (char *) kmalloc(MAXPATHLEN);
  }

  if (buf) {
    /* NOTE: This shortcuts any symlinks. But then, Unix does the
     *       same, and a shell that wants to be smart about symlinks
     *       has to track chdir()s itself as well. */
    if (NameFromLock(lock, buf, MAXPATHLEN))
      SetCurrentDirName(buf);

    /* Release buffer */
    if (buf == u.u_path_buf)
      u.u_path_buf_in_use = 0;
    else
      kfree(buf);
  }
}

/*
 * Checks if pathname "name1" is above
 * "name" in directory structure.
 * Lock()/Unlock() are used instead of __lock/__unlock
 * because this routine seems to loop if name2 is a subdir of name1
 */
static short
dirisparent (char *name1, char *name2)
{
    short ret = 0;
    BPTR lock1;

    lock1 = Lock (name1, SHARED_LOCK);
    if (lock1) {
	BPTR lock2;

	lock2 = Lock(name2, SHARED_LOCK);
	if (lock2) {
	    switch (SameLock (lock1, lock2)) {

		case LOCK_DIFFERENT:
		break;

		case LOCK_SAME:
		    ret = 2;
		break;

		case LOCK_SAME_VOLUME:
		{
		    BPTR l;

		    while (lock2) {
			l = lock2;
			lock2 = ParentDir (l);
			UnLock(l);
			/* Avoid SameLock(NULL) */
			if (lock2 && SameLock(lock1, lock2) == LOCK_SAME) {
			    ret = 1;
			    break;
			}
		    }
		    break;
		}
	    }
	    /* Only unlock if still valid */
	    if (lock2)
	      UnLock(lock2);
	}
	UnLock (lock1);
    }
    return ret;
}

/* if we change our directory, we have to remember the original cd, when
 * the process was started, because we're not allowed to unlock this
 * lock, since we didn't obtain it. */

int chdir (char *path)
{
  BPTR oldlock, newlock;
  int error = 0;
  int omask;
  int ioerr;
  struct stat stb;
  usetup;

  /* Reject NULL or empty path */
  if (!path || !*path) {
    errno = ENOENT;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return -1;
  }

  /* Sigh... CurrentDir() is a DOS-library function, it would probably be
   * ok to just use pr_CurrentDir, but alas, this way we're conformant to
   * programming style guidelines, but we pay the overhead of locking dosbase
   */

  /* chdir("/") with u.u_root_directory set, chdir's to root directory */
  if (!strcmp("/",path) && *u.u_root_directory)
    path = u.u_root_directory;

  if (syscall (SYS_stat, path, &stb) == 0 && !S_ISDIR(stb.st_mode))
  {
    errno = ENOTDIR;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return -1;
  }

  omask = syscall (SYS_sigsetmask, ~0);

  newlock = __lock (path, ACCESS_READ);
  ioerr = newlock ? 0 : IoErr();

  if (newlock == NULL && ioerr == 6262)
    {
      u.u_is_root = 1;
      SetCurrentDirName("/");
      syscall (SYS_sigsetmask, omask);
      return 0;
    }
  else if (newlock)
    {
      /* chroot() support - don't do a chdir if the path
       * is a parent directory of the root directory
       */
      if (*u.u_root_directory) {
	char dir[MAXPATHLEN];

	if (NameFromLock (newlock, dir, MAXPATHLEN)) {
	  if (dirisparent(dir,u.u_root_directory) == 1) {
	    error = EACCES;   /* set error, chdirerr will assign errno */
	    __unlock (newlock);
	    goto chdirerr;
	  }
	}
	else {
	  error = __ioerr_to_errno (IoErr ());
	  if (!error)
	    error = EIO;
	  __unlock (newlock);
	  goto chdirerr;
	}
      }

      u.u_is_root = 0;
      oldlock = CurrentDir (newlock);

      if (u.u_startup_cd == (BPTR)-1)
        u.u_startup_cd = oldlock;
      else
        __unlock (oldlock);

      set_dir_name_from_lock(newlock);

      syscall (SYS_sigsetmask, omask);
      return 0;
    }
  error = __ioerr_to_errno (ioerr);
  if (!error)
    error = ENOENT;

chdirerr:
  syscall (SYS_sigsetmask, omask);
  errno = error;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

/* change the "root" directory to dir */
int chroot(char *dir)
{
    int retval, error;
    int i;
    BPTR rootlock;
    char rootdir[MAXPATHLEN];
    usetup;

    if (!dir || !*dir) {
	errno = ENOENT;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return -1;
    }

    /*
     * Resolve the new root before chdir(dir).  If dir is relative,
     * resolving it after chdir() would look it up relative to the new
     * current directory instead of the caller's original one.
     */
    rootlock = __lock(dir, ACCESS_READ);
    if (!rootlock) {
	error = __ioerr_to_errno(IoErr());
	if (!error)
	    error = ENOENT;
	errno = error;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return -1;
    }

    if (!NameFromLock(rootlock, rootdir, MAXPATHLEN)) {
	error = __ioerr_to_errno(IoErr());
	if (!error)
	    error = EIO;
	__unlock(rootlock);
	errno = error;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return -1;
    }

    __unlock(rootlock);

    retval = chdir(dir);

    if (retval == 0)
    {
	for (i = 0; i < MAXPATHLEN - 1 && rootdir[i]; i++)
	    u.u_root_directory[i] = rootdir[i];
	u.u_root_directory[i] = '\0';
    }

    return retval;
}
