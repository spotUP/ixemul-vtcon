/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of compat/getline.c (built with -DIXC_GETDELIM=ixc_getdelim
 * -DIXC_GETLINE=ixc_getline): lines with and without the final newline,
 * an empty line, a line longer than the first buffer, a custom delimiter,
 * a reused buffer, end of file, and the EINVAL cases. Exit status = failures. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

ssize_t ixc_getline(char **, size_t *, FILE *);
ssize_t ixc_getdelim(char **, size_t *, int, FILE *);

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } } while (0)

int
main(void)
{
	char *line = NULL, big[1000];
	size_t cap = 0, i;
	ssize_t r;
	FILE *f;

	for (i = 0; i < sizeof big - 2; i++)
		big[i] = 'a' + (char)(i % 26);
	big[sizeof big - 2] = '\n';
	big[sizeof big - 1] = '\0';

	f = tmpfile();
	fputs("one\n\ntwo;three;", f);
	fputs(big, f);
	fputs("last", f);
	rewind(f);

	r = ixc_getline(&line, &cap, f);
	CHECK(r == 4 && !strcmp(line, "one\n") && cap >= 5, "first line: %ld \"%s\"", (long)r, line);
	r = ixc_getline(&line, &cap, f);
	CHECK(r == 1 && !strcmp(line, "\n"), "empty line: %ld", (long)r);
	r = ixc_getdelim(&line, &cap, ';', f);
	CHECK(r == 4 && !strcmp(line, "two;"), "delimiter ;: %ld \"%s\"", (long)r, line);
	r = ixc_getdelim(&line, &cap, ';', f);
	CHECK(r == 6 && !strcmp(line, "three;"), "delimiter ; again: %ld", (long)r);
	r = ixc_getline(&line, &cap, f);
	CHECK(r == (ssize_t)sizeof big - 1 && !strcmp(line, big) && cap > 120, "long line: %ld cap %lu", (long)r, (unsigned long)cap);
	r = ixc_getline(&line, &cap, f);
	CHECK(r == 4 && !strcmp(line, "last"), "no final newline: %ld", (long)r);
	r = ixc_getline(&line, &cap, f);
	CHECK(r == -1, "end of file: %ld", (long)r);
	fclose(f);

	errno = 0;
	CHECK(ixc_getline(NULL, &cap, stdin) == -1 && errno == EINVAL, "null lineptr");
	errno = 0;
	CHECK(ixc_getline(&line, NULL, stdin) == -1 && errno == EINVAL, "null n");
	free(line);
	if (!fails)
		printf("[OK] getline\n");
	return fails;
}
