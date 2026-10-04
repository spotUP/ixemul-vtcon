/* wctype.h for the ixemul SDK (UP-Term): wide-character classes and case
 * mapping from libixcompat.a, on the Unicode database of vtcon's
 * engine/vtwidth.h (compat/uclass_tab.h). The class rules are POSIX's over
 * the General_Category (compat/uclass.c). ixemul 48.2 has none; without
 * this header the toolchain's newlib <wctype.h> was found, whose types
 * clash with ixemul's. */
#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#include <sys/cdefs.h>
#include <wchar.h>

typedef int wctype_t;
typedef int wctrans_t;

__BEGIN_DECLS
int	iswalnum __P((wint_t));
int	iswalpha __P((wint_t));
int	iswblank __P((wint_t));
int	iswcntrl __P((wint_t));
int	iswdigit __P((wint_t));
int	iswgraph __P((wint_t));
int	iswlower __P((wint_t));
int	iswprint __P((wint_t));
int	iswpunct __P((wint_t));
int	iswspace __P((wint_t));
int	iswupper __P((wint_t));
int	iswxdigit __P((wint_t));
int	iswctype __P((wint_t, wctype_t));
wctype_t wctype __P((const char *));
wint_t	towlower __P((wint_t));
wint_t	towupper __P((wint_t));
wint_t	towctrans __P((wint_t, wctrans_t));
wctrans_t wctrans __P((const char *));
__END_DECLS

#endif /* _WCTYPE_H_ */
