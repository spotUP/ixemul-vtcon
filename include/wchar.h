/* wchar.h for the ixemul SDK (UP-Term). ixemul 48.2 has no wide-character
 * functions beyond <stdlib.h>'s mbtowc/wctomb; libixcompat.a has these,
 * in the encoding setlocale's LC_CTYPE selects: UTF-8 when the locale
 * name says so (LANG=en_US.UTF-8), else ISO 8859-1 (the C locale). wchar_t
 * is a Unicode code point. wcwidth is the vtcon terminal's own table
 * (engine/vtwidth.h), so programs and the terminal agree on widths. Without
 * this header the toolchain's newlib <wchar.h> was found, whose types clash
 * with ixemul's stdio. */
#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef _WINT_T_DECLARED
#define _WINT_T_DECLARED
typedef int wint_t;
#endif
/* wchar_t is an int here (machine/ansi.h _BSD_WCHAR_T_) */
#ifndef WCHAR_MIN
#define WCHAR_MIN (-0x7fffffff-1)
#define WCHAR_MAX 0x7fffffff
#endif
#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

#ifndef _MBSTATE_T_DECLARED
#define _MBSTATE_T_DECLARED
/* a partial UTF-8 sequence (compat/ixc_wide.h struct ixc_mbstate); all
 * zero is the initial state */
typedef struct {
	unsigned char	__need;
	unsigned char	__len;
	unsigned short	__pad;
	unsigned long	__value;
} mbstate_t;
#endif

__BEGIN_DECLS
wint_t	btowc __P((int));
int	wctob __P((wint_t));
int	mbsinit __P((const mbstate_t *));
size_t	mbrlen __P((const char *, size_t, mbstate_t *));
size_t	mbrtowc __P((wchar_t *, const char *, size_t, mbstate_t *));
size_t	wcrtomb __P((char *, wchar_t, mbstate_t *));
size_t	mbsrtowcs __P((wchar_t *, const char **, size_t, mbstate_t *));
size_t	mbsnrtowcs __P((wchar_t *, const char **, size_t, size_t, mbstate_t *));
size_t	wcsrtombs __P((char *, const wchar_t **, size_t, mbstate_t *));
size_t	wcsnrtombs __P((char *, const wchar_t **, size_t, size_t, mbstate_t *));
int	wcwidth __P((wchar_t));
int	wcswidth __P((const wchar_t *, size_t));

/* C95's wide string functions (newlib's, in libixcompat.a) */
wchar_t	*wcscat __P((wchar_t *, const wchar_t *));
wchar_t	*wcschr __P((const wchar_t *, wchar_t));
int	wcscmp __P((const wchar_t *, const wchar_t *));
int	wcscoll __P((const wchar_t *, const wchar_t *));
wchar_t	*wcscpy __P((wchar_t *, const wchar_t *));
size_t	wcscspn __P((const wchar_t *, const wchar_t *));
size_t	wcslcpy __P((wchar_t *, const wchar_t *, size_t));
size_t	wcslen __P((const wchar_t *));
wchar_t	*wcsncat __P((wchar_t *, const wchar_t *, size_t));
int	wcsncmp __P((const wchar_t *, const wchar_t *, size_t));
wchar_t	*wcsncpy __P((wchar_t *, const wchar_t *, size_t));
size_t	wcsnlen __P((const wchar_t *, size_t));
wchar_t	*wcspbrk __P((const wchar_t *, const wchar_t *));
wchar_t	*wcsrchr __P((const wchar_t *, wchar_t));
size_t	wcsspn __P((const wchar_t *, const wchar_t *));
wchar_t	*wcsstr __P((const wchar_t *, const wchar_t *));
wchar_t	*wcstok __P((wchar_t *, const wchar_t *, wchar_t **));
size_t	wcsxfrm __P((wchar_t *, const wchar_t *, size_t));
wchar_t	*wmemchr __P((const wchar_t *, wchar_t, size_t));
int	wmemcmp __P((const wchar_t *, const wchar_t *, size_t));
wchar_t	*wmemcpy __P((wchar_t *, const wchar_t *, size_t));
wchar_t	*wmemmove __P((wchar_t *, const wchar_t *, size_t));
wchar_t	*wmemset __P((wchar_t *, wchar_t, size_t));
__END_DECLS

#endif /* _WCHAR_H_ */
