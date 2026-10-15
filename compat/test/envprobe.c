/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* envprobe: does a variable set in the parent reach an exec'd child (vtcon
 * FP4.6: coreutils `env FOO=bar cmd` lost FOO). Run it by a path
 * (VTC:envprobe or /VTC/envprobe). For each way of setting the variable it
 * spawns itself as a "setter", which sets ENVPROBE=<way> and execs itself as
 * the "child"; the child exits 0 when it sees ENVPROBE=<way>.
 *   setenv     ixemul's setenv, then execv
 *   putenv     ixemul's putenv, then execv
 *   replace    environ replaced by a new array (gnulib's rpl_putenv, which
 *              coreutils env uses), then execv
 *   execvp     the same, then execvp by the name's path
 *   spawnenv   posix_spawn with the current environ after setenv
 * Each check prints [OK] or [FAIL]; the exit status is the number of
 * failures. */
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

static int fails;

static void
check(int ok, const char *what)
{
	printf("%s %s\n", ok ? "[OK]  " : "[FAIL]", what);
	if (!ok)
		fails++;
}

/* environ with one more entry in front, as gnulib's putenv builds it */
static void
replace_environ(char *entry)
{
	char **ep, **n;
	size_t size = 0;

	for (ep = environ; *ep; ep++)
		size++;
	n = malloc((size + 2) * sizeof(char *));
	n[0] = entry;
	memcpy(n + 1, environ, (size + 1) * sizeof(char *));
	environ = n;
}

static int
setter(char *self, const char *way)
{
	static char entry[64];
	char *av[] = { self, "child", (char *)way, NULL };
	pid_t pid;
	int st;

	snprintf(entry, sizeof(entry), "ENVPROBE=%s", way);
	if (strcmp(way, "setenv") == 0)
		setenv("ENVPROBE", way, 1);
	else if (strcmp(way, "putenv") == 0)
		putenv(entry);
	else if (strcmp(way, "replace") == 0 || strcmp(way, "execvp") == 0)
		replace_environ(entry);
	else if (strcmp(way, "spawnenv") == 0) {
		setenv("ENVPROBE", way, 1);
		if (posix_spawn(&pid, self, NULL, NULL, av, environ) != 0 ||
		    waitpid(pid, &st, 0) != pid || !WIFEXITED(st))
			return (99);
		return (WEXITSTATUS(st));
	}
	if (strcmp(way, "execvp") == 0)
		execvp(self, av);
	else
		execv(self, av);
	return (98);
}

int
main(int argc, char **argv)
{
	static const char *ways[] = { "setenv", "putenv", "replace", "execvp", "spawnenv" };
	char *self = argv[0], what[80];
	const char *v;
	unsigned i;
	pid_t pid;
	int st, r;

	if (argc == 3 && strcmp(argv[1], "child") == 0) {
		v = getenv("ENVPROBE");
		return (v != NULL && strcmp(v, argv[2]) == 0 ? 0 : 1);
	}
	if (argc == 3 && strcmp(argv[1], "set") == 0)
		return (setter(self, argv[2]));
	setvbuf(stdout, NULL, _IONBF, 0);

	for (i = 0; i < sizeof(ways) / sizeof(ways[0]); i++) {
		char *av[] = { self, "set", (char *)ways[i], NULL };

		st = -1;
		r = posix_spawn(&pid, self, NULL, NULL, av, environ);
		if (r == 0 && waitpid(pid, &st, 0) != pid)
			st = -1;
		snprintf(what, sizeof(what), "%s: the exec'd child saw ENVPROBE=%s (status %d)",
		    ways[i], ways[i], WIFEXITED(st) ? WEXITSTATUS(st) : -1);
		check(r == 0 && WIFEXITED(st) && WEXITSTATUS(st) == 0, what);
	}
	check(getenv("ENVPROBE") == NULL, "the parent's environment is untouched");
	printf("%d failures\n", fails);
	return (fails);
}
