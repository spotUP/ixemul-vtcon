/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 * Revision 1.7  2026/06/15  ChatGPT modifications (JJ)
 *
 *  Reduced repeated pathname scans in open().
 *
 *  Added a small open_path_info helper to classify pathnames once.
 *  Replaced repeated strchr()/strcmp()/strcasecmp() checks with
 *  cached pathname flags.
 *  Reused the precomputed pathname length for f_name storage.
 *
 *  Preserved /dev/tty, console:, *, PTY and normal __open() behavior.
 *
 *  No change to open/stat ordering, descriptor semantics, PTY lifecycle,
 *  directory conversion, O_CASE handling or deferred metadata updates.
 * 
 *  Revision 1.6 2026/06/07  Copilot modifications (JJ)
 *    -Fixed open() to seek to file end for O_APPEND,
 *     ensure seek occurs after O_TRUNC.
 *    -Fixed open(NULL, ...) to set errno = EACCES and return -1
 *     instead of returning EACCES as a file descriptor value.
 *
 *  Revision 1.5  2026/05/31  ChatGPT modifications (JJ)
 *  - Replaced heap-only f_name allocation with per-file inline name buffer.
 *  - Short pathnames are stored in struct file::f_name_buf.
 *  - Longer pathnames continue to use kmalloc().
 *  - Added f_name_inline ownership flag initialization after falloc().
 *  - Fixed PTY pathname construction to use writable stack storage instead
 *    of modifying a string literal.
 *  - No change to open(), stat(), PTY state, or descriptor semantics.
 *
 *  $Id: open.c,v 1.4 1994/06/19 15:14:07 rluebbert Exp $
 *
 *  $Log: open.c,v $
 *  Revision 1.4  1994/06/19  15:14:07  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/28  00:32:04  mwild
 *  pass convert_dir the original signal mask, to check for pending signals
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <string.h>

/* standard functions.. could get overridden in a network environment */
extern int __ioctl(), __fselect(), __close();

/* "normal" functions, means do half-async writes & sync reads */
extern int __write(), __read(), __open();

/* incore functions */
extern int __mread(), __mclose(), __mselect();

static struct ix_mutex open_sem;

struct open_path_info
{
  size_t len;
  int has_colon;
  int is_dev_tty;
  int is_star;
  int is_console;
};

static void
open_parse_path(char *name, struct open_path_info *pi)
{
  char *p;

  pi->len = 0;
  pi->has_colon = 0;
  pi->is_dev_tty = 0;
  pi->is_star = 0;
  pi->is_console = 0;

  p = name;
  while (*p)
    {
      if (*p == ':')
        pi->has_colon = 1;
      p++;
    }

  pi->len = (size_t)(p - name);

  if (pi->len == 8 && !strcmp(name, "/dev/tty"))
    pi->is_dev_tty = 1;

  if (pi->len == 1 && name[0] == '*')
    pi->is_star = 1;

  if (pi->len == 8 && !strcasecmp(name, "console:"))
    pi->is_console = 1;
}

int
open(char *name, int mode, int perms)
{
  int fd;
  struct file *f;
  BPTR fh;
  int late_stat;
  int omask, error;
  int amode = 0, i;
  char ptyname[32];
  struct open_path_info pathinfo;
  size_t final_namelen;
  int use_direct_open;
  usetup;

  if (name == NULL)     /* sanity check */
    {
      errno = EACCES;
      return -1;
    }

  open_parse_path(name, &pathinfo);
  final_namelen = pathinfo.len + 1;
  use_direct_open = pathinfo.is_star || pathinfo.is_console;

  mode = FFLAGS(mode);

  /* inhibit signals */
  omask = syscall (SYS_sigsetmask, ~0);

  error = falloc (&f, &fd);
  if (error)
    {
      syscall (SYS_sigsetmask, omask);
      errno = error;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
  /* we now got the file, ie. since its count is > 0, no other process
   * will get it with falloc() */

  /* initialize inline filename ownership state */
  f->f_name = 0;
  f->f_name_inline = 0;

  late_stat = 0;

  /* The code between the stat() and the actual open() is critical */
  ix_mutex_lock(&open_sem);

  if (stat(name, &f->f_stb) < 0)
    {
      /* there can mainly be two reasons for stat() to fail. Either the
       * file really doesn't exist (ENOENT), or then the filesystem/handler
       * doesn't support file locks. */

      /* if we should get out of here without an error, init the stat
       * buffer after having opened the file with __fstat, this sets some
       * reasonable parameters (see end of function). */
      late_stat = 1;

      if ((errno == ENOENT) && (mode & O_CREAT))
	{
	  /* can't set permissions on an open file, so this has to be done
	   * by 'close' */
	  f->f_stb.st_mode = (perms & ~u.u_cmask);
	  f->f_stb_dirty |= FSDF_MODE;

          if (!muBase)
            {
              f->f_stb.st_uid = geteuid();
              f->f_stb.st_gid = getegid();
              if (f->f_stb.st_uid != (uid_t)(-2) ||
                  f->f_stb.st_gid != (gid_t)(-2))
                f->f_stb_dirty |= FSDF_OWNER;
            }
	}
    }

  f->f_flags = mode & FMASK;
  f->f_ttyflags = IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;

  /* initialise the packet. The only thing needed at this time is its
   * header, filling in of port, action & args will be done when it's
   * used */
  __init_std_packet (&f->f_sp);
  __init_std_packet ((void *)&f->f_select_sp);

  /* check for case-sensitive filename */
  if ((mode & O_CASE) && !late_stat && !pathinfo.has_colon && filenamecmp(name))
    {
      error = ENOENT;
      goto error;
    }

  /* ok, so lets try to open the file... */

  /* do this *only* if the stat() above was successful !! */
  if (!late_stat && S_ISDIR (f->f_stb.st_mode) && !(mode & FWRITE))
    {
      if (convert_dir (f, name, omask))
        {
          ix_mutex_unlock(&open_sem);
          goto ret_ok;
        }
      else
        {
	  goto error;
	}
    }

  /* filter invalid modes */
  switch (mode & (O_CREAT|O_TRUNC|O_EXCL))
    {
    case O_EXCL:
    case O_EXCL|O_TRUNC:
      /* can never succeed ! */
      error = EINVAL;
      goto error;

    case O_CREAT|O_EXCL:
    case O_CREAT|O_EXCL|O_TRUNC:
      if (! late_stat)
        {
	  error = EEXIST;
	  goto error;
	}
      break;
    }

  amode = (mode & O_CREAT) ? MODE_READWRITE : MODE_OLDFILE;

  if (pathinfo.is_dev_tty && u.u_session && u.u_session->s_ttyname[0])
    {
      /* the session's controlling terminal (TIOCSCTTY): a pty slave */
      name = u.u_session->s_ttyname;
      final_namelen = strlen(name) + 1;
      use_direct_open = 0;
    }
  else if (pathinfo.is_dev_tty)
    {
      name = "*";
      final_namelen = 2;     /* "*" plus NUL */
      use_direct_open = 1;
    }
  else if ((i = is_pseudoterminal(name)))
    {
      /* vtcon's PTY: (UP-Term): /dev/ptyXY is the master PTY:XY/m and
         /dev/ttyXY its slave PTY:XY/s, with a real line discipline
         between them. The handler allows one master per pair, so a
         program scanning for a free pty sees the busy ones fail. (The
         FIFO: mapping this replaces wrote the name into a string literal
         that every opener shared.) */
      strcpy(ptyname, "PTY:XY/m");
      ptyname[4] = name[i + 3];
      ptyname[5] = name[i + 4];
      if (name[i] == 't')
        ptyname[7] = 's';
      name = ptyname;
      final_namelen = sizeof("PTY:XY/m");
      use_direct_open = 0;
    }

  do
  {
    if (use_direct_open)
      /* Temporary patch for KingCON 1.3, which seems to have problems with
	 ACTION_FINDINPUT of "*"/"console:" when "dp_Port" of the packet is
	 not set to sender's "pr_MsgPort" - that's what IXEmul makes on
	 clients' startup during initialization of "stderr". */
      fh = Open(name, amode);
    else
      fh = __open (name, amode);

    if (! fh)
      {
        int err = IoErr();

        /* For those handlers that do not understand MODE_READWRITE (e.g. PAR: ) */
        if (err == ERROR_ACTION_NOT_KNOWN && amode == MODE_READWRITE)
        {
          amode = MODE_NEWFILE;
        }
        else
        {
          error = __ioerr_to_errno (err);
          goto error;
        }
      }
  } while (!fh);

  /* End of critical section */
  ix_mutex_unlock(&open_sem);

  /* now.. we're lucky, we actually opened the file! */
  f->f_fh = (struct FileHandle *) BTOCPTR(fh);

  if (mode & FWRITE)
    f->f_write  = __write;
  if (mode & FREAD)
    f->f_read   = __read;

  f->f_ioctl  = __ioctl;
  f->f_select = __fselect;
  f->f_close  = __close;
  f->f_type   = DTYPE_FILE;

  /*
 * Store the filename in the per-file inline buffer when it fits.
 * Longer names use kmalloc(), because the file object may be shared.
 */
  {
    size_t namelen = final_namelen;

    if (namelen <= sizeof(f->f_name_buf))
      {
        f->f_name = f->f_name_buf;
        f->f_name_inline = 1;
      }
    else
      {
        f->f_name = kmalloc(namelen);
        f->f_name_inline = 0;
      }

    if (f->f_name)
      strcpy(f->f_name, name);
  }

ret_ok:
  /* ok, we're almost done. If desired, init the stat buffer to the
   * information we can get from an open file descriptor */
  if (late_stat) __fstat (f);
  
  /* if the file qualifies, try to change it into a DTYPE_MEM file */
  if (!late_stat && f->f_type == DTYPE_FILE 
      && f->f_stb.st_size < ix.ix_membuf_limit && mode == FREAD)
    {
      void *buf;
      
      /* try to obtain the needed memory */
      buf = (void *) kmalloc (f->f_stb.st_size);
      if (buf)
	if (syscall (SYS_read, fd, buf, f->f_stb.st_size) == f->f_stb.st_size)
	  {
	    __Close (CTOBPTR (f->f_fh));
	    f->f_type 		= DTYPE_MEM;
	    f->f_mf.mf_offset 	= 0;
	    f->f_mf.mf_buffer 	= buf;
	    f->f_read		= __mread;
	    f->f_close 		= __mclose;
	    f->f_ioctl		= 0;
	    f->f_select		= __mselect;
	  }
	else
	  kfree (buf);
    }

  syscall (SYS_sigsetmask, omask);

  if (error)
    {
      errno = error;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }
  if (!error && (mode & O_TRUNC) && (amode != MODE_NEWFILE))
    {
      syscall(SYS_ftruncate, fd, 0);
      f->f_stb_dirty |= FSDF_UTIME;
    }

  /* Honor O_APPEND: seek to end after any truncation. */
  if (!error && (mode & O_APPEND) && f->f_type == DTYPE_FILE)
    {
      Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
    }

  /* return the descriptor */
  return fd;

error:
  /* End of critical section */
  ix_mutex_unlock(&open_sem);

  /* free the file */
  u.u_ofile[fd] = 0;
  f->f_count--;
  syscall (SYS_sigsetmask, omask);
  errno = error;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
