/*
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * poll.c,v
 *
 *    include hierarchy, matching the pattern used by pipe.c.
 *
 * Revision 1.4  2026/09/02  ChatGPT modifications (JJ)
 *
 *    Replace the revision 1.3 select()-wrapper with a direct implementation
 *    based on ixemul's existing f_select() readiness protocol.
 *
 *    Use NOFILE-sized private bitmaps for ixemul descriptor numbers, so the
 *    public poll() descriptor range is no longer limited by FD_SETSIZE.
 *    fd_set remains in use only for the backend ixnet socket sets populated
 *    by f_select(), preserving the existing public fd_set ABI.
 *
 *    Follow the current select.c readiness state machine: an initial
 *    SELCMD_POLL probe, followed when necessary by
 *    SELCMD_PREPARE -> Wait()/NET_waitselect() -> SELCMD_CHECK.
 *
 *    Preserve select.c timer ordering, select-port draining, ixnet waiting,
 *    FASYNC/SIGIO handling, historical CTRL-C/SIGINT delivery and callback
 *    errno across SELCMD_CANCEL rollback.
 *
 *    Preserve poll() handling of negative descriptors, per-entry POLLNVAL,
 *    duplicate pollfd entries, millisecond timeouts, POLLPRI and pipe
 *    POLLHUP/POLLERR conditions without changing struct file or fd_set.
 *
 * Revision 1.3  2026/09/02  ChatGPT modifications (JJ)
 *
 *    Include the declarations required by select() and FD_ZERO()/bzero().
 *    Correct descriptor validation so u_lastfile itself remains valid.
 *    Report exceptional select readiness as POLLPRI rather than POLLERR.
 *
 * Revision 1.2  2026/08/21  ChatGPT (JJ)
 *
 *    Add a C89-compatible poll() implementation using ixemul select().
 *    Ignore negative descriptors as required by poll() semantics.
 *    Validate positive descriptors directly against the ixemul descriptor
 *    table and report invalid descriptors with POLLNVAL.
 *    Preserve timeout semantics when no descriptors are monitored.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <poll.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>

#include "select.h"

#if UINT_MAX != 0xFFFFFFFFU
# error "poll() NOFILE bitmaps require 32-bit u_int"
#endif

#define POLL_WORD_SHIFT 5
#define POLL_WORD_MASK  31
#define POLL_WORDS      ((NOFILE + POLL_WORD_MASK) >> POLL_WORD_SHIFT)

#define __time_req (u.u_time_req)
#define __tport    (u.u_sync_mp)

static void
poll_handle_select_port(void)
{
	usetup;
	struct StandardPacket *prw;

	while ((prw = GetPacket(u.u_select_mp)))
		prw->sp_Pkt.dp_Port = 0;
}

static void
poll_bits_zero(u_int *bits)
{
	int i;

	for (i = 0; i < POLL_WORDS; i++)
		bits[i] = 0;
}

static void
poll_bit_set(u_int *bits, int fd)
{
	bits[fd >> POLL_WORD_SHIFT] |= 1U << (fd & POLL_WORD_MASK);
}

static int
poll_bit_isset(const u_int *bits, int fd)
{
	return (bits[fd >> POLL_WORD_SHIFT] &
	        (1U << (fd & POLL_WORD_MASK))) != 0;
}

/*
 * The ixemul pipe backend already makes a reader ready when the writer has
 * disappeared and makes a writer ready when the reader has disappeared.
 * poll() additionally exposes those conditions using the conventional
 * POLLHUP/POLLERR result bits.
 */
static short
poll_pipe_revents(struct file *f)
{
	struct sock_stream *ss;
	short revents;

	if (f == NULL || f->f_type != DTYPE_PIPE)
		return 0;

	revents = 0;

	Forbid();
	ss = f->f_ss;
	if (ss != NULL)
	{
		if (f->f_read != NULL && (ss->flags & UNF_NO_WRITER))
			revents |= POLLHUP;
		if (f->f_write != NULL && (ss->flags & UNF_NO_READER))
			revents |= POLLERR;
	}
	Permit();

	return revents;
}

static void
poll_cancel_fds(int maxfd, const u_int *watch_in, const u_int *watch_out,
                const u_int *watch_exc, struct user *p)
{
	int fd;

	for (fd = 0; fd <= maxfd; fd++)
	{
		struct file *f;

		if (poll_bit_isset(watch_in, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_IN,
			                  NULL, NULL);
		}

		if (poll_bit_isset(watch_out, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_OUT,
			                  NULL, NULL);
		}

		if (poll_bit_isset(watch_exc, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_EXC,
			                  NULL, NULL);
		}
	}

	/*
	 * select() also prepares FASYNC descriptors below its descriptor limit.
	 * Mirror that behaviour below poll()'s highest watched descriptor.
	 */
	if (!(p->p_sigignore & sigmask(SIGIO)))
	{
		int async_max;

		async_max = p->u_lastfile;
		if (async_max > maxfd)
			async_max = maxfd;

		for (fd = 0; fd <= async_max; fd++)
		{
			struct file *f;

			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL &&
			    (f->f_flags & FASYNC) && !poll_bit_isset(watch_in, fd))
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_IN,
			                  NULL, NULL);
		}
	}
}

/*
 * Run the requested readiness callbacks once.  PREPARE accumulates wait
 * signals; POLL and CHECK record ready descriptors in the ready bitmaps.
 * The file table is reloaded before each mode callback, matching select().
 */
static int
poll_run_callbacks(int cmd, int maxfd,
                   const u_int *watch_in, const u_int *watch_out,
                   const u_int *watch_exc,
                   u_int *ready_in, u_int *ready_out, u_int *ready_exc,
                   fd_set *netin, fd_set *netout, fd_set *netexc,
                   u_long *net_nfds, u_int *wait_sigs,
                   struct user *p, struct file **failed_file,
                   int *failed_mode)
{
	int fd;

	for (fd = 0; fd <= maxfd; fd++)
	{
		struct file *f;
		int selret;

		if (poll_bit_isset(watch_in, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
			{
				selret = f->f_select(f, cmd, SELMODE_IN, netin,
				                     cmd == SELCMD_PREPARE ? net_nfds : NULL);
				if (selret == -1)
				{
					*failed_file = f;
					*failed_mode = SELMODE_IN;
					return -1;
				}
				if (cmd == SELCMD_PREPARE)
					*wait_sigs |= (u_int)selret;
				else if (selret)
					poll_bit_set(ready_in, fd);
			}
		}

		if (poll_bit_isset(watch_out, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
			{
				selret = f->f_select(f, cmd, SELMODE_OUT, netout,
				                     cmd == SELCMD_PREPARE ? net_nfds : NULL);
				if (selret == -1)
				{
					*failed_file = f;
					*failed_mode = SELMODE_OUT;
					return -1;
				}
				if (cmd == SELCMD_PREPARE)
					*wait_sigs |= (u_int)selret;
				else if (selret)
					poll_bit_set(ready_out, fd);
			}
		}

		if (poll_bit_isset(watch_exc, fd))
		{
			f = p->u_ofile[fd];
			if (f != NULL && f->f_select != NULL)
			{
				selret = f->f_select(f, cmd, SELMODE_EXC, netexc,
				                     cmd == SELCMD_PREPARE ? net_nfds : NULL);
				if (selret == -1)
				{
					*failed_file = f;
					*failed_mode = SELMODE_EXC;
					return -1;
				}
				if (cmd == SELCMD_PREPARE)
					*wait_sigs |= (u_int)selret;
				else if (selret)
					poll_bit_set(ready_exc, fd);
			}
		}
	}

	return 0;
}

static int
poll_run_fasync(int cmd, int maxfd, const u_int *watch_in,
                fd_set *netin, u_long *net_nfds, u_int *wait_sigs,
                struct user *p, struct file **failed_file,
                int *failed_mode)
{
	int fd;
	int sigio;

	if (p->p_sigignore & sigmask(SIGIO))
		return 0;

	sigio = 0;
	if (maxfd > p->u_lastfile)
		maxfd = p->u_lastfile;

	for (fd = 0; fd <= maxfd; fd++)
	{
		struct file *f;
		int selret;

		f = p->u_ofile[fd];
		if (f == NULL || f->f_select == NULL || !(f->f_flags & FASYNC) ||
		    poll_bit_isset(watch_in, fd))
			continue;

		selret = f->f_select(f, cmd, SELMODE_IN, netin,
		                     cmd == SELCMD_PREPARE ? net_nfds : NULL);
		if (selret == -1)
		{
			*failed_file = f;
			*failed_mode = SELMODE_IN;
			return -1;
		}

		if (cmd == SELCMD_PREPARE)
			*wait_sigs |= (u_int)selret;
		else if (selret)
			sigio = 1;
	}

	return sigio;
}

static int
poll_map_revents(struct pollfd *fds, nfds_t nfds,
                 const u_int *ready_in, const u_int *ready_out,
                 const u_int *ready_exc, struct user *p)
{
	nfds_t i;
	int count;

	count = 0;

	for (i = 0; i < nfds; i++)
	{
		struct file *f;
		int fd;
		short revents;

		fd = fds[i].fd;
		revents = 0;

		if (fd < 0)
		{
			fds[i].revents = 0;
			continue;
		}

		if (fd >= NOFILE || (f = p->u_ofile[fd]) == NULL)
			revents = POLLNVAL;
		else
		{
			if (poll_bit_isset(ready_in, fd))
				revents |= fds[i].events & (POLLIN | POLLRDNORM);
			if (poll_bit_isset(ready_out, fd))
				revents |= fds[i].events &
				           (POLLOUT | POLLWRNORM | POLLWRBAND);
			if (poll_bit_isset(ready_exc, fd))
				revents |= fds[i].events & (POLLPRI | POLLRDBAND);

			revents |= poll_pipe_revents(f);
		}

		fds[i].revents = revents;
		if (revents != 0)
			count++;
	}

	return count;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	usetup;
	struct user *p;
	u_int watch_in[POLL_WORDS];
	u_int watch_out[POLL_WORDS];
	u_int watch_exc[POLL_WORDS];
	u_int ready_in[POLL_WORDS];
	u_int ready_out[POLL_WORDS];
	u_int ready_exc[POLL_WORDS];
	struct timeval tv;
	struct file *failed_file;
	int failed_mode;
	int maxfd;
	int result;
	int ostat;
	int poll_error;
	int sigio;
	u_long recv_wait_sigs;
	nfds_t n;

	p = &u;

	if (nfds > (nfds_t)NOFILE)
	{
		*(p->u_errno) = EINVAL;
		return -1;
	}

	if (nfds != 0 && fds == NULL)
	{
		*(p->u_errno) = EFAULT;
		return -1;
	}

	if (CURSIG(p))
	{
		*(p->u_errno) = EINTR;
		return -1;
	}

	poll_bits_zero(watch_in);
	poll_bits_zero(watch_out);
	poll_bits_zero(watch_exc);
	poll_bits_zero(ready_in);
	poll_bits_zero(ready_out);
	poll_bits_zero(ready_exc);

	maxfd = -1;
	for (n = 0; n < nfds; n++)
	{
		struct file *f;
		int fd;

		fds[n].revents = 0;
		fd = fds[n].fd;

		if (fd < 0 || fd >= NOFILE)
			continue;

		f = p->u_ofile[fd];
		if (f == NULL || f->f_select == NULL)
			continue;

		if ((fds[n].events & (POLLIN | POLLRDNORM)) && f->f_read != NULL)
			poll_bit_set(watch_in, fd);
		if ((fds[n].events & (POLLOUT | POLLWRNORM | POLLWRBAND)) &&
		    f->f_write != NULL)
			poll_bit_set(watch_out, fd);
		if ((fds[n].events & (POLLPRI | POLLRDBAND)) && f->f_read != NULL)
			poll_bit_set(watch_exc, fd);

		if (fd > maxfd)
			maxfd = fd;
	}

	poll_handle_select_port();

	/* Initial non-blocking probe, matching select()'s first POLL phase. */
	poll_bits_zero(ready_in);
	poll_bits_zero(ready_out);
	poll_bits_zero(ready_exc);
	{
		fd_set netin;
		fd_set netout;
		fd_set netexc;
		u_long net_nfds;
		u_int dummy_wait;

		FD_ZERO(&netin);
		FD_ZERO(&netout);
		FD_ZERO(&netexc);
		net_nfds = 0;
		dummy_wait = 0;
		failed_file = NULL;
		failed_mode = SELMODE_IN;

		if (poll_run_callbacks(SELCMD_POLL, maxfd,
		                       watch_in, watch_out, watch_exc,
		                       ready_in, ready_out, ready_exc,
		                       &netin, &netout, &netexc, &net_nfds,
		                       &dummy_wait, p, &failed_file,
		                       &failed_mode) < 0)
		{
			result = *(p->u_errno);
			if (failed_file != NULL && failed_file->f_select != NULL)
				(void)failed_file->f_select(failed_file, SELCMD_CANCEL,
				                            failed_mode, NULL, NULL);
			poll_cancel_fds(maxfd, watch_in, watch_out, watch_exc, p);
			*(p->u_errno) = result;
			return -1;
		}

		sigio = poll_run_fasync(SELCMD_POLL, maxfd, watch_in,
		                         &netin, &net_nfds, &dummy_wait, p,
		                         &failed_file, &failed_mode);
		if (sigio < 0)
		{
			result = *(p->u_errno);
			if (failed_file != NULL && failed_file->f_select != NULL)
				(void)failed_file->f_select(failed_file, SELCMD_CANCEL,
				                            failed_mode, NULL, NULL);
			poll_cancel_fds(maxfd, watch_in, watch_out, watch_exc, p);
			*(p->u_errno) = result;
			return -1;
		}
	}

	result = poll_map_revents(fds, nfds, ready_in, ready_out, ready_exc, p);
	if (result != 0 || timeout == 0)
	{
		if (sigio > 0)
		{
			_psignal(FindTask(0), SIGIO);
			setrun(FindTask(0));
		}
		return result;
	}

	if (sigio > 0)
	{
		_psignal(FindTask(0), SIGIO);
		setrun(FindTask(0));
		*(p->u_errno) = EINTR;
		return -1;
	}

	if (timeout > 0)
	{
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
	}
	else
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}

	ostat = p->p_stat;
	p->p_stat = SSLEEP;
	p->p_wchan = (caddr_t)poll;
	p->p_wmesg = "poll";

	result = 0;
	poll_error = 0;
	sigio = 0;
	recv_wait_sigs = 0;

	for (;;)
	{
		fd_set netin;
		fd_set netout;
		fd_set netexc;
		u_long net_nfds;
		u_int wait_sigs;
		int timer_active;
		int timedout;
		int async_result;

		FD_ZERO(&netin);
		FD_ZERO(&netout);
		FD_ZERO(&netexc);
		net_nfds = 0;
		wait_sigs = SIGBREAKF_CTRL_C | (1U << p->u_sleep_sig);
		timer_active = 0;
		timedout = 0;
		failed_file = NULL;
		failed_mode = SELMODE_IN;

		poll_handle_select_port();

		if (timeout > 0)
		{
			__time_req->tr_time.tv_sec = tv.tv_sec;
			__time_req->tr_time.tv_usec = tv.tv_usec;
			__time_req->tr_node.io_Command = TR_ADDREQUEST;

			/*
			 * Match select.c: clear the timer signal before SendIO() so a
			 * very short timeout cannot complete and then be erased.
			 */
			SetSignal(0, 1U << __tport->mp_SigBit);
			wait_sigs |= 1U << __tport->mp_SigBit;
			SendIO((struct IORequest *)__time_req);
			timer_active = 1;
		}

		if (poll_run_callbacks(SELCMD_PREPARE, maxfd,
		                       watch_in, watch_out, watch_exc,
		                       ready_in, ready_out, ready_exc,
		                       &netin, &netout, &netexc, &net_nfds,
		                       &wait_sigs, p, &failed_file,
		                       &failed_mode) < 0)
			goto poll_callback_error;

		async_result = poll_run_fasync(SELCMD_PREPARE, maxfd, watch_in,
		                                 &netin, &net_nfds, &wait_sigs, p,
		                                 &failed_file, &failed_mode);
		if (async_result < 0)
			goto poll_callback_error;

		if (u.u_ixnetbase)
			recv_wait_sigs = netcall(NET_waitselect, wait_sigs,
			                         &netin, &netout, &netexc, net_nfds);
		else
			while (!(recv_wait_sigs = Wait(wait_sigs))) ;

		if (timeout > 0)
		{
			if (!CheckIO((struct IORequest *)__time_req))
				AbortIO((struct IORequest *)__time_req);
			else
				recv_wait_sigs |= 1U << __tport->mp_SigBit;
			WaitIO((struct IORequest *)__time_req);
			timer_active = 0;
		}

		poll_handle_select_port();

		poll_bits_zero(ready_in);
		poll_bits_zero(ready_out);
		poll_bits_zero(ready_exc);

		if (poll_run_callbacks(SELCMD_CHECK, maxfd,
		                       watch_in, watch_out, watch_exc,
		                       ready_in, ready_out, ready_exc,
		                       &netin, &netout, &netexc, &net_nfds,
		                       &wait_sigs, p, &failed_file,
		                       &failed_mode) < 0)
			goto poll_callback_error;

		async_result = poll_run_fasync(SELCMD_CHECK, maxfd, watch_in,
		                                 &netin, &net_nfds, &wait_sigs, p,
		                                 &failed_file, &failed_mode);
		if (async_result < 0)
			goto poll_callback_error;
		if (async_result)
			sigio = 1;

		result = poll_map_revents(fds, nfds,
		                           ready_in, ready_out, ready_exc, p);

		if (result == 0 && timeout > 0)
			timedout = (recv_wait_sigs & (1U << __tport->mp_SigBit)) != 0;

		if (result != 0 || timedout)
		{
			if (timedout)
				result = 0;
			break;
		}

		if (sigio ||
		    (recv_wait_sigs & (SIGBREAKF_CTRL_C | (1U << p->u_sleep_sig))))
		{
			result = -1;
			break;
		}

		continue;

poll_callback_error:
		result = *(p->u_errno);
		if (failed_file != NULL && failed_file->f_select != NULL)
			(void)failed_file->f_select(failed_file, SELCMD_CANCEL,
			                            failed_mode, NULL, NULL);
		poll_cancel_fds(maxfd, watch_in, watch_out, watch_exc, p);

		if (timer_active)
		{
			if (!CheckIO((struct IORequest *)__time_req))
				AbortIO((struct IORequest *)__time_req);
			WaitIO((struct IORequest *)__time_req);
			timer_active = 0;
		}
		poll_handle_select_port();
		*(p->u_errno) = result;
		poll_error = 1;
		result = -1;
		break;
	}

	p->p_wchan = 0;
	p->p_wmesg = 0;
	p->p_stat = ostat;

	if (recv_wait_sigs == (u_long)-1)
		return -1;

	if (recv_wait_sigs & SIGBREAKF_CTRL_C)
		_psignal(FindTask(0), SIGINT);

	if (sigio)
		_psignal(FindTask(0), SIGIO);
	setrun(FindTask(0));

	if (result == -1 && !poll_error)
		*(p->u_errno) = EINTR;

	return result;
}
