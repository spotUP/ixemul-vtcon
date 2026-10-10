/* pselect (UP-Term libixcompat): select with a signal mask and a timespec.
 * ixemul has no pselect; fzy 1.0's tty_input_ready calls it. ixemul has one
 * thread per program, so the mask swap around select() cannot lose a signal
 * to another thread; a signal arriving between the swap and the select()
 * runs its handler before select starts waiting, which POSIX's atomic form
 * avoids. Callers that need that wait in sigsuspend or block signals
 * themselves. */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

int
pselect(int nfds, fd_set *rd, fd_set *wr, fd_set *ex, const struct timespec *ts, const sigset_t *mask)
{
	struct timeval tv, *tvp = 0;
	sigset_t old;
	int r, e;

	if (ts) {
		tv.tv_sec = ts->tv_sec;
		tv.tv_usec = ts->tv_nsec / 1000;
		tvp = &tv;
	}
	if (mask)
		sigprocmask(SIG_SETMASK, mask, &old);
	r = select(nfds, rd, wr, ex, tvp);
	e = errno;
	if (mask)
		sigprocmask(SIG_SETMASK, &old, 0);
	errno = e;
	return r;
}
