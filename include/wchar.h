/* wchar.h for the ixemul SDK (UP-Term): the types only. ixemul 48.2 has
 * no wide-character functions beyond <stdlib.h>'s mbtowc/wctomb, and
 * without this header the toolchain's newlib <wchar.h> was found instead,
 * whose types clash with ixemul's stdio. */
#ifndef _WCHAR_H_
#define _WCHAR_H_

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

#endif /* _WCHAR_H_ */
