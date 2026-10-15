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
