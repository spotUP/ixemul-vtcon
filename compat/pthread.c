/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* The single-thread pthread subset (include/pthread.h says why). One thread
 * means a lock that is already held can never be released by anybody else:
 * a normal mutex locked twice reports EDEADLK instead of hanging for ever,
 * as POSIX permits for error-checking mutexes, and a recursive one counts.
 * Unlocking an unheld mutex is EPERM. The host test builds this file with
 * test/pthread_ixc.h force-included, which renames every name to ixc_*
 * (the host's own <pthread.h> types and functions are in the way). */
#include <errno.h>
#include <stddef.h>
#include <pthread.h>

#define NKEYS PTHREAD_KEYS_MAX

static struct { int used; void *val; } keys[NKEYS];

int
pthread_once(pthread_once_t *once, void (*fn)(void))
{
	if (once == NULL || fn == NULL)
		return EINVAL;
	if (!*once) {
		*once = 1;	/* set first: fn may call pthread_once on the same flag */
		fn();
	}
	return 0;
}

int
pthread_key_create(pthread_key_t *key, void (*dtor)(void *))
{
	unsigned i;

	(void)dtor;
	for (i = 0; i < NKEYS; i++)
		if (!keys[i].used) {
			keys[i].used = 1;
			keys[i].val = NULL;
			*key = i;
			return 0;
		}
	return EAGAIN;
}

int
pthread_key_delete(pthread_key_t key)
{
	if (key >= NKEYS || !keys[key].used)
		return EINVAL;
	keys[key].used = 0;
	keys[key].val = NULL;
	return 0;
}

int
pthread_setspecific(pthread_key_t key, const void *val)
{
	if (key >= NKEYS || !keys[key].used)
		return EINVAL;
	keys[key].val = (void *)val;
	return 0;
}

void *
pthread_getspecific(pthread_key_t key)
{
	return key < NKEYS && keys[key].used ? keys[key].val : NULL;
}

int
pthread_mutexattr_init(pthread_mutexattr_t *a)
{
	a->type = PTHREAD_MUTEX_DEFAULT;
	return 0;
}

int
pthread_mutexattr_destroy(pthread_mutexattr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_mutexattr_settype(pthread_mutexattr_t *a, int type)
{
	if (type < PTHREAD_MUTEX_NORMAL || type > PTHREAD_MUTEX_RECURSIVE)
		return EINVAL;
	a->type = type;
	return 0;
}

int
pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
	m->locked = 0;
	m->type = a ? a->type : PTHREAD_MUTEX_DEFAULT;
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *m)
{
	return m->locked ? EBUSY : 0;
}

int
pthread_mutex_lock(pthread_mutex_t *m)
{
	if (m->locked && m->type != PTHREAD_MUTEX_RECURSIVE)
		return EDEADLK;
	m->locked++;
	return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *m)
{
	if (m->locked && m->type != PTHREAD_MUTEX_RECURSIVE)
		return EBUSY;
	m->locked++;
	return 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *m)
{
	if (!m->locked)
		return EPERM;
	m->locked--;
	return 0;
}

pthread_t
pthread_self(void)
{
	return 1;
}

int
pthread_equal(pthread_t a, pthread_t b)
{
	return a == b;
}
