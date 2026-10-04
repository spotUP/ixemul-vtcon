/* sys/select.h for the ixemul SDK (UP-Term): POSIX's home of select() and
 * fd_set, which 4.4BSD ixemul keeps in <sys/types.h> (fd_set, FD_*),
 * <sys/time.h> (struct timeval) and <unistd.h> (select). Without this
 * header the toolchain's newlib <sys/select.h> was found, whose types
 * clash with ixemul's (ncurses 6.6's tty_update.c, in the UP-Term ports
 * build). ixemul has no pselect. */
#ifndef _SYS_SELECT_H_
#define _SYS_SELECT_H_

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#endif /* _SYS_SELECT_H_ */
