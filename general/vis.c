/*	$OpenBSD: vis.c,v 1.26 2022/05/04 18:57:50 deraadt Exp $ */
/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * Revision 1.2  2026/07/20  ChatGPT/JJ
 * Namespaced the private isoctal() and isvisible() helper functions
 * to avoid macro and identifier collisions in ixemul's all.c unity build.
 *
 * Revision 1.1  2026/07/14  ChatGPT/JJ
 * Adapted the OpenBSD vis implementation for ixemul and GCC 2.95.3.
 * Removed OpenBSD weak-symbol declarations, replaced BSD-specific
 * integer types and isascii(), and retained ixemul's reallocarray().
 *
 * Reworked strnvis() to support siz == 0 and to calculate the required
 * output length without forming or advancing pointers outside the
 * destination object.
 */

#include "ixemul.h"
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <vis.h>

static int
vis_isoctal(int c)
{
	unsigned char uc = (unsigned char)c;

	return uc >= '0' && uc <= '7';
}

static int
vis_isvisible(int c, int flag)
{
	int vis_sp = flag & VIS_SP;
	int vis_tab = flag & VIS_TAB;
	int vis_nl = flag & VIS_NL;
	int vis_safe = flag & VIS_SAFE;
	int vis_glob = flag & VIS_GLOB;
	int vis_all = flag & VIS_ALL;
	unsigned char uc = (unsigned char)c;

	if (c == '\\' || !vis_all) {
		if ((unsigned int)c <= UCHAR_MAX && uc <= 0x7f &&
		    ((c != '*' && c != '?' && c != '[' && c != '#') || !vis_glob) &&
		    isgraph(uc))
			return 1;
		if (!vis_sp && c == ' ')
			return 1;
		if (!vis_tab && c == '\t')
			return 1;
		if (!vis_nl && c == '\n')
			return 1;
		if (vis_safe && (c == '\b' || c == '\007' || c == '\r' || isgraph(uc)))
			return 1;
	}
	return 0;
}

/*
 * vis - visually encode characters
 */
char *
vis(char *dst, int c, int flag, int nextc)
{
	int vis_dq = flag & VIS_DQ;
	int vis_noslash = flag & VIS_NOSLASH;
	int vis_cstyle = flag & VIS_CSTYLE;
	int vis_octal = flag & VIS_OCTAL;
	int vis_glob = flag & VIS_GLOB;

	if (vis_isvisible(c, flag)) {
		if ((c == '"' && vis_dq) ||
		    (c == '\\' && !vis_noslash))
			*dst++ = '\\';
		*dst++ = c;
		*dst = '\0';
		return (dst);
	}

	if (vis_cstyle) {
		switch (c) {
		case '\n':
			*dst++ = '\\';
			*dst++ = 'n';
			goto done;
		case '\r':
			*dst++ = '\\';
			*dst++ = 'r';
			goto done;
		case '\b':
			*dst++ = '\\';
			*dst++ = 'b';
			goto done;
		case '\a':
			*dst++ = '\\';
			*dst++ = 'a';
			goto done;
		case '\v':
			*dst++ = '\\';
			*dst++ = 'v';
			goto done;
		case '\t':
			*dst++ = '\\';
			*dst++ = 't';
			goto done;
		case '\f':
			*dst++ = '\\';
			*dst++ = 'f';
			goto done;
		case ' ':
			*dst++ = '\\';
			*dst++ = 's';
			goto done;
		case '\0':
			*dst++ = '\\';
			*dst++ = '0';
			if (vis_isoctal(nextc)) {
				*dst++ = '0';
				*dst++ = '0';
			}
			goto done;
		}
	}
	if (((c & 0177) == ' ') || vis_octal ||
	    (vis_glob && (c == '*' || c == '?' || c == '[' || c == '#'))) {
		*dst++ = '\\';
		*dst++ = ((unsigned char)c >> 6 & 07) + '0';
		*dst++ = ((unsigned char)c >> 3 & 07) + '0';
		*dst++ = ((unsigned char)c & 07) + '0';
		goto done;
	}
	if (!vis_noslash)
		*dst++ = '\\';
	if (c & 0200) {
		c &= 0177;
		*dst++ = 'M';
	}
	if (iscntrl((unsigned char)c)) {
		*dst++ = '^';
		if (c == 0177)
			*dst++ = '?';
		else
			*dst++ = c + '@';
	} else {
		*dst++ = '-';
		*dst++ = c;
	}
done:
	*dst = '\0';
	return (dst);
}

/*
 * strvis, strnvis, strvisx - visually encode characters from src into dst
 *	
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned. 
 *
 *	Strnvis will write no more than siz-1 bytes (and will NULL terminate).
 *	The number of bytes needed to fully encode the string is returned.
 *
 *	Strvisx encodes exactly len bytes from src into dst.
 *	This is useful for encoding a block of data.
 */
int
strvis(char *dst, const char *src, int flag)
{
	int c;
	char *start;

	start = dst;
	while ((c = (unsigned char)*src) != '\0') {
		src++;
		dst = vis(dst, c, flag, (unsigned char)*src);
	}
	*dst = '\0';
	return (dst - start);
}
/*
 * Encode src into dst, writing at most siz - 1 bytes and always
 * terminating dst when siz is non-zero.
 *
 * The return value is the complete encoded length, including bytes
 * that could not be stored.  Separate counters are used so that
 * siz == 0 does not require invalid destination-pointer arithmetic.
 */
int
strnvis(char *dst, const char *src, size_t siz, int flag)
{
	int vis_dq = flag & VIS_DQ;
	int vis_noslash = flag & VIS_NOSLASH;
	char tbuf[5];
	size_t written;
	int c, i, total;
	int truncated;

	written = 0;
	total = 0;
	truncated = (siz == 0);

	while ((c = (unsigned char)*src) != '\0') {
		if (vis_isvisible(c, flag)) {
			i = 1;
			if ((c == '"' && vis_dq) ||
			    (c == '\\' && !vis_noslash))
				i = 2;

			if (!truncated) {
				if ((size_t)i < siz - written) {
					if (i == 2)
						dst[written++] = '\\';
					dst[written++] = (char)c;
				} else {
					truncated = 1;
				}
			}
		} else {
			i = (int)(vis(tbuf, c, flag,
			    (unsigned char)src[1]) - tbuf);

			if (!truncated) {
				if ((size_t)i < siz - written) {
					memcpy(dst + written, tbuf, (size_t)i);
					written += (size_t)i;
				} else {
					truncated = 1;
				}
			}
		}

		total += i;
		src++;
	}

	if (siz > 0)
		dst[written] = '\0';

	return (total);
}

int
stravis(char **outp, const char *src, int flag)
{
	char *buf, *newbuf;
	int len, serrno;

	usetup;

	buf = reallocarray(NULL, 4, strlen(src) + 1);
	if (buf == NULL)
		return -1;
	len = strvis(buf, src, flag);
	serrno = errno;
	newbuf = realloc(buf, len + 1);
	if (newbuf == NULL) {
		*outp = buf;
		errno = serrno;
	} else {
		*outp = newbuf;
	}
	return (len);
}

int
strvisx(char *dst, const char *src, size_t len, int flag)
{
	int c, nextc;
	char *start;

	start = dst;
	while (len > 0) {
		c = (unsigned char)*src++;
		len--;
		nextc = len > 0 ? (unsigned char)*src : '\0';
		dst = vis(dst, c, flag, nextc);
	}
	*dst = '\0';
	return (dst - start);
}
