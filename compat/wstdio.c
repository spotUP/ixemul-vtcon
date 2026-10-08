/* Wide-character output on ixemul's byte stdio (UP-Term): the character in
 * the encoding setlocale's LC_CTYPE selects (wcrtomb), byte by byte through
 * putc. mandoc 1.14.6's -T utf8 needs putwchar (its configure test failed
 * to compile on it). No orientation is kept: byte and wide output mix. */
#include <stdio.h>
#include <limits.h>
#include <wchar.h>
#include <errno.h>

wint_t
fputwc(wchar_t wc, FILE *f)
{
	char buf[8];
	mbstate_t st = { 0, 0, 0, 0 };
	size_t n = wcrtomb(buf, wc, &st), i;

	if (n == (size_t)-1)
		return WEOF;		/* EILSEQ from wcrtomb */
	for (i = 0; i < n; i++)
		if (putc((unsigned char)buf[i], f) == EOF)
			return WEOF;
	return (wint_t)wc;
}

wint_t
putwc(wchar_t wc, FILE *f)
{
	return fputwc(wc, f);
}

wint_t
putwchar(wchar_t wc)
{
	return fputwc(wc, stdout);
}
