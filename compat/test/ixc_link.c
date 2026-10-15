/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* A program calling getopt_long, pselect, posix_openpt, lchown, strtonum,
 * reallocarray, isnormal, strptime, getline and the pthread subset must link against
 * libixcompat.a: ixemul has none of them (fzy 1.0: "undefined reference to
 * `getopt_long'" and `pselect'; script and ptyrun need the pty calls; coreutils 9's cp lchown).
 * Built by `make test` (bebbo's gcc, the repo's headers); it links or the
 * test fails. */
#include <getopt.h>
#include <sys/select.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
	static const struct option lo[] = { { "x", no_argument, 0, 'x' }, { 0, 0, 0, 0 } };
	fd_set r;
	struct timespec ts = { 0, 0 };
	int m;
	const char *errstr;
	struct tm tm;
	pthread_key_t key;
	pthread_once_t once = PTHREAD_ONCE_INIT;
	pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
	char *ln = 0;
	size_t lncap = 0;

	FD_ZERO(&r);
	m = posix_openpt(O_RDWR);
	return getopt_long(argc, argv, "x", lo, 0) + pselect(0, &r, 0, 0, &ts, 0) +
	    (m >= 0 && grantpt(m) == 0 && unlockpt(m) == 0 && ptsname(m) != 0 ? 0 : 1) + (argc > 5 ? lchown(argv[1], 0, 0) : 0) +
	    (int)strtonum(argv[0], 0, 9, &errstr) + (reallocarray(0, 1, 2) != 0) + isnormal(1.0 * argc) +
	    (strptime(argv[0], "%Y", &tm) != 0) + pthread_key_create(&key, 0) + pthread_once(&once, 0) + pthread_mutex_lock(&mu) + (int)getline(&ln, &lncap, stdin);
}
