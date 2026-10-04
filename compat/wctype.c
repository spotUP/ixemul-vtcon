/* <wctype.h> on the Unicode classes of uclass.c (UP-Term). WEOF and
 * anything negative is in no class and maps to itself. */
#include <wctype.h>
#include "ixc_wide.h"

#define CLASS(name, cls) \
	int name(wint_t c) { return (c >= 0 && __ixc_isclass((unsigned long)c, cls)); }
CLASS(iswalnum, IXC_ALNUM)
CLASS(iswalpha, IXC_ALPHA)
CLASS(iswblank, IXC_BLANK)
CLASS(iswcntrl, IXC_CNTRL)
CLASS(iswdigit, IXC_DIGIT)
CLASS(iswgraph, IXC_GRAPH)
CLASS(iswlower, IXC_LOWER)
CLASS(iswprint, IXC_PRINT)
CLASS(iswpunct, IXC_PUNCT)
CLASS(iswspace, IXC_SPACE)
CLASS(iswupper, IXC_UPPER)
CLASS(iswxdigit, IXC_XDIGIT)

int
iswctype(wint_t c, wctype_t t)
{
	return (c >= 0 && __ixc_isclass((unsigned long)c, t));
}

wctype_t
wctype(const char *name)
{
	return (__ixc_classname(name));
}

wint_t
towlower(wint_t c)
{
	return (c < 0 ? c : (wint_t)__ixc_tolower((unsigned long)c));
}

wint_t
towupper(wint_t c)
{
	return (c < 0 ? c : (wint_t)__ixc_toupper((unsigned long)c));
}

wctrans_t
wctrans(const char *name)
{
	if (name[0] == 't' && name[1] == 'o') {
		if (name[2] == 'l' && name[3] == 'o' && name[4] == 'w' &&
		    name[5] == 'e' && name[6] == 'r' && name[7] == '\0')
			return (1);
		if (name[2] == 'u' && name[3] == 'p' && name[4] == 'p' &&
		    name[5] == 'e' && name[6] == 'r' && name[7] == '\0')
			return (2);
	}
	return (0);
}

wint_t
towctrans(wint_t c, wctrans_t t)
{
	return (t == 1 ? towlower(c) : t == 2 ? towupper(c) : c);
}
