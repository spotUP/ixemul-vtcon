/* getpgid, which the SDK's 48.2 stub set has not (mandoc 1.14.6 failed to
 * link on it): a process's own group from getpgrp. ixemul keeps no table
 * of other processes' groups reachable from here, so any other pid gives
 * ESRCH, which POSIX allows when the process is not visible. */
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

pid_t
getpgid(pid_t pid)
{
	if (pid == 0 || pid == getpid())
		return getpgrp();
	errno = ESRCH;
	return -1;
}
