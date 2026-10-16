/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of static/getopt.c (the 4.4BSD getopt), built with
 * -Dgetopt=ixc_bsd_getopt and optind/opterr/optopt/optarg renamed likewise.
 * "--" ends options only when it is the whole argument; "--version" is the
 * illegal option '-'; a bare "-" is an operand. Exit status = failures. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int ixc_bsd_getopt(int, char *const *, const char *);
extern int ixc_bsd_optind, ixc_bsd_opterr, ixc_bsd_optopt;
extern char *ixc_bsd_optarg;

static int fails;

#define CHECK(c, what) do { if (!(c)) { fails++; printf("FAIL: %s\n", what); } } while (0)

int main(void)
{
	char *dd[] = {"prog", "-a", "--", "-b", NULL};
	char *lg[] = {"prog", "--version", NULL};
	char *bare[] = {"prog", "-", "x", NULL};
	char *cl[] = {"prog", "-ab", "-c", "val", "rest", NULL};
	int c;

	/* "--" ends options, is consumed */
	ixc_bsd_optind = 1;
	CHECK(ixc_bsd_getopt(3, dd, "ab") == 'a', "dd: -a");
	CHECK(ixc_bsd_getopt(3, dd, "ab") == -1, "dd: -- gives EOF");
	CHECK(ixc_bsd_optind == 3, "dd: optind past --");

	/* "--version" is illegal option '-', message printed */
	{
		char buf[128] = "";
		int first_optopt;
		FILE *f = tmpfile();
		int saved = dup(2);
		ixc_bsd_optind = 1;
		fflush(stderr);
		dup2(fileno(f), 2);
		c = ixc_bsd_getopt(2, lg, "ab");
		first_optopt = ixc_bsd_optopt;
		/* the rest of "version" is more illegal letters; drain them so the
		 * static scan state is clean for the next case */
		while (ixc_bsd_getopt(2, lg, "ab") != -1)
			;
		fflush(stderr);
		dup2(saved, 2);
		rewind(f);
		if (!fgets(buf, sizeof buf, f)) buf[0] = 0;
		CHECK(c == '?', "lg: returns '?'");
		CHECK(first_optopt == '-', "lg: optopt is '-'");
		CHECK(strstr(buf, "illegal option -- -\n") != NULL, "lg: message");
	}

	/* bare "-" is an operand: EOF, optind unchanged */
	ixc_bsd_optind = 1;
	CHECK(ixc_bsd_getopt(3, bare, "ab") == -1, "bare: EOF");
	CHECK(ixc_bsd_optind == 1, "bare: optind unchanged");

	/* clustering and option argument */
	ixc_bsd_optind = 1;
	CHECK(ixc_bsd_getopt(5, cl, "abc:") == 'a', "cl: a");
	CHECK(ixc_bsd_getopt(5, cl, "abc:") == 'b', "cl: b");
	CHECK(ixc_bsd_getopt(5, cl, "abc:") == 'c', "cl: c");
	CHECK(ixc_bsd_optarg && !strcmp(ixc_bsd_optarg, "val"), "cl: optarg");
	CHECK(ixc_bsd_getopt(5, cl, "abc:") == -1, "cl: EOF");
	CHECK(ixc_bsd_optind == 4, "cl: optind at operand");

	printf(fails ? "getopt_bsd_test: %d failed\n" : "getopt_bsd_test: ok\n", fails);
	return fails;
}
