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
 * clock_gettime.c,v
 *
 * Revision 1.1  2026/08/12  ChatGPT modifications (JJ)
 *
 *    Added CLOCK_MONOTONIC using timer.device ReadEClock().
 *    The 64-bit E-Clock value is converted using 32-bit arithmetic
 *    so the implementation remains C89-compatible with GCC 2.95.3.
 *    CLOCK_REALTIME behaviour is unchanged.
 */

#include <time.h>
#include <limits.h>

#define _KERNEL
#include "ixemul.h"
#include <inline/timer.h>

static int
clock_gettime_eclock_to_timespec(const struct EClockVal *value,
                                 ULONG frequency,
                                 struct timespec *tp)
{
    ULONG seconds;
    ULONG remainder;
    ULONG nsec;
    ULONG fraction;
    int bit;
    int digit;

    /*
     * The conversion below doubles remainders and multiplies the
     * fractional remainder by ten.  The real Amiga E-Clock frequency
     * is far below this limit; keep the arithmetic explicitly bounded.
     */
    if (frequency == 0 || frequency > ULONG_MAX / 10UL ||
        value->ev_hi >= frequency)
        return -1;

    /*
     * Divide the unsigned 64-bit E-Clock value by the 32-bit frequency.
     * value->ev_hi < frequency guarantees that the quotient fits in
     * 32 bits.  This avoids requiring long long support.
     */
    seconds = 0;
    remainder = value->ev_hi;

    for (bit = 31; bit >= 0; bit--)
    {
        remainder = (remainder << 1) |
                    ((value->ev_lo >> bit) & 1UL);

        if (remainder >= frequency)
        {
            remainder -= frequency;
            seconds |= (1UL << bit);
        }
    }

    /*
     * Convert the fractional remainder to nanoseconds without forming
     * remainder * 1000000000, which would overflow a 32-bit ULONG.
     */
    nsec = 0;
    fraction = remainder;

    for (digit = 0; digit < 9; digit++)
    {
        fraction *= 10UL;
        nsec = nsec * 10UL + fraction / frequency;
        fraction %= frequency;
    }

    if (seconds > (ULONG)LONG_MAX)
        return -1;

    tp->tv_sec = (time_t)seconds;
    tp->tv_nsec = (long)nsec;

    return 0;
}

int
clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    usetup;
    struct timeval tv;
    struct EClockVal eclock;
    struct Device *TimerBase;
    ULONG frequency;

    if (tp == NULL)
    {
        errno = EFAULT;
        return -1;
    }

    switch (clk_id)
    {
        case CLOCK_REALTIME:
            if (syscall(SYS_gettimeofday, &tv, 0) < 0)
                return -1;

            tp->tv_sec = (time_t)tv.tv_secs;
            tp->tv_nsec = (long)tv.tv_micro * 1000L;
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

            if (clock_gettime_eclock_to_timespec(&eclock, frequency, tp) < 0)
            {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE;
#endif
                return -1;
            }

            return 0;

        default:
            errno = EINVAL;
            return -1;
    }
}
