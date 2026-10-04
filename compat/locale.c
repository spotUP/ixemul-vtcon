/* setlocale with a UTF-8 LC_CTYPE (UP-Term). ixemul.library's setlocale
 * knows only "C" and answers "" with "C", so no program could ever select
 * UTF-8. This one, linked from libixcompat.a ahead of the library's stub,
 * keeps a name per category: "" resolves LC_ALL, then LC_<category>, then
 * LANG, then "C", as POSIX says. Every name is accepted (31 characters at
 * most); only LC_CTYPE's has an effect: a codeset of UTF-8 ("en_US.UTF-8",
 * "C.utf8") makes the multibyte functions UTF-8 (wchar.c), anything else
 * leaves them ISO 8859-1, AmigaOS's own character set. The other
 * categories stay the C locale's behaviour (ixemul has no other). */
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "ixc_wide.h"

int __ixc_ctype_utf8;		/* LC_CTYPE is UTF-8 (wchar.c, langinfo.c) */

static char names[_LC_LAST][32] = {
	"C", "C", "C", "C", "C", "C", "C"
};
static const char *const envs[_LC_LAST] = {
	"LC_ALL", "LC_COLLATE", "LC_CTYPE", "LC_MONETARY", "LC_NUMERIC",
	"LC_TIME", "LC_MESSAGES"
};

int
__ixc_mb_cur_max(void)
{
	return (__ixc_ctype_utf8 ? 4 : 1);
}

static const char *
from_env(int cat)
{
	const char *v;

	if ((v = getenv("LC_ALL")) != NULL && *v != '\0')
		return (v);
	if ((v = getenv(envs[cat])) != NULL && *v != '\0')
		return (v);
	if ((v = getenv("LANG")) != NULL && *v != '\0')
		return (v);
	return ("C");
}

char *
setlocale(int category, const char *locale)
{
	const char *want[_LC_LAST];
	int c, lo, hi;

	if (category < 0 || category >= _LC_LAST)
		return (NULL);
	if (category == LC_ALL) {
		lo = 1;
		hi = _LC_LAST - 1;
	} else
		lo = hi = category;
	if (locale != NULL) {
		for (c = lo; c <= hi; c++) {
			want[c] = *locale != '\0' ? locale : from_env(c);
			if (strlen(want[c]) >= sizeof names[c])
				return (NULL);	/* change nothing */
		}
		for (c = lo; c <= hi; c++)
			strcpy(names[c], want[c]);
		__ixc_ctype_utf8 = __ixc_name_is_utf8(names[LC_CTYPE]);
	}
	if (category != LC_ALL)
		return (names[category]);
	/* LC_ALL: the common name; when the categories differ, LC_CTYPE's
	 * (the one that has an effect here) */
	for (c = 2; c < _LC_LAST; c++)
		if (strcmp(names[c], names[1]) != 0)
			return (names[LC_CTYPE]);
	return (names[1]);
}
