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
 * clock_getres.c,v
 *
 * Revision 1.1  2026/08/12  ChatGPT modifications (JJ)
 *
 *    Added POSIX clock_getres() support for CLOCK_REALTIME and
 *    CLOCK_MONOTONIC.  CLOCK_MONOTONIC resolution is derived from
 *    the timer.device E-Clock frequency returned by ReadEClock().
 */

#include <time.h>

#define _KERNEL
#include "ixemul.h"
#include <inline/timer.h>

int
clock_getres(clockid_t clk_id, struct timespec *res)
{
    usetup;
    struct EClockVal eclock;
    struct Device *TimerBase;
    ULONG frequency;
    ULONG nsec;

    switch (clk_id)
    {
        case CLOCK_REALTIME:
            if (res != NULL)
            {
                res->tv_sec = 0;
                res->tv_nsec = 1000L;
            }
            return 0;

        case CLOCK_MONOTONIC:
            if (u.u_time_req == NULL ||
                u.u_time_req->tr_node.io_Device == NULL)
            {
                errno = EIO;
                return -1;
            }

            TimerBase = u.u_time_req->tr_node.io_Device;
            frequency = ReadEClock(&eclock);

            if (frequency == 0)
            {
                errno = EIO;
                return -1;
            }

            if (res != NULL)
            {
                /* Round one E-Clock tick up to the next nanosecond. */
                nsec = 1000000000UL / frequency;
                if ((1000000000UL % frequency) != 0)
                    nsec++;

                res->tv_sec = 0;
                res->tv_nsec = (long)nsec;
            }

            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}
