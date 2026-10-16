/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* pthread.h for the ixemul SDK (UP-Term): the single-thread subset that
 * libixcompat (compat/pthread.c) provides. An ixemul program has one thread
 * of its own; this is what libraries written against pthreads (jq's
 * thread-local decNumber context and allocator handler) need to run in it:
 * once-only initialisation, thread-specific keys, mutexes. There is no
 * thread to wait for: pthread_create fails with EAGAIN, a condition wait can
 * never be signalled (EDEADLK), so a program that starts threads degrades
 * to its single-threaded path. The type set is complete enough for gnulib's
 * <pthread.h> wrapper (sed). The toolchain's own
 * <pthread.h> (libnix) clashes with ixemul's <sys/types.h>; this header
 * shadows it when the SDK headers are overlaid (docker/install-sdk-headers.sh). */
#ifndef _IXEMUL_PTHREAD_H_
#define _IXEMUL_PTHREAD_H_

#include <sys/types.h>
#include <stddef.h>
#include <time.h>
#include <sys/cdefs.h>

typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0

typedef unsigned int pthread_key_t;
#define PTHREAD_KEYS_MAX 64
#define PTHREAD_DESTRUCTOR_ITERATIONS 4

typedef struct { int locked; int type; } pthread_mutex_t;
typedef struct { int type; } pthread_mutexattr_t;
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE  2
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_INITIALIZER { 0, PTHREAD_MUTEX_NORMAL }

typedef unsigned long pthread_t;

typedef struct { int detachstate; size_t stacksize; } pthread_attr_t;
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

typedef struct { int unused; } pthread_condattr_t;
typedef struct { int unused; } pthread_cond_t;
#define PTHREAD_COND_INITIALIZER { 0 }

typedef struct { int unused; } pthread_rwlockattr_t;
typedef struct { int readers; int writer; } pthread_rwlock_t;
#define PTHREAD_RWLOCK_INITIALIZER { 0, 0 }

__BEGIN_DECLS
int	pthread_once __P((pthread_once_t *, void (*)(void)));
/* the destructor is stored but never run: the thread ends with the program */
int	pthread_key_create __P((pthread_key_t *, void (*)(void *)));
int	pthread_key_delete __P((pthread_key_t));
int	pthread_setspecific __P((pthread_key_t, const void *));
void	*pthread_getspecific __P((pthread_key_t));
int	pthread_mutexattr_init __P((pthread_mutexattr_t *));
int	pthread_mutexattr_destroy __P((pthread_mutexattr_t *));
int	pthread_mutexattr_settype __P((pthread_mutexattr_t *, int));
int	pthread_mutex_init __P((pthread_mutex_t *, const pthread_mutexattr_t *));
int	pthread_mutex_destroy __P((pthread_mutex_t *));
int	pthread_mutex_lock __P((pthread_mutex_t *));
int	pthread_mutex_trylock __P((pthread_mutex_t *));
int	pthread_mutex_unlock __P((pthread_mutex_t *));
pthread_t pthread_self __P((void));
int	pthread_equal __P((pthread_t, pthread_t));
int	pthread_attr_init __P((pthread_attr_t *));
int	pthread_attr_destroy __P((pthread_attr_t *));
int	pthread_attr_setdetachstate __P((pthread_attr_t *, int));
int	pthread_attr_getdetachstate __P((const pthread_attr_t *, int *));
int	pthread_attr_setstacksize __P((pthread_attr_t *, size_t));
int	pthread_attr_getstacksize __P((const pthread_attr_t *, size_t *));
int	pthread_create __P((pthread_t *, const pthread_attr_t *, void *(*)(void *), void *));
int	pthread_join __P((pthread_t, void **));
int	pthread_detach __P((pthread_t));
int	pthread_condattr_init __P((pthread_condattr_t *));
int	pthread_condattr_destroy __P((pthread_condattr_t *));
int	pthread_cond_init __P((pthread_cond_t *, const pthread_condattr_t *));
int	pthread_cond_destroy __P((pthread_cond_t *));
int	pthread_cond_signal __P((pthread_cond_t *));
int	pthread_cond_broadcast __P((pthread_cond_t *));
int	pthread_cond_wait __P((pthread_cond_t *, pthread_mutex_t *));
int	pthread_cond_timedwait __P((pthread_cond_t *, pthread_mutex_t *, const struct timespec *));
int	pthread_rwlockattr_init __P((pthread_rwlockattr_t *));
int	pthread_rwlockattr_destroy __P((pthread_rwlockattr_t *));
int	pthread_rwlock_init __P((pthread_rwlock_t *, const pthread_rwlockattr_t *));
int	pthread_rwlock_destroy __P((pthread_rwlock_t *));
int	pthread_rwlock_rdlock __P((pthread_rwlock_t *));
int	pthread_rwlock_tryrdlock __P((pthread_rwlock_t *));
int	pthread_rwlock_wrlock __P((pthread_rwlock_t *));
int	pthread_rwlock_trywrlock __P((pthread_rwlock_t *));
int	pthread_rwlock_unlock __P((pthread_rwlock_t *));
__END_DECLS

#endif /* _IXEMUL_PTHREAD_H_ */
