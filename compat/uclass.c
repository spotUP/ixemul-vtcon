/* Unicode classes and simple case mapping for libixcompat's <wctype.h>,
 * on the generated uclass_tab.h (gen_uclass.py). See ixc_wide.h. The
 * POSIX class rules are neovim-amiga's (amiga/wide/wctype.c). (UP-Term) */
#include "ixc_wide.h"

struct ixc_case_run {
	unsigned long	first;
	unsigned short	count;
	unsigned char	stride;
	long		delta;
};

#include "uclass_tab.h"

#define NRUNS	(sizeof ixc_class_run / sizeof ixc_class_run[0])

int
__ixc_uclass(unsigned long c)
{
	size_t lo = 0, hi = NRUNS;

	if (c > 0x10ffff)
		return (IXC_CN);
	/* the last run whose first code point is <= c */
	while (hi - lo > 1) {
		size_t mid = (lo + hi) / 2;
		if ((ixc_class_run[mid] >> 4) <= c)
			lo = mid;
		else
			hi = mid;
	}
	return ((int)(ixc_class_run[lo] & 15));
}

static unsigned long
casemap(const struct ixc_case_run *r, size_t n, unsigned long c)
{
	size_t lo = 0, hi = n;

	while (hi - lo > 1) {
		size_t mid = (lo + hi) / 2;
		if (r[mid].first <= c)
			lo = mid;
		else
			hi = mid;
	}
	r += lo;
	if (c >= r->first && c < r->first + (unsigned long)r->count * r->stride &&
	    (c - r->first) % r->stride == 0)
		return ((unsigned long)((long)c + r->delta));
	return (c);
}

unsigned long
__ixc_toupper(unsigned long c)
{
	if (c < 0x80)
		return (c >= 'a' && c <= 'z' ? c - 32 : c);
	return (casemap(ixc_upper, sizeof ixc_upper / sizeof ixc_upper[0], c));
}

unsigned long
__ixc_tolower(unsigned long c)
{
	if (c < 0x80)
		return (c >= 'A' && c <= 'Z' ? c + 32 : c);
	return (casemap(ixc_lower, sizeof ixc_lower / sizeof ixc_lower[0], c));
}

int
__ixc_isclass(unsigned long c, int cls)
{
	int k = __ixc_uclass(c);

	switch (cls) {
	case IXC_ALPHA:
		return (k == IXC_LU || k == IXC_LL || k == IXC_LO);
	case IXC_ALNUM:
		return (k == IXC_LU || k == IXC_LL || k == IXC_LO ||
		    k == IXC_ND || k == IXC_NO);
	case IXC_DIGIT:
		return (c >= '0' && c <= '9');	/* POSIX: these only */
	case IXC_XDIGIT:
		return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		    (c >= 'A' && c <= 'F'));
	case IXC_UPPER:
		return (k == IXC_LU);
	case IXC_LOWER:
		return (k == IXC_LL);
	case IXC_SPACE:
		if (c == ' ' || (c >= '\t' && c <= '\r'))
			return (1);
		return ((k == IXC_ZS || k == IXC_ZL) &&
		    c != 0xa0 && c != 0x2007 && c != 0x202f);	/* no-break */
	case IXC_BLANK:
		if (c == ' ' || c == '\t')
			return (1);
		return (k == IXC_ZS && c != 0xa0 && c != 0x2007 && c != 0x202f);
	case IXC_CNTRL:
		return (k == IXC_CC);
	case IXC_PUNCT:
		return (k == IXC_P || k == IXC_S);
	case IXC_PRINT:
		return (k != IXC_CN && k != IXC_CC && k != IXC_CS && k != IXC_ZL);
	case IXC_GRAPH:
		return (k != IXC_CN && k != IXC_CC && k != IXC_CS && k != IXC_ZL &&
		    k != IXC_ZS);
	}
	return (0);
}

int
__ixc_classname(const char *name)
{
	static const char *const names[] = { "alnum", "alpha", "blank",
	    "cntrl", "digit", "graph", "lower", "print", "punct", "space",
	    "upper", "xdigit" };
	int i;

	for (i = 0; i < 12; i++) {
		const char *a = names[i], *b = name;
		while (*a != '\0' && *a == *b)
			a++, b++;
		if (*a == '\0' && *b == '\0')
			return (i + 1);
	}
	return (0);
}
