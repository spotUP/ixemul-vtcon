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

/* attributes: stored, never acted on */
int
pthread_attr_init(pthread_attr_t *a)
{
	a->detachstate = PTHREAD_CREATE_JOINABLE;
	a->stacksize = 0;
	return 0;
}

int
pthread_attr_destroy(pthread_attr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_attr_setdetachstate(pthread_attr_t *a, int state)
{
	if (state != PTHREAD_CREATE_JOINABLE && state != PTHREAD_CREATE_DETACHED)
		return EINVAL;
	a->detachstate = state;
	return 0;
}

int
pthread_attr_getdetachstate(const pthread_attr_t *a, int *state)
{
	*state = a->detachstate;
	return 0;
}

int
pthread_attr_setstacksize(pthread_attr_t *a, size_t size)
{
	a->stacksize = size;
	return 0;
}

int
pthread_attr_getstacksize(const pthread_attr_t *a, size_t *size)
{
	*size = a->stacksize;
	return 0;
}

/* no second thread can be started, so none can be joined or detached */
int
pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg)
{
	(void)t; (void)a; (void)fn; (void)arg;
	return EAGAIN;
}

int
pthread_join(pthread_t t, void **ret)
{
	(void)t; (void)ret;
	return ESRCH;
}

int
pthread_detach(pthread_t t)
{
	(void)t;
	return ESRCH;
}

/* condition variables: nobody else can signal, so a wait would hang for ever */
int
pthread_condattr_init(pthread_condattr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_condattr_destroy(pthread_condattr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a)
{
	(void)a;
	c->unused = 0;
	return 0;
}

int
pthread_cond_destroy(pthread_cond_t *c)
{
	(void)c;
	return 0;
}

int
pthread_cond_signal(pthread_cond_t *c)
{
	(void)c;
	return 0;
}

int
pthread_cond_broadcast(pthread_cond_t *c)
{
	(void)c;
	return 0;
}

int
pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
	(void)c; (void)m;
	return EDEADLK;
}

int
pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *ts)
{
	(void)c; (void)m; (void)ts;
	return ETIMEDOUT;
}

/* read-write locks: a reader count and a writer flag */
int
pthread_rwlockattr_init(pthread_rwlockattr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_rwlockattr_destroy(pthread_rwlockattr_t *a)
{
	(void)a;
	return 0;
}

int
pthread_rwlock_init(pthread_rwlock_t *l, const pthread_rwlockattr_t *a)
{
	(void)a;
	l->readers = 0;
	l->writer = 0;
	return 0;
}

int
pthread_rwlock_destroy(pthread_rwlock_t *l)
{
	return l->readers || l->writer ? EBUSY : 0;
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *l)
{
	if (l->writer)
		return EDEADLK;
	l->readers++;
	return 0;
}

int
pthread_rwlock_tryrdlock(pthread_rwlock_t *l)
{
	if (l->writer)
		return EBUSY;
	l->readers++;
	return 0;
}

int
pthread_rwlock_wrlock(pthread_rwlock_t *l)
{
	if (l->writer || l->readers)
		return EDEADLK;
	l->writer = 1;
	return 0;
}

int
pthread_rwlock_trywrlock(pthread_rwlock_t *l)
{
	if (l->writer || l->readers)
		return EBUSY;
	l->writer = 1;
	return 0;
}

int
pthread_rwlock_unlock(pthread_rwlock_t *l)
{
	if (l->writer)
		l->writer = 0;
	else if (l->readers)
		l->readers--;
	else
		return EPERM;
	return 0;
}
