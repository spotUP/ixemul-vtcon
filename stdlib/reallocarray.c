/*	$OpenBSD: reallocarray.c,v 1.2 2014/12/08 03:45:00 bcook Exp $	*/
/*
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Revision 1.2  2026/07/13  ChatGPT/JJ
 * Imported and adapted the OpenBSD reallocarray() overflow check
 * for ixemul. Retained usetup and replaced SIZE_MAX with
 * (size_t)-1 for compatibility with older C environments.
 *
 * Revision 1.1  2026/07/06  ChatGPT/JJ
 * Added reallocarray() overflow-safe realloc wrapper for ixemul.
 */


#include "ixemul.h"
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

/*
 * If both operands are smaller than this value, their product
 * cannot overflow size_t.
 */
#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

void *
reallocarray(void *ptr, size_t nmemb, size_t size)
{
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
      nmemb != 0 && size > (size_t)-1 / nmemb)
    {
      usetup;
      
      errno = ENOMEM;
      return NULL;
    }

  return realloc(ptr, nmemb * size);
}
