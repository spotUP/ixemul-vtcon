/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test of compat/pthread.c (built with test/pthread_ixc.h force-
 * included, names ixc_*): once runs the function once, also when it
 * re-enters; keys hold a value, are reused after deletion and run out with
 * EAGAIN; mutexes: a normal one locked twice is EDEADLK (it would hang
 * a single thread), a recursive one counts, unlocking an unheld one is EPERM,
 * try-lock reports EBUSY. Exit status = failures. */
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

static int fails, ran;
static pthread_once_t once = PTHREAD_ONCE_INIT;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void
init(void)
{
	ran++;
	pthread_once(&once, init);	/* re-entry must not recurse */
}

int
main(void)
{
	pthread_key_t k[PTHREAD_KEYS_MAX], extra, again;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER, r;
	pthread_mutexattr_t a;
	int v1 = 1, v2 = 2, i;

	pthread_once(&once, init);
	pthread_once(&once, init);
	CHECK(ran == 1, "once ran %d times", ran);

	CHECK(pthread_key_create(&k[0], 0) == 0 && pthread_key_create(&k[1], 0) == 0 && k[0] != k[1], "key create");
	CHECK(pthread_getspecific(k[0]) == NULL, "a new key reads NULL");
	CHECK(pthread_setspecific(k[0], &v1) == 0 && pthread_setspecific(k[1], &v2) == 0, "setspecific");
	CHECK(pthread_getspecific(k[0]) == &v1 && pthread_getspecific(k[1]) == &v2, "keys keep their own values");
	CHECK(pthread_key_delete(k[0]) == 0 && pthread_getspecific(k[0]) == NULL, "deleted key reads NULL");
	CHECK(pthread_setspecific(k[0], &v1) == EINVAL && pthread_key_delete(k[0]) == EINVAL, "a deleted key is invalid");
	CHECK(pthread_key_create(&again, 0) == 0 && again == k[0] && pthread_getspecific(again) == NULL, "slot reused, value cleared");
	for (i = 2; i < PTHREAD_KEYS_MAX; i++)
		CHECK(pthread_key_create(&k[i], 0) == 0, "key %d", i);
	CHECK(pthread_key_create(&extra, 0) == EAGAIN, "key table full is EAGAIN");
	CHECK(pthread_getspecific(PTHREAD_KEYS_MAX + 5) == NULL && pthread_setspecific(PTHREAD_KEYS_MAX + 5, &v1) == EINVAL, "out of range key");

	CHECK(pthread_mutex_lock(&m) == 0, "lock");
	CHECK(pthread_mutex_lock(&m) == EDEADLK, "second lock of a normal mutex is EDEADLK");
	CHECK(pthread_mutex_trylock(&m) == EBUSY, "trylock of a held mutex is EBUSY");
	CHECK(pthread_mutex_destroy(&m) == EBUSY, "destroy of a held mutex is EBUSY");
	CHECK(pthread_mutex_unlock(&m) == 0 && pthread_mutex_unlock(&m) == EPERM, "unlock, then unlock of an unheld mutex is EPERM");
	CHECK(pthread_mutex_trylock(&m) == 0 && pthread_mutex_unlock(&m) == 0 && pthread_mutex_destroy(&m) == 0, "trylock, unlock, destroy");

	CHECK(pthread_mutexattr_init(&a) == 0 && pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE) == 0, "attr");
	CHECK(pthread_mutexattr_settype(&a, 99) == EINVAL, "bad mutex type");
	CHECK(pthread_mutex_init(&r, &a) == 0, "init recursive");
	CHECK(pthread_mutex_lock(&r) == 0 && pthread_mutex_lock(&r) == 0, "recursive mutex locks twice");
	CHECK(pthread_mutex_unlock(&r) == 0 && pthread_mutex_destroy(&r) == EBUSY, "still held after one unlock");
	CHECK(pthread_mutex_unlock(&r) == 0 && pthread_mutex_destroy(&r) == 0, "free after two unlocks");
	pthread_mutexattr_destroy(&a);

	CHECK(pthread_equal(pthread_self(), pthread_self()), "self equals self");

	if (fails)
		return 1;
	puts("[OK] pthread subset");
	return 0;
}
