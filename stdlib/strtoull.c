/*  Derived from NetBSD strtoull.c
 *
 *  Copyright (c) 1992, 1993
 *      The Regents of the University of California.
 *      All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

/*
 * strtoull.c,v
 *
 * Revision 1.1  2026/08/16  ChatGPT  (JJ)
 *
 *    Accept a 0x/0X prefix only when followed by a hexadecimal digit.
 */

/*
 * strtoull() - imported and integrated into ixemul.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

unsigned long long
strtoull(const char *nptr, char **endptr, int base)
{
	register const char *s = nptr;
	register unsigned long long acc;
	register int c;
	register unsigned long long cutoff;
	register int neg = 0, any, cutlim;

	usetup;

	do {
		c = *s++;
	} while (isspace((unsigned char)c));

	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;

	if (base < 0 || base == 1 || base > 36) {
		if (endptr != 0)
			*endptr = (char *)nptr;
		errno = EINVAL;
		return 0;
	}

	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X') &&
	    ((s[1] >= '0' && s[1] <= '9') ||
	     (s[1] >= 'a' && s[1] <= 'f') ||
	     (s[1] >= 'A' && s[1] <= 'F'))) {
		c = s[1];
		s += 2;
		base = 16;
	}

	if (base == 0)
		base = c == '0' ? 8 : 10;

	cutoff = (unsigned long long)ULLONG_MAX / (unsigned long long)base;
	cutlim = (unsigned long long)ULLONG_MAX % (unsigned long long)base;

	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit((unsigned char)c))
			c -= '0';
		else if (isalpha((unsigned char)c))
			c -= isupper((unsigned char)c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;

		if (any < 0 ||
		    acc > cutoff ||
		    (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}

	if (any < 0) {
		acc = ULLONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;

	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);

	return acc;
}
