/* basename and dirname (POSIX, OpenBSD's behaviour): the result in a
 * static buffer, the argument untouched. "/" is the root as in a Unix path;
 * an AmigaDOS "VOL:" is a directory with no parent here. (UP-Term: tmux) */
#include <string.h>
#include <sys/param.h>
#include <libgen.h>

char *
basename(const char *path)
{
	static char b[MAXPATHLEN];
	const char *end, *start;
	size_t n;

	if (path == NULL || *path == '\0')
		return strcpy(b, ".");
	end = path + strlen(path) - 1;
	while (end > path && *end == '/')
		end--;
	if (end == path && *end == '/')
		return strcpy(b, "/");
	start = end;
	while (start > path && start[-1] != '/' && start[-1] != ':')
		start--;
	n = end - start + 1;
	if (n >= sizeof b)
		n = sizeof b - 1;
	memcpy(b, start, n);
	b[n] = '\0';
	return b;
}

char *
dirname(const char *path)
{
	static char d[MAXPATHLEN];
	const char *end;
	size_t n;

	if (path == NULL || *path == '\0')
		return strcpy(d, ".");
	end = path + strlen(path) - 1;
	while (end > path && *end == '/')
		end--;
	while (end > path && *end != '/' && *end != ':')
		end--;
	if (end == path && *end != '/' && *end != ':')
		return strcpy(d, ".");
	if (*end == ':') {
		n = end - path + 1;	/* "VOL:x" -> "VOL:" */
	} else {
		while (end > path && end[-1] == '/')
			end--;
		n = end == path ? 1 : (size_t)(end - path);
	}
	if (n >= sizeof d)
		n = sizeof d - 1;
	memcpy(d, path, n);
	d[n] = '\0';
	return d;
}
