/* gmtime_r and localtime_r (POSIX) over ixemul's gmtime/localtime: the
 * result copied out of the library's static struct. ixemul programs have
 * one thread, so nothing can overwrite it in between. (libevent, tmux) */
#include <time.h>

struct tm *
gmtime_r(const time_t *t, struct tm *out)
{
	struct tm *r = gmtime(t);
	if (!r)
		return 0;
	*out = *r;
	return out;
}

struct tm *
localtime_r(const time_t *t, struct tm *out)
{
	struct tm *r = localtime(t);
	if (!r)
		return 0;
	*out = *r;
	return out;
}
