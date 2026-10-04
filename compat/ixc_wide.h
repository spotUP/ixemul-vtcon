/* The wide-character core of libixcompat (UP-Term): plain C types only, so
 * the logic is tested on the host (test/wide_test.c) without the SDK
 * headers. The POSIX functions in locale.c, wchar.c, wctype.c and
 * wcwidth.c are thin wrappers over it.
 *
 * A code point is an unsigned long. The multibyte encoding is UTF-8 or
 * ISO 8859-1 (AmigaOS's own character set, the "C" locale), chosen by
 * setlocale's LC_CTYPE (locale.c). */
#ifndef IXC_WIDE_H
#define IXC_WIDE_H

#include <stddef.h>

/* the conversion state of a partial UTF-8 sequence, kept inside the
 * caller's mbstate_t: need = continuation bytes still missing, value =
 * the bits so far, min = the smallest code point the length allows */
struct ixc_mbstate {
	unsigned char	need;
	unsigned char	len;
	unsigned short	pad;
	unsigned long	value;
};

/* mbrtowc on n bytes of s. Returns the bytes used (0 for a NUL),
 * (size_t)-2 for an incomplete sequence (state kept), (size_t)-1 for an
 * invalid one (EILSEQ is the caller's to set). utf8 = 0: ISO 8859-1. */
size_t	__ixc_mbrtowc(unsigned long *pwc, const char *s, size_t n,
	    struct ixc_mbstate *st, int utf8);
/* wcrtomb: bytes written to s (at most 4), or (size_t)-1 when wc has no
 * encoding (a surrogate, > U+10FFFF, or > 0xFF in ISO 8859-1) */
size_t	__ixc_wcrtomb(char *s, unsigned long wc, int utf8);
/* 1 when a locale name selects UTF-8 ("en_US.UTF-8", "C.utf8", ...) */
int	__ixc_name_is_utf8(const char *name);

/* the Unicode class of a code point (Unicode version of vtcon's
 * engine/vtwidth.h, generated table uclass_tab.h) */
enum {
	IXC_CN,		/* unassigned, noncharacter */
	IXC_LU,		/* uppercase letter */
	IXC_LL,		/* lowercase letter */
	IXC_LO,		/* other letter: Lt, Lm, Lo */
	IXC_M,		/* mark: Mn, Mc, Me */
	IXC_ND,		/* decimal digit */
	IXC_NO,		/* other number: Nl, No */
	IXC_P,		/* punctuation */
	IXC_S,		/* symbol */
	IXC_ZS,		/* space separator */
	IXC_ZL,		/* line, paragraph separator */
	IXC_CC,		/* control */
	IXC_CF,		/* format, private use */
	IXC_CS		/* surrogate */
};
int		__ixc_uclass(unsigned long c);
unsigned long	__ixc_toupper(unsigned long c);
unsigned long	__ixc_tolower(unsigned long c);

/* the POSIX classes over __ixc_uclass, one rule each (neovim-amiga's
 * amiga/wide/wctype.c, so both ports classify alike) */
enum {
	IXC_ALNUM = 1, IXC_ALPHA, IXC_BLANK, IXC_CNTRL, IXC_DIGIT, IXC_GRAPH,
	IXC_LOWER, IXC_PRINT, IXC_PUNCT, IXC_SPACE, IXC_UPPER, IXC_XDIGIT
};
int	__ixc_isclass(unsigned long c, int cls);
int	__ixc_classname(const char *name);	/* "alpha" -> IXC_ALPHA, else 0 */

#endif
