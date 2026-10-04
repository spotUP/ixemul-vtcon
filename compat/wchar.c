/* <wchar.h>'s conversions and <stdlib.h>'s multibyte functions in the
 * LC_CTYPE encoding setlocale chose (locale.c): UTF-8 or ISO 8859-1, on the
 * codec in utf8.c. The <stdlib.h> ones (mblen, mbtowc, wctomb, mbstowcs,
 * wcstombs) replace ixemul.library's single-byte ones. (UP-Term) */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "ixc_wide.h"

extern int __ixc_ctype_utf8;
#define ST(ps)	((struct ixc_mbstate *)(void *)(ps))

wint_t
btowc(int c)
{
	if (c == -1)
		return (WEOF);
	c &= 0xff;
	return (__ixc_ctype_utf8 && c >= 0x80 ? WEOF : (wint_t)c);
}

int
wctob(wint_t wc)
{
	if (wc < 0 || wc > (__ixc_ctype_utf8 ? 0x7f : 0xff))
		return (-1);
	return (wc);
}

int
mbsinit(const mbstate_t *ps)
{
	return (ps == NULL || ps->__need == 0);
}

size_t
mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t own;
	unsigned long c;
	size_t r;

	if (ps == NULL)
		ps = &own;
	if (s == NULL) {
		s = "";
		n = 1;
		pwc = NULL;
	}
	r = __ixc_mbrtowc(&c, s, n, ST(ps), __ixc_ctype_utf8);
	if (r == (size_t)-1)
		errno = EILSEQ;
	else if (r != (size_t)-2 && pwc != NULL)
		*pwc = (wchar_t)c;
	return (r);
}

size_t
mbrlen(const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t own;

	return (mbrtowc(NULL, s, n, ps != NULL ? ps : &own));
}

size_t
wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
	char buf[4];
	size_t r;

	if (ps != NULL)
		ps->__need = 0;
	if (s == NULL) {
		s = buf;
		wc = 0;
	}
	r = __ixc_wcrtomb(s, (unsigned long)wc, __ixc_ctype_utf8);
	if (r == (size_t)-1)
		errno = EILSEQ;
	return (r);
}

size_t
mbsnrtowcs(wchar_t *dst, const char **src, size_t nms, size_t len, mbstate_t *ps)
{
	static mbstate_t own;
	mbstate_t copy;
	const char *s = *src;
	wchar_t wc;
	size_t k = 0, r;

	if (ps == NULL)
		ps = &own;
	if (dst == NULL) {	/* count only: *src and the state stay */
		copy = *ps;
		ps = &copy;
	}
	while (dst == NULL || k < len) {
		r = mbrtowc(&wc, s, nms, ps);
		if (r == (size_t)-1) {
			if (dst != NULL)
				*src = s;
			return (r);
		}
		if (r == (size_t)-2) {	/* input ends inside a character */
			if (dst != NULL)
				*src = s + nms;
			return (k);
		}
		if (r == 0) {
			if (dst != NULL) {
				dst[k] = 0;
				*src = NULL;
			}
			return (k);
		}
		if (dst != NULL)
			dst[k] = wc;
		s += r;
		nms -= r;
		k++;
	}
	*src = s;
	return (k);
}

size_t
mbsrtowcs(wchar_t *dst, const char **src, size_t len, mbstate_t *ps)
{
	return (mbsnrtowcs(dst, src, (size_t)-1, len, ps));
}

size_t
wcsnrtombs(char *dst, const wchar_t **src, size_t nwc, size_t len, mbstate_t *ps)
{
	const wchar_t *w = *src;
	char buf[4];
	size_t k = 0, r;

	if (ps != NULL)
		ps->__need = 0;
	for (; nwc > 0; nwc--, w++) {
		r = __ixc_wcrtomb(buf, (unsigned long)*w, __ixc_ctype_utf8);
		if (r == (size_t)-1) {
			errno = EILSEQ;
			if (dst != NULL)
				*src = w;
			return (r);
		}
		if (dst != NULL) {
			if (k + r > len)
				break;
			memcpy(dst + k, buf, r);
		}
		if (*w == 0) {
			if (dst != NULL)
				*src = NULL;
			return (k);
		}
		k += r;
	}
	if (dst != NULL)
		*src = w;
	return (k);
}

size_t
wcsrtombs(char *dst, const wchar_t **src, size_t len, mbstate_t *ps)
{
	return (wcsnrtombs(dst, src, (size_t)-1, len, ps));
}

/* <stdlib.h>: stateless forms (UTF-8 has no shift states, so 0 for s NULL) */
int
mblen(const char *s, size_t n)
{
	static mbstate_t st;
	size_t r;

	if (s == NULL)
		return (0);
	r = mbrtowc(NULL, s, n, &st);
	if (r == (size_t)-1 || r == (size_t)-2) {
		memset(&st, 0, sizeof st);
		errno = EILSEQ;
		return (-1);
	}
	return ((int)r);
}

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	static mbstate_t st;
	size_t r;

	if (s == NULL)
		return (0);
	r = mbrtowc(pwc, s, n, &st);
	if (r == (size_t)-1 || r == (size_t)-2) {
		memset(&st, 0, sizeof st);
		errno = EILSEQ;
		return (-1);
	}
	return ((int)r);
}

int
wctomb(char *s, wchar_t wc)
{
	if (s == NULL)
		return (0);
	return ((int)wcrtomb(s, wc, NULL));
}

size_t
mbstowcs(wchar_t *dst, const char *src, size_t n)
{
	mbstate_t st;

	memset(&st, 0, sizeof st);
	return (mbsrtowcs(dst, &src, n, &st));
}

size_t
wcstombs(char *dst, const wchar_t *src, size_t n)
{
	mbstate_t st;

	memset(&st, 0, sizeof st);
	return (wcsrtombs(dst, &src, n, &st));
}
