/* nanosleep on what ixemul 48.2 has (select with a timeout): ixemul 80's
 * vector 622 is not in the SDK's 48.2 stub set, which the ports keep so
 * they run on both libraries (ixemul-80 migration plan, M8). mandoc 1.14.6's
 * configure stops without it. Microsecond resolution; a signal ends the
 * sleep with EINTR and the time left in *rem. */
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
	struct timeval tv, start, now;
	long left_us;

	if (req == NULL || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L || req->tv_sec < 0) {
		errno = EINVAL;
		return -1;
	}
	tv.tv_sec = req->tv_sec;
	tv.tv_usec = (req->tv_nsec + 999) / 1000;
	if (tv.tv_usec >= 1000000) {
		tv.tv_sec++;
		tv.tv_usec -= 1000000;
	}
	gettimeofday(&start, NULL);
	if (select(0, NULL, NULL, NULL, &tv) == 0)
		return 0;
	if (errno == EINTR && rem != NULL) {
		gettimeofday(&now, NULL);
		left_us = (req->tv_sec - (now.tv_sec - start.tv_sec)) * 1000000L +
		    req->tv_nsec / 1000 - (now.tv_usec - start.tv_usec);
		if (left_us < 0)
			left_us = 0;
		rem->tv_sec = left_us / 1000000L;
		rem->tv_nsec = (left_us % 1000000L) * 1000;
	}
	return -1;
}
