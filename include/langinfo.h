/* langinfo.h for the ixemul SDK (UP-Term): nl_langinfo from libixcompat.a.
 * ixemul has one locale, the C locale on AmigaOS's own character set, so
 * CODESET is ISO8859-1. Without this header the toolchain's newlib
 * <langinfo.h> was found, and its <sys/features.h> defined _POSIX_SOURCE,
 * which hides half of ixemul's <stdlib.h>. */
#ifndef _LANGINFO_H_
#define _LANGINFO_H_

#include <sys/cdefs.h>

typedef int nl_item;

#define	CODESET		0	/* the character set */
#define	D_T_FMT		1	/* date and time format */
#define	D_FMT		2	/* date format */
#define	T_FMT		3	/* time format */
#define	RADIXCHAR	4	/* decimal point */
#define	THOUSEP		5	/* thousands separator */
#define	YESEXPR		6	/* affirmative answer, a regular expression */
#define	NOEXPR		7	/* negative answer */

__BEGIN_DECLS
char *nl_langinfo __P((nl_item));
__END_DECLS

#endif /* _LANGINFO_H_ */
