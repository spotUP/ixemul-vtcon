/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* Host test only (force-included with -include): renames the pthread names
 * of ../include/pthread.h and pthread.c to ixc_*, so they do not collide
 * with the host's <sys/types.h> and <pthread.h>. The host headers that
 * declare them come first, under their own names. */
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#define pthread_once_t ixc_pthread_once_t
#define pthread_key_t ixc_pthread_key_t
#define pthread_mutex_t ixc_pthread_mutex_t
#define pthread_mutexattr_t ixc_pthread_mutexattr_t
#define pthread_t ixc_pthread_t
#define pthread_once ixc_pthread_once
#define pthread_key_create ixc_pthread_key_create
#define pthread_key_delete ixc_pthread_key_delete
#define pthread_setspecific ixc_pthread_setspecific
#define pthread_getspecific ixc_pthread_getspecific
#define pthread_mutexattr_init ixc_pthread_mutexattr_init
#define pthread_mutexattr_destroy ixc_pthread_mutexattr_destroy
#define pthread_mutexattr_settype ixc_pthread_mutexattr_settype
#define pthread_mutex_init ixc_pthread_mutex_init
#define pthread_mutex_destroy ixc_pthread_mutex_destroy
#define pthread_mutex_lock ixc_pthread_mutex_lock
#define pthread_mutex_trylock ixc_pthread_mutex_trylock
#define pthread_mutex_unlock ixc_pthread_mutex_unlock
#define pthread_self ixc_pthread_self
#define pthread_equal ixc_pthread_equal
#define pthread_attr_t ixc_pthread_attr_t
#define pthread_cond_t ixc_pthread_cond_t
#define pthread_condattr_t ixc_pthread_condattr_t
#define pthread_rwlock_t ixc_pthread_rwlock_t
#define pthread_rwlockattr_t ixc_pthread_rwlockattr_t
#define pthread_attr_destroy ixc_pthread_attr_destroy
#define pthread_attr_getdetachstate ixc_pthread_attr_getdetachstate
#define pthread_attr_getstacksize ixc_pthread_attr_getstacksize
#define pthread_attr_init ixc_pthread_attr_init
#define pthread_attr_setdetachstate ixc_pthread_attr_setdetachstate
#define pthread_attr_setstacksize ixc_pthread_attr_setstacksize
#define pthread_cond_broadcast ixc_pthread_cond_broadcast
#define pthread_cond_destroy ixc_pthread_cond_destroy
#define pthread_cond_init ixc_pthread_cond_init
#define pthread_cond_signal ixc_pthread_cond_signal
#define pthread_cond_timedwait ixc_pthread_cond_timedwait
#define pthread_cond_wait ixc_pthread_cond_wait
#define pthread_condattr_destroy ixc_pthread_condattr_destroy
#define pthread_condattr_init ixc_pthread_condattr_init
#define pthread_create ixc_pthread_create
#define pthread_detach ixc_pthread_detach
#define pthread_join ixc_pthread_join
#define pthread_mutex_t ixc_pthread_mutex_t
#define pthread_rwlock_destroy ixc_pthread_rwlock_destroy
#define pthread_rwlock_init ixc_pthread_rwlock_init
#define pthread_rwlock_rdlock ixc_pthread_rwlock_rdlock
#define pthread_rwlock_tryrdlock ixc_pthread_rwlock_tryrdlock
#define pthread_rwlock_trywrlock ixc_pthread_rwlock_trywrlock
#define pthread_rwlock_unlock ixc_pthread_rwlock_unlock
#define pthread_rwlock_wrlock ixc_pthread_rwlock_wrlock
#define pthread_rwlockattr_destroy ixc_pthread_rwlockattr_destroy
#define pthread_rwlockattr_init ixc_pthread_rwlockattr_init
#define pthread_t ixc_pthread_t
