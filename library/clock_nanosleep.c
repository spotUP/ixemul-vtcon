/*
 * This file is part of ixemul.library for the Amiga.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * clock_nanosleep.c,v
 *
 * Revision 1.1  2026/08/12  ChatGPT modifications (JJ)
 *
 *    Added POSIX clock_nanosleep() support for CLOCK_REALTIME and
 *    CLOCK_MONOTONIC, including TIMER_ABSTIME.  CLOCK_MONOTONIC relative
 *    sleeps compute EINTR remaining time from the monotonic clock rather
 *    than nanosleep()'s wall-clock helper.  Absolute sleeps repeatedly
 *    derive the remaining interval from clock_gettime().  Error numbers
 *    are returned directly as required by clock_nanosleep().
 */

#include <time.h>

#define _KERNEL
#include "ixemul.h"

static int
clock_nanosleep_timespec_cmp(const struct timespec *a,
                             const struct timespec *b)
{
    if (a->tv_sec < b->tv_sec)
        return -1;
    if (a->tv_sec > b->tv_sec)
        return 1;
    if (a->tv_nsec < b->tv_nsec)
        return -1;
    if (a->tv_nsec > b->tv_nsec)
        return 1;
    return 0;
}

static void
clock_nanosleep_timespec_sub(struct timespec *result,
                             const struct timespec *a,
                             const struct timespec *b)
{
    result->tv_sec = a->tv_sec - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;

    if (result->tv_nsec < 0)
    {
        result->tv_sec--;
        result->tv_nsec += 1000000000L;
    }
}

static void
clock_nanosleep_remaining(struct timespec *remaining,
                           const struct timespec *requested,
                           const struct timespec *start,
                           const struct timespec *end)
{
    struct timespec elapsed;

    if (clock_nanosleep_timespec_cmp(end, start) < 0)
    {
        *remaining = *requested;
        return;
    }

    clock_nanosleep_timespec_sub(&elapsed, end, start);

    if (clock_nanosleep_timespec_cmp(&elapsed, requested) >= 0)
    {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
        return;
    }

    clock_nanosleep_timespec_sub(remaining, requested, &elapsed);
}

int
clock_nanosleep(clockid_t clock_id, int flags,
                const struct timespec *rqtp, struct timespec *rmtp)
{
    struct timespec now;
    struct timespec delay;
    int saved_errno;
    int error;

    usetup;

    if (rqtp == NULL)
        return EFAULT;

    if ((flags & ~TIMER_ABSTIME) != 0)
        return EINVAL;

    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC)
        return EINVAL;

    if (rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000L)
        return EINVAL;

    if ((flags & TIMER_ABSTIME) == 0)
    {
        struct timespec start;
        struct timespec end;
        int have_start;

        if (rqtp->tv_sec < 0)
            return EINVAL;

        /*
         * Existing nanosleep() computes rmtp with gettimeofday().  That is
         * suitable for CLOCK_REALTIME, but CLOCK_MONOTONIC must not let a
         * wall-clock adjustment distort the remaining interval.
         */
        if (clock_id == CLOCK_REALTIME || rmtp == NULL)
        {
            saved_errno = errno;

            if (nanosleep(rqtp, rmtp) == 0)
            {
                errno = saved_errno;
                return 0;
            }

            error = errno;
            errno = saved_errno;
            return error;
        }

        saved_errno = errno;
        have_start = clock_gettime(CLOCK_MONOTONIC, &start) == 0;
        if (!have_start)
        {
            error = errno;
            errno = saved_errno;
            return error;
        }
        errno = saved_errno;

        if (nanosleep(rqtp, NULL) == 0)
        {
            errno = saved_errno;
            return 0;
        }

        error = errno;

        if (error == EINTR &&
            clock_gettime(CLOCK_MONOTONIC, &end) == 0)
            clock_nanosleep_remaining(rmtp, rqtp, &start, &end);
        else if (error == EINTR)
            *rmtp = *rqtp;

        errno = saved_errno;
        return error;
    }

    /*
     * For an absolute request, rmtp must not be modified.  Re-evaluate
     * the selected clock after every completed relative wait so a caller
     * may safely reuse the same absolute deadline after interruptions.
     */
    for (;;)
    {
        saved_errno = errno;

        if (clock_gettime(clock_id, &now) < 0)
        {
            error = errno;
            errno = saved_errno;
            return error;
        }

        errno = saved_errno;

        if (clock_nanosleep_timespec_cmp(rqtp, &now) <= 0)
            return 0;

        clock_nanosleep_timespec_sub(&delay, rqtp, &now);

        saved_errno = errno;

        if (nanosleep(&delay, NULL) < 0)
        {
            error = errno;
            errno = saved_errno;
            return error;
        }

        errno = saved_errno;
    }
}
