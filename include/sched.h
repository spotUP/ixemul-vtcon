/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* <sched.h>: the types and policy numbers only. The toolchain's newlib
 * sys-include/sched.h pulls in sys/_timespec.h, whose struct timespec guard
 * differs from ixemul's <time.h>/<sys/time.h> (_STRUCT_TIMESPEC_DECLARED), so
 * a program including both failed with "redefinition of struct timespec".
 * ixemul has no scheduler calls (sched_yield, sched_setscheduler, ...), so none
 * are declared: a program that uses them fails at compile time, not at link. */
#ifndef _SCHED_H_
#define _SCHED_H_

#define SCHED_OTHER	0
#define SCHED_FIFO	1
#define SCHED_RR	2

struct sched_param {
	int sched_priority;
};

#endif /* _SCHED_H_ */
