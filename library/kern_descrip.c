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
 * Revision 1.10  2026/07/28  ChatGPT modifications (JJ)
 *
 * - Restored the historical ixemul close-handler ownership model in dup2().
 * - Removed the manual pre-decrement of oldf->f_count.
 * - Close the replaced descriptor reference through __close_file_ref()
 *   after releasing ix_lock_base(), so the file-specific close handler owns
 *   the reference decrement and final resource cleanup.
 * - Preserve descriptor replacement ordering and dup2() errno semantics.
 *
 * Revision 1.9  2026/07/27  ChatGPT modifications (JJ)
 *
 * - Reset f_fs_blocksize in falloc() whenever a file-table slot is
 *   allocated or reused, preventing stale filesystem block-size data.
 *
 * Revision 1.8  2026/07/07  ChatGPT modifications (JJ)
 *
 * - Implemented ixemul-internal advisory flock().
 * - Supports LOCK_SH, LOCK_EX, LOCK_UN and LOCK_NB.
 * - Uses struct file ownership so duplicated descriptors share flock state.
 * - Uses f_name as the advisory lock key for DTYPE_FILE/DTYPE_MEM files.
 * - Blocking flock waits on ix_flock_list and is woken by unlock, final close
 *   or lock type changes.
 * - This does not implement fcntl() record locks or native AmigaDOS locks.
 *
 * Revision 1.7  2026/07/01  ChatGPT modifications (JJ)
 * 
 * - Added ixemul-internal advisory flock state and __flock_close().
 * - Locks are owned by struct file so duplicated descriptors share one
 *   flock owner and cleanup happens only on final close.
 * - flock() itself is still the historical stub in this revision.
 * 
 * - Changed dup2() to drop the replaced descriptor's file reference under
 *   ix_lock_base(), but call the close handler only after releasing the lock.
 * - Reuses __close_file_ref() so close() and dup2() now share the same
 *   final-reference close path.
 * 
 * Revision 1.6  2026/06/26  ChatGPT modifications (JJ)
 * - Harden dup() descriptor handling by protecting socket rollback and
 *   normal descriptor installation/f_count updates with ix_lock_base().
 * - Keep F_DUPFD's final u.u_lastfile update inside the same critical
 *   section as u.u_ofile[], u.u_pofile[] and f_count updates.
 * - Clear f_stb in falloc() so reused file-table slots cannot expose stale
 *   cached stat data.
 * - Preserve dup2() close semantics: descriptor replacement remains protected
 *   by ix_lock_base(), while f_close() is still called outside the lock
 *   without a manual pre-decrement of f_count.
 * - Leave DTYPE_USOCKET on the existing non-NET__dup path.
 * 
 * Revision 1.5  2026/06/13  ChatGPT modifications (JJ)
 * - Cleared stale descriptor state in falloc() by resetting f_flags,
 *   file operation handlers and f_sync_flags when reusing a file slot.
 * - Reworked dup2() replacement order so descriptor j is overwritten under
 *   ix_lock_base() before the old file is closed.  This keeps f_close()
 *   outside the global lock while avoiding a window where descriptor j
 *   appears free and could be reused reentrantly.
 * - Validated negative file descriptors before indexing u.u_ofile[] in
 *   dup(), dup2(), fcntl() and fstat().  Also fix fstat() debug logging
 *   so errno is set before KPRINTF().
 * - This supersedes the earlier Revision 1.3 dup2() detach/reinstall
 *   strategy.
 * 
 * Revision 1.4  2026/06/07  Copilot modifications (JJ)
 * Fix dup()/dup2() prototypes to match unistd.h (int).
 *
 * Revision 1.3  2026/06/02  Copilot modfication (JJ)
 *
 * Fix dup2() locking and descriptor handling.
 *
 * The original implementation called f_close() while holding ix_lock_base(),
 * which could deadlock when the close operation triggered filesystem or
 * socket callbacks that also acquire ix_lock_base() or other global locks.
 *
 * The new implementation:
 *   - Detaches descriptor j under ix_lock_base() without calling f_close().
 *   - Updates u.u_lastfile correctly when j was the highest descriptor.
 *   - Releases ix_lock_base() before calling f_close() on the old file.
 *   - Reacquires ix_lock_base() to install the new descriptor j.
 *
 * This preserves dup2() descriptor semantics for normal ixemul use,
 * avoids calling f_close() while ix_lock_base() is held, and keeps
 * per-process descriptor state consistent.
 *
 *  kern_descrip.c,v 1.1.1.1 1994/04/04 04:30:42 amiga Exp
 *
 *  kern_descrip.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:42  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/07/04  19:19:04  mwild
 *  add support for F_INTERNALIZE/F_EXTERNALIZE, which are used by IXPIPE
 *  and execve().
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 *
 *  Since the code originated from Berkeley, the following copyright
 *  header applies as well. The code has been changed, it's not the
 *  original Berkeley code!
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  @(#)kern_descrip.c  7.16 (Berkeley) 6/28/90
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>

/*
 * Descriptor management.
 */

/*
 * ixemul-internal advisory flock state.
 *
 * Locks are owned by struct file, not by descriptor number.  This matches
 * dup()/dup2()/F_DUPFD sharing: duplicated descriptors point at the same
 * struct file and only the final close releases the lock.
 */
struct ix_flock {
  struct ixnode node;
  char *name;
  struct file *owner;
  int type;
};

#define IX_FLOCK_SH 1
#define IX_FLOCK_EX 2

#ifndef LOCK_SH
#define LOCK_SH 0x01
#endif
#ifndef LOCK_EX
#define LOCK_EX 0x02
#endif
#ifndef LOCK_NB
#define LOCK_NB 0x04
#endif
#ifndef LOCK_UN
#define LOCK_UN 0x08
#endif

static struct ixlist ix_flock_list;
static int ix_flock_inited = 0;

static void
ix_flock_init(void)
{
  Forbid();
  if (!ix_flock_inited)
    {
      ixnewlist(&ix_flock_list);
      ix_flock_inited = 1;
    }
  Permit();
}

static const char *
ix_flock_key(struct file *fp)
{
  if (!fp)
    return NULL;

  /*
   * DTYPE_MEM is included because ixemul may turn a small read-only
   * DTYPE_FILE into an in-memory file after open().  It still represents
   * the same opened pathname and keeps f_name.
   */
  if (fp->f_type != DTYPE_FILE && fp->f_type != DTYPE_MEM)
    return NULL;

  if (!fp->f_name || fp->f_name[0] == '\0')
    return NULL;

  return fp->f_name;
}

static struct ix_flock *
ix_flock_find_owner(struct file *owner)
{
  struct ix_flock *fl;

  for (fl = (struct ix_flock *)ix_flock_list.head; fl;
       fl = (struct ix_flock *)fl->node.next)
    {
      if (fl->owner == owner)
        return fl;
    }

  return NULL;
}

static int
ix_flock_conflict(const char *name, struct file *owner, int type)
{
  struct ix_flock *fl;

  for (fl = (struct ix_flock *)ix_flock_list.head; fl;
       fl = (struct ix_flock *)fl->node.next)
    {
      if (fl->owner == owner)
        continue;

      if (strcmp(fl->name, name) != 0)
        continue;

      /*
       * Shared locks conflict only with exclusive locks.
       * Exclusive locks conflict with any other lock.
       */
      if (type == IX_FLOCK_EX || fl->type == IX_FLOCK_EX)
        return 1;
    }

  return 0;
}

static void
ix_flock_unlock_owner(struct file *owner)
{
  struct ix_flock *fl;

  if (!owner)
    return;

  for (;;)
    {
      Forbid();

      for (fl = (struct ix_flock *)ix_flock_list.head; fl;
           fl = (struct ix_flock *)fl->node.next)
        {
          if (fl->owner == owner)
            break;
        }

      if (!fl)
        {
          Permit();
          break;
        }

      ixremove(&ix_flock_list, (struct ixnode *)fl);
      ix_wakeup((u_int)&ix_flock_list);
      Permit();

      if (fl->name)
        kfree(fl->name);
      kfree(fl);
    }
}

void
__flock_close(struct file *fp)
{
  if (!fp || !ix_flock_inited)
    return;

  ix_flock_unlock_owner(fp);
}

/*
 * System calls on descriptors.
 */
/* ARGSUSED */
int
getdtablesize (void)
{
  /* This was NOFILE, but getdtablesize is also used to determine the number
     number of filehandles select() should test. And if you pass a value
     larger than FD_SETSIZE to select(), you'll get fireworks. And it is very
     hard to discover why your program won't work. I know: I've been through
     this process twice now, so I thought I'd better fix this here by just
     returning FD_SETSIZE. 
     
     I could also change the FD_SETSIZE macro to the value of NOFILE (changing
     256 to 512), but that would be a waste of memory, and besides, in the
     future the number of filehandles might become dynamic, so it wouldn't work
     in any case. */
  return FD_SETSIZE;
}

/*
 * Duplicate a file descriptor.
 */
/* ARGSUSED */
int
dup (int i)
{
  struct file *fp;
  int fd, error;
  usetup;

  if (i < 0 || i >= NOFILE || (fp = u.u_ofile[i]) == NULL)
    errno_return(EBADF, -1);

  if (fp->f_type == DTYPE_SOCKET)
    {
      struct file *fp2;
      int fd2;
      int err;

      if ((err = falloc (&fp2, &fd2)))
        errno_return(err, -1);

      fp2->f_so = netcall(NET__dup, fp);
      if (fp2->f_so == -1)
      {
        /* free the allocated fd */
        ix_lock_base ();
        u.u_ofile[fd2] = 0;
        fp2->f_count = 0;
        ix_unlock_base ();
        return -1;
      }
      fp2->f_socket_domain = fp->f_socket_domain;
      fp2->f_socket_type = fp->f_socket_type;
      fp2->f_socket_protocol = fp->f_socket_protocol;
      _set_socket_params(fp2, fp->f_socket_domain, fp->f_socket_type, fp->f_socket_protocol);
      return fd2;
    }

  if ((error = ufalloc (0, &fd)))
    errno_return(error, -1);

  ix_lock_base ();
  u.u_ofile[fd] = fp;
  u.u_pofile[fd] = u.u_pofile[i] &~ UF_EXCLOSE;
  fp->f_count++;

  if (fd > u.u_lastfile)
    u.u_lastfile = fd;
  ix_unlock_base ();

  return fd;
}

/*
 * Duplicate a file descriptor to a particular value.
 */
/* ARGSUSED */
int
dup2 (int i, int j)
{
  register struct file *fp;
  int old_err;
  struct file *oldf;   /* C89: declarations at top */
  usetup;

  if (i < 0 || i >= NOFILE || (fp = u.u_ofile[i]) == NULL)
    errno_return(EBADF, -1);

  if (j < 0 || j >= NOFILE)
    errno_return(EBADF, -1);

  if (i == j) return (int)j;

  old_err = errno;

  /*
   * Replace descriptor j while holding ix_lock_base(), but defer
   * f_close() until after the lock is released.  This avoids calling
   * handler close code under the global ixemul lock while also avoiding
   * an interval where descriptor j appears free.
   */
  ix_lock_base ();
  oldf = u.u_ofile[j];
  u.u_ofile[j] = fp;
  u.u_pofile[j] = u.u_pofile[i] &~ UF_EXCLOSE;
  fp->f_count++;

  if (j > u.u_lastfile)
    u.u_lastfile = j;
  ix_unlock_base ();

  /*
   * Preserve the historical ixemul ownership model.  The file-specific
   * close handler owns the f_count decrement and final resource cleanup.
   * __close_file_ref() must therefore be called without pre-decrementing
   * oldf->f_count.
   */
  if (oldf)
    (void)__close_file_ref (oldf);

  /*
   * dup2() must suceed even though the close had an error.
   */
  errno = old_err;  /* XXX */
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return (int)j;
}

/*
 * The file control system call.
 */
/* ARGSUSED */
int
fcntl(int fdes, int cmd, int arg)
{
  register struct file *fp;
  register char *pop;
  int i, error;
  usetup;

  /* F_INTERNALIZE doesn't need a valid descriptor. Check for this first */
  if (cmd == F_INTERNALIZE)
    {
      if ((error = ufalloc (0, &i)))
        {
          errno = error;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }
      fp = (struct file *) arg;
      u.u_ofile[i] = fp;
      u.u_pofile[i] = 0;
      fp->f_count++;
      return i;
    }


  if (fdes < 0 || fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  pop = &u.u_pofile[fdes];
  switch (cmd) 
    {
      case F_DUPFD:
    if (arg < 0 || arg >= NOFILE)
      {
        errno = EINVAL; 
        KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
        return -1;
      }
    if ((error = ufalloc (arg, &i)))
      {
        errno = error;
        KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
        return -1;
      }
    ix_lock_base ();
    u.u_ofile[i] = fp;
    u.u_pofile[i] = *pop &~ UF_EXCLOSE;
    fp->f_count++;
    if (i > u.u_lastfile)
      u.u_lastfile = i;
    ix_unlock_base ();
    return i;

      case F_GETFD:
    return *pop & 1;

      case F_SETFD:
    *pop = (*pop &~ 1) | (arg & 1);
    return 0;

      case F_GETFL:
    return OFLAGS(fp->f_flags);

      case F_SETFL:
    fp->f_flags &= ~FCNTLFLAGS;
    fp->f_flags |= FFLAGS(arg) & FCNTLFLAGS;
    if (fp->f_type == DTYPE_SOCKET)
    {
      arg = (fp->f_flags & FASYNC) ? 1 : 0;
      ioctl(fdes, FIOASYNC, &arg);
      arg = (fp->f_flags & FNONBLOCK) ? 1 : 0;
      ioctl(fdes, FIONBIO, &arg);
    }
    return 0;

      case F_EXTERNALIZE:
        return (int)fp;

      default:
        errno = EINVAL;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return -1;
    }
    /* NOTREACHED */
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
fstat (int fdes, struct stat *sb)
{
  struct file *fp;
  usetup;

  if (fdes < 0 || fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  /* I found this code from the AmiTCP sdk. It *should* work with AS225 */
  if (fp->f_type == DTYPE_SOCKET && u.u_ixnetbase)
    return netcall(NET__fstat, fp, sb);

  /* if this file is writable, then it's quite probably that the stat buffer
     information stored at open time is no longer valid. So if the file really
     is a file, update that information */
  if ((fp->f_flags & FWRITE) && fp->f_type == DTYPE_FILE)
    __fstat (fp);

  *sb = fp->f_stb;
  return 0;
}

/*
 * Allocate a user file descriptor.
 */
int
ufalloc(int want, int *result)
{
  usetup;

  for (; want < NOFILE; want++) 
    {
      if (u.u_ofile[want] == NULL) 
        {
      u.u_pofile[want] = 0;
      if (want > u.u_lastfile) u.u_lastfile = want;
            
      *result = want;
      return 0;
    }
    }
  return EMFILE;
}

/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
int
falloc(struct file **resultfp, int *resultfd)
{
  register struct file *fp;
  int error, i;
  usetup;

  if ((error = ufalloc(0, &i)))
    return (error);

  ix_lock_base ();

  if (ix.ix_lastf == 0)
    ix.ix_lastf = ix.ix_file_tab;

  for (fp = ix.ix_lastf; fp < ix.ix_fileNFILE; fp++)
    if (fp->f_count == 0)
      goto slot;

  for (fp = ix.ix_file_tab; fp < ix.ix_lastf; fp++)
    if (fp->f_count == 0)
      goto slot;

  /* YES I know.. it's not optimal, we should resize the table...
   * unfortunately all code accessing file structures will then have
   * to be changed as well, and this is a job for later improvement,
   * first goal is to get this baby working... */
  ix_warning("ixemul.library file table full!");
  error = ENFILE;
  goto do_ret;

slot:
  u.u_ofile[i] = fp;
  memset(&fp->f_stb, 0, sizeof(fp->f_stb));
  fp->f_stb_dirty = 0;
  fp->f_fs_blocksize = 0;
  fp->f_name = 0;
  fp->f_name_inline = 0;   /* ensure clean filename invariant */
  fp->f_flags = 0;
  fp->f_count = 1;
  fp->f_type = 0;       /* inexistant type ;-) */
  fp->f_write = 0;
  fp->f_read = 0;
  fp->f_ioctl = 0;
  fp->f_select = 0;
  fp->f_close = 0;
  fp->f_sync_flags = 0;
  memset(&fp->f__fh, 0, sizeof(fp->f__fh));
  ix.ix_lastf = fp + 1;
  if (resultfp)
    *resultfp = fp;
  if (resultfd)
    *resultfd = i;

  error = 0;

do_ret:
  ix_unlock_base();

  return error;
}


/*
 * Apply an advisory lock on a file descriptor.
 */
int
flock (int fdes, int how)
{
  struct file *fp;
  struct ix_flock *fl;
  struct ix_flock *newfl;
  const char *name;
  char *newname;
  size_t namelen;
  int op;
  int type;
  int err;
  int sleep_rc;
  usetup;

  newfl = NULL;
  newname = NULL;
  err = 0;

  /*
   * Validate operation bits.  LOCK_NB is a modifier; exactly one real
   * operation must remain after removing it.
   */
  if (how & ~(LOCK_SH | LOCK_EX | LOCK_NB | LOCK_UN))
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  op = how & ~LOCK_NB;

  if (op != LOCK_SH && op != LOCK_EX && op != LOCK_UN)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  if (fdes < 0 || fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  ix_flock_init();

  /*
   * Unlock is allowed as a no-op for a valid descriptor.  This keeps
   * LOCK_UN harmless even when no lock was held.
   */
  if (op == LOCK_UN)
    {
      ix_flock_unlock_owner(fp);
      return 0;
    }

  name = ix_flock_key(fp);
  if (!name)
    {
      errno = EOPNOTSUPP;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  type = (op == LOCK_EX) ? IX_FLOCK_EX : IX_FLOCK_SH;

  /*
   * Preallocate outside Forbid().  If this owner already has a lock,
   * the allocation will be discarded after the lock is updated.
   */
  namelen = strlen(name) + 1;
  newname = (char *)kmalloc(namelen);
  if (!newname)
    {
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  strcpy(newname, name);

  newfl = (struct ix_flock *)kmalloc(sizeof(*newfl));
  if (!newfl)
    {
      kfree(newname);
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  /*
   * Keep the file structure stable while a blocking flock sleeps.
   */
  __get_file(fp);

  for (;;)
    {
      Forbid();

      if (!ix_flock_conflict(name, fp, type))
        {
          fl = ix_flock_find_owner(fp);

          if (fl)
            {
              if (fl->type != type)
                {
                  fl->type = type;
                  ix_wakeup((u_int)&ix_flock_list);
                }
              Permit();

              kfree(newname);
              kfree(newfl);
              __release_file(fp);
              return 0;
            }

          newfl->name = newname;
          newfl->owner = fp;
          newfl->type = type;
          ixaddtail(&ix_flock_list, (struct ixnode *)newfl);
          Permit();

          __release_file(fp);
          return 0;
        }

      if (how & LOCK_NB)
        {
          Permit();
          err = EWOULDBLOCK;
          break;
        }

      sleep_rc = ix_sleep((caddr_t)&ix_flock_list, "flock");
      Permit();

      if (sleep_rc < 0)
        {
          setrun(FindTask(0));
          err = EINTR;
          break;
        }
    }

  __release_file(fp);

  if (newname)
    kfree(newname);
  if (newfl)
    kfree(newfl);

  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
