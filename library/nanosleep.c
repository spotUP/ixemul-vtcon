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
 *
 */

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

int gettimeofday(struct timeval *, struct timezone *);

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
	struct timeval tv;
	struct timeval start;
	struct timeval end;
	time_t req_sec;
	time_t rem_sec;
	time_t elapsed_sec;
	long req_usec;
	long rem_usec;
	long elapsed_usec;
	int save_errno;
	int have_start;

	usetup;

	if (req == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (req->tv_sec < 0 ||
	    req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
		errno = EINVAL;
		return -1;
	}

	req_sec = req->tv_sec;
	req_usec = (req->tv_nsec + 999L) / 1000L;

	if (req_usec >= 1000000L) {
		req_sec++;
		req_usec -= 1000000L;
	}

	tv.tv_sec = req_sec;
	tv.tv_usec = req_usec;

	have_start = 0;
	if (rem != NULL) {
		if (gettimeofday(&start, NULL) == 0)
			have_start = 1;
		else {
			/*
			 * Sleeping itself does not require gettimeofday().
			 * If we cannot measure remaining time, continue anyway.
			 */
			have_start = 0;
		}
	}

	if (select(0, NULL, NULL, NULL, &tv) < 0) {
		save_errno = errno;

		if (save_errno == EINTR && rem != NULL) {
			if (have_start && gettimeofday(&end, NULL) == 0) {
				elapsed_sec = end.tv_sec - start.tv_sec;
				elapsed_usec = end.tv_usec - start.tv_usec;

				if (elapsed_usec < 0) {
					elapsed_sec--;
					elapsed_usec += 1000000L;
				}

				rem_sec = req_sec - elapsed_sec;
				rem_usec = req_usec - elapsed_usec;

				if (rem_usec < 0) {
					rem_sec--;
					rem_usec += 1000000L;
				}

				if (rem_sec < 0) {
					rem->tv_sec = 0;
					rem->tv_nsec = 0;
				} else {
					rem->tv_sec = rem_sec;
					rem->tv_nsec = rem_usec * 1000L;
				}
			} else {
				/*
				 * Could not compute elapsed time. Returning the
				 * original request is safer than claiming that no
				 * time remains.
				 */
				rem->tv_sec = req->tv_sec;
				rem->tv_nsec = req->tv_nsec;
			}
		}

		errno = save_errno;
		return -1;
	}

	return 0;
}
