/* wcwidth and wcswidth from vtcon's engine/vtwidth.h (VTCON in the
 * Makefile): the table the XCON console draws with, the one tmux-amiga's
 * utf8proc.c uses too, so every program lays text out as the terminal
 * shows it. A control (C0, DEL, C1), a surrogate or anything above
 * U+10FFFF has no width (-1); NUL has 0, as POSIX says. (UP-Term) */
#include <wchar.h>

typedef unsigned long vt_u32;
#include "vtwidth.h"

int
wcwidth(wchar_t wc)
{
	if (wc == 0)
		return (0);
	if (wc < 0x20 || (wc >= 0x7f && wc < 0xa0) || wc > 0x10ffff ||
	    (wc >= 0xd800 && wc <= 0xdfff))
		return (-1);
	return (vt_char_width((vt_u32)wc));
}

int
wcswidth(const wchar_t *s, size_t n)
{
	int w, total = 0;

	for (; n > 0 && *s != 0; n--, s++) {
		if ((w = wcwidth(*s)) < 0)
			return (-1);
		total += w;
	}
	return (total);
}
