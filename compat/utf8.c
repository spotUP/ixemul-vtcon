/* The multibyte codec of libixcompat: strict UTF-8 (no overlong forms, no
 * surrogates, nothing above U+10FFFF), restartable across calls, or
 * ISO 8859-1 one byte per character. See ixc_wide.h. (UP-Term) */
#include "ixc_wide.h"

size_t
__ixc_mbrtowc(unsigned long *pwc, const char *s, size_t n,
    struct ixc_mbstate *st, int utf8)
{
	static const unsigned long min[5] = { 0, 0, 0x80, 0x800, 0x10000 };
	const unsigned char *p = (const unsigned char *)s;
	unsigned long c;
	size_t i;
	int need, len;

	if (n == 0)
		return ((size_t)-2);
	if (!utf8) {
		if (pwc != NULL)
			*pwc = p[0];
		return (p[0] != 0);
	}
	if (st->need == 0) {
		if (p[0] < 0x80) {
			if (pwc != NULL)
				*pwc = p[0];
			return (p[0] != 0);
		}
		if (p[0] >= 0xc2 && p[0] <= 0xdf) {
			len = 2; c = p[0] & 0x1f;
		} else if ((p[0] & 0xf0) == 0xe0) {
			len = 3; c = p[0] & 0x0f;
		} else if (p[0] >= 0xf0 && p[0] <= 0xf4) {
			len = 4; c = p[0] & 0x07;
		} else
			return ((size_t)-1);
		need = len - 1;
		i = 1;
	} else {
		len = st->len;
		need = st->need;
		c = st->value;
		i = 0;
	}
	for (; need > 0 && i < n; i++, need--) {
		if ((p[i] & 0xc0) != 0x80) {
			st->need = 0;
			return ((size_t)-1);
		}
		c = (c << 6) | (p[i] & 0x3f);
		/* reject an overlong or out-of-range form as soon as the
		 * second byte shows it, as a strict decoder does */
		if (len - need == 1 && ((len == 3 && c < 0x20) ||
		    (len == 4 && (c < 0x10 || c > 0x10f)) ||
		    (len == 3 && c >= 0x360 && c <= 0x37f))) {
			st->need = 0;
			return ((size_t)-1);
		}
	}
	if (need > 0) {
		st->need = (unsigned char)need;
		st->len = (unsigned char)len;
		st->value = c;
		return ((size_t)-2);
	}
	st->need = 0;
	if (c < min[len] || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
		return ((size_t)-1);
	if (pwc != NULL)
		*pwc = c;
	return (c == 0 ? 0 : i);
}

size_t
__ixc_wcrtomb(char *s, unsigned long c, int utf8)
{
	if (!utf8) {
		if (c > 0xff)
			return ((size_t)-1);
		s[0] = (char)c;
		return (1);
	}
	if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
		return ((size_t)-1);
	if (c < 0x80) {
		s[0] = (char)c;
		return (1);
	}
	if (c < 0x800) {
		s[0] = (char)(0xc0 | (c >> 6));
		s[1] = (char)(0x80 | (c & 0x3f));
		return (2);
	}
	if (c < 0x10000) {
		s[0] = (char)(0xe0 | (c >> 12));
		s[1] = (char)(0x80 | ((c >> 6) & 0x3f));
		s[2] = (char)(0x80 | (c & 0x3f));
		return (3);
	}
	s[0] = (char)(0xf0 | (c >> 18));
	s[1] = (char)(0x80 | ((c >> 12) & 0x3f));
	s[2] = (char)(0x80 | ((c >> 6) & 0x3f));
	s[3] = (char)(0x80 | (c & 0x3f));
	return (4);
}

/* the codeset part of a locale name, after the '.', up to a '@' */
int
__ixc_name_is_utf8(const char *name)
{
	const char *p = name;
	char k[6];
	int i = 0;

	while (*p != '\0' && *p != '.')
		p++;
	if (*p == '\0')
		return (0);
	for (p++; *p != '\0' && *p != '@' && i < 6; p++) {
		char ch = *p;
		if (ch == '-' || ch == '_')
			continue;
		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		k[i++] = ch;
	}
	return (i == 4 && (*p == '\0' || *p == '@') &&
	    k[0] == 'u' && k[1] == 't' && k[2] == 'f' && k[3] == '8');
}
