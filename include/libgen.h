/* libgen.h for the ixemul SDK (UP-Term): basename and dirname, from
 * libixcompat.a (ixemul 48.2 has neither; without this header the
 * toolchain's newlib <libgen.h> was found and does not compile here). */
#ifndef _LIBGEN_H_
#define _LIBGEN_H_

#include <sys/cdefs.h>

__BEGIN_DECLS
char *basename __P((const char *));
char *dirname __P((const char *));
__END_DECLS

#endif /* _LIBGEN_H_ */
