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
 * Revision 1.5  2026/06/29  ChatGPT modifications  (JJ)
 *
 * Replaced the original small-block buddy backend with an Exec PoolMem
 * based backend while preserving the b_alloc()/b_free() interface and
 * the historical small/large allocation split.
 *
 * - Uses separate private/public PoolMem pools.
 * - Uses two PoolMem size classes, 1K and 2K, selected by total block size.
 * - Keeps direct AllocMem()/FreeMem() for blocks larger than the pooled range.
 * - Adds a small internal PoolMem header for validation and correct FreePooled().
 *
 * No public ABI change.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <exec/memory.h>
#include <stddef.h>
#include <limits.h>

#define MINLOG2         4
#define MINSIZE         (1 << MINLOG2)

#define MAXLOG2         15
#define MAXSIZE         (1 << MAXLOG2)

#define BUDDY_LIMIT     (1 << (MAXLOG2 - 5))   /* 1024 */

#define PRIVATE_POOL    0
#define PUBLIC_POOL     1
#define NUMPOOLS        2

#define BPOOL_CLASS_1K      0
#define BPOOL_CLASS_2K      1
#define BPOOL_NUM_CLASSES   2

#define BPOOL_LIMIT_1K      BUDDY_LIMIT        /* 1024 */
#define BPOOL_LIMIT_2K      (BUDDY_LIMIT << 1) /* 2048 */

#define BPOOL_MAGIC     0x42504f4cUL  /* 'BPOL' */
#define BPOOL_ALIGN     8
#define BPOOL_MASK      (BPOOL_ALIGN - 1)
#define BPOOL_HDR_SIZE  ((sizeof (struct bpool_hdr) + BPOOL_MASK) & ~BPOOL_MASK)

struct bpool_hdr {
  u_int magic;
  u_int pool;
  u_int size;
  u_int total;
};

static void *poolmem_pool[NUMPOOLS][BPOOL_NUM_CLASSES];
static struct ix_mutex poolmem_private_sem;

/*
 * PRIVATE_POOL is protected by ix_mutex. PUBLIC_POOL keeps the
 * historical Forbid()/Permit() protection, since public allocations
 * may occur in contexts where semaphore waiting would be unsafe.
 */

void cleanup_buddy (void);

static int
bpool_class_for_total (size_t total)
{
  if (total <= BPOOL_LIMIT_1K)
    return BPOOL_CLASS_1K;

  if (total <= BPOOL_LIMIT_2K)
    return BPOOL_CLASS_2K;

  return -1;
}

void
init_buddy (void)
{
  poolmem_pool[PRIVATE_POOL][BPOOL_CLASS_1K] =
    CreatePool (0, MAXSIZE, BPOOL_LIMIT_1K);

  poolmem_pool[PRIVATE_POOL][BPOOL_CLASS_2K] =
    CreatePool (0, MAXSIZE, BPOOL_LIMIT_2K);

  poolmem_pool[PUBLIC_POOL][BPOOL_CLASS_1K] =
    CreatePool (MEMF_PUBLIC, MAXSIZE, BPOOL_LIMIT_1K);

  poolmem_pool[PUBLIC_POOL][BPOOL_CLASS_2K] =
    CreatePool (MEMF_PUBLIC, MAXSIZE, BPOOL_LIMIT_2K);

  if (!poolmem_pool[PRIVATE_POOL][BPOOL_CLASS_1K] ||
      !poolmem_pool[PRIVATE_POOL][BPOOL_CLASS_2K] ||
      !poolmem_pool[PUBLIC_POOL][BPOOL_CLASS_1K] ||
      !poolmem_pool[PUBLIC_POOL][BPOOL_CLASS_2K])
    {
      cleanup_buddy ();
      ix_panic ("poolmem allocator: failed to create memory pools!");
      Wait (0);
    }
}

/*
 * b_alloc()/b_free() are size-aware internal allocation primitives.
 * The caller must pass the same size to b_free() that was used for
 * b_alloc(). Sizes smaller than MINSIZE are normalized internally.
 *
 * Blocks whose total size, including bpool_hdr, fits within the 1K/2K
 * classes are allocated from PoolMem. Larger blocks are passed directly
 * to AllocMem()/FreeMem().
 */

void *
b_alloc (int size, unsigned pool)
{
  struct bpool_hdr *hdr;
  size_t total;
  void *mem;
  void *ppool;
  int bclass;

  if (size < 0)
    return 0;

  pool = (pool & MEMF_PUBLIC) ? PUBLIC_POOL : PRIVATE_POOL;

  if (size < MINSIZE)
    size = MINSIZE;

  if ((size_t)size > (size_t)-1 - BPOOL_HDR_SIZE)
    return 0;

  total = (size_t)size + BPOOL_HDR_SIZE;
  if (total > (size_t)UINT_MAX)
    return 0;

  bclass = bpool_class_for_total (total);
  if (bclass < 0)
    return AllocMem (size, pool == PUBLIC_POOL ? MEMF_PUBLIC : 0);

  ppool = poolmem_pool[pool][bclass];
  if (!ppool)
    return 0;

  if (pool == PRIVATE_POOL)
    {
      ix_mutex_lock (&poolmem_private_sem);
      hdr = (struct bpool_hdr *)AllocPooled (ppool, total);
      ix_mutex_unlock (&poolmem_private_sem);
    }
  else
    {
      Forbid ();
      hdr = (struct bpool_hdr *)AllocPooled (ppool, total);
      Permit ();
    }

  if (!hdr)
    return 0;

  hdr->magic = BPOOL_MAGIC;
  hdr->pool = pool;
  hdr->size = size;
  hdr->total = total;

  mem = (void *)((u_char *)hdr + BPOOL_HDR_SIZE);
  return mem;
}

void
b_free (void *mem, int size)
{
  struct bpool_hdr *hdr;
  unsigned pool;
  void *ppool;
  size_t total;
  int bclass;

  if (!mem)
    return;

  if (size < 0)
    return;

  if (size < MINSIZE)
    size = MINSIZE;

  if ((size_t)size > (size_t)-1 - BPOOL_HDR_SIZE)
    {
      FreeMem (mem, size);
      return;
    }

  total = (size_t)size + BPOOL_HDR_SIZE;
  if (total > (size_t)UINT_MAX)
    {
      FreeMem (mem, size);
      return;
    }

  bclass = bpool_class_for_total (total);
  if (bclass < 0)
    {
      FreeMem (mem, size);
      return;
    }

  hdr = (struct bpool_hdr *)((u_char *)mem - BPOOL_HDR_SIZE);

  if (hdr->magic != BPOOL_MAGIC)
    {
      ix_panic ("poolmem allocator: corrupt block header!");
      Wait (0);
    }

  pool = hdr->pool;
  if (pool >= NUMPOOLS)
    {
      ix_panic ("poolmem allocator: corrupt pool id!");
      Wait (0);
    }

  if (hdr->total < BPOOL_HDR_SIZE ||
      hdr->size != hdr->total - BPOOL_HDR_SIZE)
    {
      ix_panic ("poolmem allocator: corrupt block size!");
      Wait (0);
    }

  hdr->magic = 0;

  bclass = bpool_class_for_total (hdr->total);
  if (bclass < 0)
    {
      ix_panic ("poolmem allocator: corrupt pooled block size!");
      Wait (0);
    }

  ppool = poolmem_pool[pool][bclass];
  if (!ppool)
    {
      ix_panic ("poolmem allocator: missing pool!");
      Wait (0);
    }

  if (pool == PRIVATE_POOL)
    {
      ix_mutex_lock (&poolmem_private_sem);
      FreePooled (ppool, hdr, hdr->total);
      ix_mutex_unlock (&poolmem_private_sem);
    }
  else
    {
      Forbid ();
      FreePooled (ppool, hdr, hdr->total);
      Permit ();
    }
}

void
cleanup_buddy (void)
{
  int p, c;

  for (p = 0; p < NUMPOOLS; p++)
    {
      for (c = 0; c < BPOOL_NUM_CLASSES; c++)
        {
          if (poolmem_pool[p][c])
            {
              DeletePool (poolmem_pool[p][c]);
              poolmem_pool[p][c] = 0;
            }
        }
    }
}
