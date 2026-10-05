/* stpcpy and mempcpy for programs built against the 80.x SDK headers.
 * <string.h> declares them (80.x has vectors 636 and up), and gcc then
 * writes calls to them itself, turning strcpy/memcpy chains into one
 * call: GNU screen had no stpcpy in its source and failed to link. The
 * SDK's libc.a keeps 48.2's stubs so a port runs on both libraries;
 * these static copies win at link and run on both. (UP-Term) */
#include <string.h>

char *
stpcpy(char *d, const char *s)
{
	while ((*d = *s++) != '\0')
		d++;
	return d;
}

void *
mempcpy(void *d, const void *s, size_t n)
{
	return (char *)memcpy(d, s, n) + n;
}
