/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996  Hans Verkuil
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

/*
 * mmap.c,v
 *
 * Revision 1.4  2026/09/05  ChatGPT modifications (JJ)
 *
 *    Match madvise(), mprotect() and msync() return types and argument
 *    types to the public <sys/mman.h> declarations.
 *    Reject zero-length mappings instead of passing size zero to malloc().
 *    Clear newly allocated mapping memory so MAP_ANON mappings start
 *    zero-filled and short file reads cannot expose stale heap contents.
 *    Check lseek(), read() and write() failures and always restore the
 *    original file offset after file-backed mapping or synchronization.
 *    Handle partial read() and write() results.
 *    Use off_t rather than int for saved file offsets in msync().
 *    Make address-range lookup overflow-safe.
 *    Reject partial mprotect() and munmap() operations that the existing
 *    single-record mapping representation cannot implement correctly.
 *    Make msync() synchronize only the requested subrange.
 *
 *  Revision 1.3  2026/06/07  Copilot/ChatGPT modifications (JJ)
 *
 *  - Fix mmap(): MAP_FIXED error returned NULL instead of MAP_FAILED.
 *    Return (caddr_t)-1 for this error path, consistent with the
 *    other mmap() error returns in this file.
 *
 *  Revision 1.2  2026/04/30  JJ
 *
 *  - Added missing headers <unistd.h> and <stdlib.h> for lseek(),
 *    malloc() and free(). No functional changes.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

extern int read(), write();

static int
read_mapping_data(int fd, void *buf, size_t len, off_t offset)
{
  off_t curoff;
  size_t done;
  int n;
  int chunk;
  int saved_errno;
  usetup;

  curoff = lseek(fd, 0, SEEK_CUR);
  if (curoff == (off_t)-1)
    return -1;

  if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
    return -1;

  done = 0;
  saved_errno = 0;

  while (done < len)
    {
      if (len - done > (size_t)INT_MAX)
        chunk = INT_MAX;
      else
        chunk = (int)(len - done);

      n = read(fd, (char *)buf + done, chunk);
      if (n < 0)
        {
          saved_errno = errno;
          break;
        }

      if (n == 0)
        break;

      done += (size_t)n;
    }

  if (lseek(fd, curoff, SEEK_SET) == (off_t)-1 && saved_errno == 0)
    saved_errno = errno;

  if (saved_errno)
    {
      errno = saved_errno;
      return -1;
    }

  return 0;
}

static int
write_mapping_data(int fd, const void *buf, size_t len, off_t offset)
{
  off_t curoff;
  size_t done;
  int n;
  int chunk;
  int saved_errno;
  usetup;

  curoff = lseek(fd, 0, SEEK_CUR);
  if (curoff == (off_t)-1)
    return -1;

  if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
    return -1;

  done = 0;
  saved_errno = 0;

  while (done < len)
    {
      if (len - done > (size_t)INT_MAX)
        chunk = INT_MAX;
      else
        chunk = (int)(len - done);

      n = write(fd, (const char *)buf + done, chunk);
      if (n < 0)
        {
          saved_errno = errno;
          break;
        }

      if (n == 0)
        {
          saved_errno = EIO;
          break;
        }

      done += (size_t)n;
    }

  if (lseek(fd, curoff, SEEK_SET) == (off_t)-1 && saved_errno == 0)
    saved_errno = errno;

  if (saved_errno)
    {
      errno = saved_errno;
      return -1;
    }

  return 0;
}

caddr_t
mmap(caddr_t addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  struct mmap_mem *m;
  usetup;

  if (len == 0)
    {
      errno = EINVAL;
      return (caddr_t)-1;
    }

  if (flags & MAP_FIXED)
    {
      errno = ENOMEM;
      return (caddr_t)-1;
    }

  if (!(flags & MAP_ANON) && (fd < 0 || fd >= NOFILE || !u.u_ofile[fd]))
    {
      errno = EBADF;
      return (caddr_t)-1;
    }

  m = malloc(sizeof(struct mmap_mem));
  if (m == NULL)
    {
      errno = ENOMEM;
      return (caddr_t)-1;
    }

  m->addr = malloc(len);
  if (m->addr == NULL)
    {
      free(m);
      errno = ENOMEM;
      return (caddr_t)-1;
    }

  bzero(m->addr, len);

  if (!(flags & MAP_ANON))
    {
      if (read_mapping_data(fd, m->addr, len, offset) < 0)
        {
          free(m->addr);
          free(m);
          return (caddr_t)-1;
        }
    }

  m->length = len;
  m->prot = prot;
  m->fd = fd;
  m->flags = flags;
  m->offset = offset;
  m->next = u.u_mmap;
  u.u_mmap = m;

  return m->addr;
}

static struct mmap_mem *
find_mmap_range(caddr_t addr, size_t len)
{
  struct mmap_mem *m;
  unsigned long p;
  unsigned long base;
  size_t off;
  usetup;

  p = (unsigned long)addr;

  for (m = u.u_mmap; m; m = m->next)
    {
      base = (unsigned long)m->addr;

      if (p < base)
        continue;

      off = (size_t)(p - base);
      if (off > m->length)
        continue;

      if (len <= m->length - off)
        return m;
    }

  return NULL;
}

/* NetBSD also doesn't support this */
int
madvise(caddr_t addr, size_t len, int behav)
{
  (void)addr;
  (void)len;
  (void)behav;
  return 0;
}

int
mlock(caddr_t addr, size_t len)
{
  (void)addr;
  (void)len;
  return 0;
}

int
munlock(caddr_t addr, size_t len)
{
  (void)addr;
  (void)len;
  return 0;
}

int
mprotect(caddr_t addr, size_t len, int prot)
{
  struct mmap_mem *m;
  usetup;

  if (len == 0)
    {
      errno = EINVAL;
      return -1;
    }

  m = find_mmap_range(addr, len);
  if (!m || addr != (caddr_t)m->addr || len != m->length)
    {
      errno = EINVAL;
      return -1;
    }

  m->prot = prot;
  return 0;
}

int
msync(caddr_t addr, size_t len)
{
  struct mmap_mem *m;
  size_t off;
  off_t file_off;
  usetup;

  if (len == 0)
    {
      errno = EINVAL;
      return -1;
    }

  m = find_mmap_range(addr, len);
  if (!m)
    {
      errno = EINVAL;
      return -1;
    }

  if ((m->flags & MAP_ANON) ||
      !(m->flags & MAP_SHARED) ||
      !(m->prot & PROT_WRITE))
    return 0;

  off = (size_t)((unsigned long)addr - (unsigned long)m->addr);
  file_off = m->offset + (off_t)off;

  return write_mapping_data(m->fd, addr, len, file_off);
}

int
munmap(caddr_t addr, size_t len)
{
  struct mmap_mem *m;
  struct mmap_mem *prev;
  usetup;

  if (len == 0)
    {
      errno = EINVAL;
      return -1;
    }

  prev = NULL;
  for (m = u.u_mmap; m; m = m->next)
    {
      if ((caddr_t)m->addr == addr)
        break;
      prev = m;
    }

  if (!m || len != m->length)
    {
      errno = EINVAL;
      return -1;
    }

  if (msync(addr, len) < 0)
    return -1;

  if (prev)
    prev->next = m->next;
  else
    u.u_mmap = m->next;

  free(m->addr);
  free(m);
  return 0;
}
