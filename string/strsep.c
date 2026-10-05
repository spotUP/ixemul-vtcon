/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 * strsep.c,v
 *
 * Revision 1.1  2026/08/11  ChatGPT modifications (JJ)
 *
 *    Replace repeated delimiter rescans with direct fast paths for empty
 *    and one- through four-character delimiter sets.  For larger sets,
 *    build a compact 256-bit membership table so each input byte can be
 *    tested in constant time while preserving strsep() token semantics.
 */

#include <sys/cdefs.h>
#include <string.h>
#include <stdio.h>

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "@(#)strsep.c	5.4 (Berkeley) 1/26/91";
#endif /* LIBC_SCCS and not lint */

/*
 * Get next token from string *stringp, where tokens are nonempty
 * strings separated by characters from delim.  
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strtoken returns NULL.
 */

char *
strsep(stringp, delim)
	register char **stringp;
	register const char *delim;
{
	register unsigned char *p;
	register unsigned char c;
	unsigned char d0, d1, d2, d3;
	unsigned char map[32];
	static const unsigned char bitmask[8] = {
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
	};
	register int i;
	char *tok;

	if ((tok = *stringp) == NULL)
		return (NULL);

	p = (unsigned char *)tok;
	d0 = (unsigned char)delim[0];

	/*
	 * Empty delimiter string: the complete remainder is one token.
	 */
	if (d0 == 0) {
		while (*p != 0)
			p++;
		*stringp = NULL;
		return (tok);
	}

	/*
	 * One-character delimiter set.
	 */
	d1 = (unsigned char)delim[1];
	if (d1 == 0) {
		while ((c = *p) != 0) {
			if (c == d0) {
				*p++ = 0;
				*stringp = (char *)p;
				return (tok);
			}
			p++;
		}
		*stringp = NULL;
		return (tok);
	}

	/*
	 * Two-character delimiter set.
	 */
	d2 = (unsigned char)delim[2];
	if (d2 == 0) {
		while ((c = *p) != 0) {
			if (c == d0 || c == d1) {
				*p++ = 0;
				*stringp = (char *)p;
				return (tok);
			}
			p++;
		}
		*stringp = NULL;
		return (tok);
	}

	/*
	 * Three-character delimiter set.
	 */
	d3 = (unsigned char)delim[3];
	if (d3 == 0) {
		while ((c = *p) != 0) {
			if (c == d0 || c == d1 || c == d2) {
				*p++ = 0;
				*stringp = (char *)p;
				return (tok);
			}
			p++;
		}
		*stringp = NULL;
		return (tok);
	}

	/*
	 * Four-character delimiter set.
	 */
	if ((unsigned char)delim[4] == 0) {
		while ((c = *p) != 0) {
			if (c == d0 || c == d1 || c == d2 || c == d3) {
				*p++ = 0;
				*stringp = (char *)p;
				return (tok);
			}
			p++;
		}
		*stringp = NULL;
		return (tok);
	}

	/*
	 * Larger delimiter sets: build one membership table for this call.
	 */
	for (i = 0; i < 32; i++)
		map[i] = 0;

	p = (unsigned char *)delim;
	while ((c = *p++) != 0)
		map[c >> 3] |= bitmask[c & 7];

	p = (unsigned char *)tok;
	while ((c = *p) != 0) {
		if ((map[c >> 3] & bitmask[c & 7]) != 0) {
			*p++ = 0;
			*stringp = (char *)p;
			return (tok);
		}
		p++;
	}

	*stringp = NULL;
	return (tok);
}
