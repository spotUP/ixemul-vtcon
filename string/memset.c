/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)memset.c	5.6 (Berkeley) 1/26/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/cdefs.h>
#include <string.h>

/* Modified 2026-10-19 by spotUP for UP-Term (ixemul-vtcon): the BSD byte
 * loop cost 66 ms for 100 KB on an A1200 (memcpy is CopyMem: 4 ms). Head
 * bytes up to a longword boundary, then 32 bytes per pass as longword
 * stores (unsigned int: 32 bits in ixemul), then the longword and byte
 * tails. Longword-aligned stores are safe on every flavor of the library including 68000 (word alignment
 * suffices there). Same semantics: returns dst, c is taken as unsigned
 * char, n == 0 writes nothing. */
void *
memset(dst, c, n)
	void *dst;
	register int c;
	register size_t n;
{
	register unsigned char *d = dst;

	if (n >= 8) {
		register unsigned int v = (unsigned char)c;
		register size_t head = (- (unsigned long)d) & 3;

		v |= v << 8;
		v |= v << 16;
		n -= head;
		while (head != 0) {
			*d++ = (unsigned char)c;
			head--;
		}
		{
			register unsigned int *l = (unsigned int *)d;
			register size_t blocks = n >> 5;

			while (blocks != 0) {
				l[0] = v; l[1] = v; l[2] = v; l[3] = v;
				l[4] = v; l[5] = v; l[6] = v; l[7] = v;
				l += 8;
				blocks--;
			}
			n &= 31;
			blocks = n >> 2;
			while (blocks != 0) {
				*l++ = v;
				blocks--;
			}
			n &= 3;
			d = (unsigned char *)l;
		}
	}
	while (n != 0) {
		*d++ = (unsigned char)c;
		n--;
	}
	return (dst);
}
