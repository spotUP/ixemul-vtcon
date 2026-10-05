/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
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

/*
 * strpbrk.c,v
 *
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Add direct fast paths for one- through four-character search sets.
 *    For larger sets, build a compact 256-bit membership table so each
 *    input byte can be tested in constant time instead of rescanning s2.
 *    Preserve byte-oriented semantics for all unsigned character values.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strpbrk.c	5.8 (Berkeley) 1/26/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/cdefs.h>
#include <string.h>

/*
 * Find the first occurrence in s1 of a character in s2 (excluding NUL).
 */
char *
strpbrk(s1, s2)
	register const char *s1, *s2;
{
	register const unsigned char *p;
	register unsigned char c;
	unsigned char c0, c1, c2, c3;
	unsigned char map[32];
	static const unsigned char bitmask[8] = {
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
	};
	register int i;

	c0 = (unsigned char)s2[0];
	if (c0 == 0)
		return (NULL);

	c1 = (unsigned char)s2[1];
	if (c1 == 0) {
		p = (const unsigned char *)s1;
		while ((c = *p) != 0) {
			if (c == c0)
				return ((char *)p);
			p++;
		}
		return (NULL);
	}

	c2 = (unsigned char)s2[2];
	if (c2 == 0) {
		p = (const unsigned char *)s1;
		while ((c = *p) != 0) {
			if (c == c0 || c == c1)
				return ((char *)p);
			p++;
		}
		return (NULL);
	}

	c3 = (unsigned char)s2[3];
	if (c3 == 0) {
		p = (const unsigned char *)s1;
		while ((c = *p) != 0) {
			if (c == c0 || c == c1 || c == c2)
				return ((char *)p);
			p++;
		}
		return (NULL);
	}

	if ((unsigned char)s2[4] == 0) {
		p = (const unsigned char *)s1;
		while ((c = *p) != 0) {
			if (c == c0 || c == c1 || c == c2 || c == c3)
				return ((char *)p);
			p++;
		}
		return (NULL);
	}

	for (i = 0; i < 32; i++)
		map[i] = 0;

	p = (const unsigned char *)s2;
	while ((c = *p++) != 0)
		map[c >> 3] |= bitmask[c & 7];

	p = (const unsigned char *)s1;
	while ((c = *p) != 0) {
		if (map[c >> 3] & bitmask[c & 7])
			return ((char *)p);
		p++;
	}

	return (NULL);
}
