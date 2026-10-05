/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * select.c,v
 *
 * Revision 1.16  2026/08/10  ChatGPT modifications (JJ)
 *
 *  Remove skipped_wait from the callback-error cleanup decision.
 *  select_cancel_fds() is now always run after cancelling the exact
 *  failing callback, relying on SELCMD_CANCEL idempotence as required
 *  by the rollback helper. This shortens skipped_wait's live range
 *  across f_select() calls without weakening callback rollback.
 *
 * Revision 1.15  2026/08/09  ChatGPT modifications (JJ)
 *
 *  Reduce the 68020-68040 BFFFO scanner from two output temporaries to
 *  one result register. BFFFO now overwrites its isolated-bit source and
 *  performs the MSB-to-LSB index conversion inside the asm block with
 *  EORI, reducing the scanner from six to five instructions and lowering
 *  D-register pressure.
 * 
 *  The MOVE/NEG/AND bit-isolation sequence used in the 68020-68040
 *  BFFFO path was derived from assembly emitted by GCC 2.95.3 for the
 *  corresponding C implementation and then hand-optimized.
 *
 *  All select() semantics are otherwise unchanged.
 *
 * Revision 1.14  2026/08/09  ChatGPT modifications (JJ)
 *
 *  Return EBADF when a caller fd_set contains a closed descriptor.
 *  Revalidate f_select after each u_ofile[] reload before callbacks.
 *  Bound FASYNC/SIGIO sweeps to min(nfd, u_lastfile + 1) and retain
 *  the efficient single-load FASYNC callback path.
 *  Add SELCMD_CANCEL rollback for callback failures and preserve the
 *  callback errno across cancellation, timer and select-port cleanup.
 *  Keep the existing fd_set scanner, timer ordering and NET_waitselect()
 *  zero-return behaviour unchanged.
 *
 * Revision 1.13  2026/07/24  ChatGPT modifications  (JJ)
 *
 *  Restored the byte-table scanner for the 68000/68010 path after
 *  benchmark_select_fair showed it consistently outperforming both the
 *  BTST loop and the C bit-scanner, including in the normal ready-low
 *  case and in the sparse cases.
 *
 *  The 68060 De Bruijn path, 68020-68040 BFFFO path, portable C
 *  fallback, timer ordering and select() state machine are unchanged.
 *
 * Revision 1.12  2026/07/23  ChatGPT modifications  (JJ)
 *
 *  Fixed a short-timeout race in timer startup. The timer reply-port
 *  signal is now cleared and added to the wait mask before SendIO().
 *  Previously, a very short timer could complete between SendIO() and
 *  SetSignal(), after which SetSignal() erased the completion signal and
 *  Wait()/NET_waitselect() could block indefinitely.
 *
 *  Timer cancellation, WaitIO() cleanup, signal handling and the
 *  POLL -> PREPARE/WAIT/CHECK state machine are otherwise unchanged.
 *
 * Revision 1.11  2026/07/22  ChatGPT modifications  (JJ)
 *
 *  Updated the De Bruijn-plus select implementation to the same
 *  correctness and cleanup level as the current C bit-scanner
 *  implementation.
 *
 *  Added an EINVAL check for negative nfd values and limited nfd to
 *  FD_SETSIZE before direct fd_set word access. This prevents scans,
 *  zeroing and result copies from exceeding fd_set storage when NOFILE
 *  is larger than FD_SETSIZE.
 *
 *  Limited FASYNC exclusion checks to descriptors below nfd, restored
 *  per-descriptor IN -> OUT -> EXC callback ordering and re-read
 *  p->u_ofile[i] before each callback.
 *
 *  Added propagation of the reserved -1 f_select() callback return.
 *  Callback errors now preserve the callback's errno and follow the
 *  timer, select-port and process-state cleanup path instead of being
 *  treated as readiness or signal-mask bits.
 *
 *  Retained the CPU-specific fdword_first_set() implementations:
 *  De Bruijn on 68060, BFFFO on 68020-68040, BTST on 68000/68010 and
 *  the portable C fallback for other targets.
 *
 * Revision 1.3.6  2026/07/20  ChatGPT modifications  (JJ)
 *
 *   Reduced select() fd_set overhead without changing its state machine.
 *
 *   Result sets are now cleared only for active arguments and only for
 *   the words covered by nfd.  The large select_prepare_fds() and
 *   select_poll_fds() helpers were removed and their scans were placed
 *   directly in ix_select(), avoiding their argument and usetup overhead.
 *
 *   Sparse iteration now loads each 32-bit fd_set word once per pass.
 *   Processed bits are removed from the local word copy with w &= w - 1,
 *   avoiding repeated fdset_next_set() calls and repeated word reloads.
 *
 *   Timer ordering, signal handling, ixnet integration and externally
 *   visible select() semantics are unchanged.
 *
 * Revision 1.3.5  2026/06/23  ChatGPT modifications  (JJ)
 *
 *   Completed the fd_set sparse-scan conversion in ix_select().
 *   The initial descriptor sanitizing pass now uses fdset_next_set()
 *   instead of a linear 0..nfd scan, so the existing CPU-specific
 *   scanners also cover this phase.
 *
 *   Sanitizing semantics, select state-machine behaviour, timer ordering,
 *   signal handling and ixnet integration are unchanged.
 *
 * Revision 1.3.4  2026/06/16  ChatGPT modifications  (JJ)
 *
 *   Added an explicit 68000/68010 BTST fd_set scanner to
 *   fdset_next_set(). Plain 68000 builds no longer use the portable
 *   C fallback scanner in this variant. 68060 De Bruijn and
 *   68020/030/040 BFFFO paths are unchanged.
 *
 * Revision 1.3.3  2026/06/14  ChatGPT modifications  (JJ)
 *
 *   Replaced the portable fallback fd_set scanner with a new
 *   shift-based C bit scanner.
 *
 *   Plain 68000 builds now use this C scanner through the fallback
 *   path. 68060 De Bruijn and 68020/030/040 BFFFO paths are unchanged.
 *
 * Revision 1.3.2  2026/06/14  ChatGPT modifications  (JJ)
 *
 *   Removed the byte-table fd_set scanner from the 68000 path.
 *   Plain 68000 builds now use the portable C bit scanner instead.
 *
 *   68060 still uses De Bruijn; 68020/030/040 still use BFFFO.
 *
 * Revision 1.3.1  2026/06/10  Copilot/ChatGPT modifications  (JJ)
 *
 *   Corrected CPU dispatch in fdset_next_set() so that the intended
 *   CPU-specific implementations are selected:
 *
 *     68060         De Bruijn multiply-and-table LSB scan
 *     68020-68040   BFFFO fast path
 *     68000/68010   byte-table scanner
 *     other targets portable C fallback
 *
 *   The 68000 branch is now guarded so that a generic __mc68000__
 *   definition cannot shadow the 68020/030/040 branches.
 *
 *   Removed unnecessary fd_set copies in select_prepare_fds() and
 *   switched to direct read-only iteration via fdset_next_set(),
 *   eliminating redundant FD_CLR() operations. No semantic changes.
 *
 *   Updated state-machine documentation to reflect the actual
 *   SELCMD_POLL -> SELCMD_PREPARE -> Wait()/NET_waitselect() ->
 *   SELCMD_CHECK sequence.
 *
 * Revision 1.3  2026/06/05  Copilot/ChatGPT modifications  (JJ)
 *
 *   Added a 32-bit De Bruijn multiply-and-table LSB scanner for the
 *   68060 implementation of fdset_next_set().
 * 
 * Revision 1.2.2  2026/05/30  Copilot modifications  (JJ)
 *  Replaced 68060/68000 BTST loops in fdset_next_set() with a C89
 *  table-based byte scanner (fdset_next_set_tab()). Preserved 020-040
 *  BFFFO path and fallback C logic. No semantic changes.
 * 
 * Revision 1.2.1  2026/02/20  Copilot modifications  (JJ)
 *
 *    Introduced CPU-optimized m68k inline assembly into fdset_next_set()
 *    for all 680x0 processors (68000/020/030/040/060), replacing the
 *    original linear C-based bit scan. This provided a uniform fast-path
 *    across all m68k CPUs and significantly improved performance on
 *    dense fd_sets.
 *
 * Revision 1.2  2026/02/15  Copilot modifications  (JJ)
 *
 *  Modernized select() subsystem in four waves:
 *
 *    Wave 1: Added semantically neutral documentation describing the
 *            select() state-machine invariant, timer ordering rules,
 *            and select-port behaviour.
 *
 *    Wave 2: Introduced minor structural cleanups and helper extraction
 *            without altering control flow or semantics.
 *
 *    Wave 3: Added fdset helpers (fdset_next_set(), setcopy()) to
 *            replace linear scans with efficient bit-iteration.
 *
 *    Wave 4: Fully refactored select_prepare_fds() and select_poll_fds()
 *            into clear PREPARE/POLL phases while preserving the
 *            POLL -> PREPARE/WAIT/CHECK invariant and historical behaviour.
 *
 *  No functional changes to external select() semantics.
 *
 *
 * --------------------------------------------------------------------
 *  INTERNAL NOTES FOR MAINTAINERS
 *
 *  1. STATE MACHINE INVARIANT
 *
 *     Correct invariant:
 *       First iteration:
 *           SELCMD_POLL
 *       Later iterations:
 *           SELCMD_PREPARE -> Wait()/NET_waitselect() -> SELCMD_CHECK
 *
 *     This behaviour is required for BSD compatibility and ixnet
 *     integration. It is implemented via:
 *
 *         for (skipped_wait = 0; ; skipped_wait = 1)
 *
 *     Do NOT alter this logic.
 *
 *  2. TIMER REQUEST ORDERING
 *
 *     Clear the timer reply-port signal before SendIO(); clearing it
 *     afterwards can erase an already delivered short-timeout signal.
 *     The timer.device request must then be queued before DOS packet
 *     polling and dequeued before fd polling. Otherwise the timer request
 *     may be mistaken for a DOS packet, causing missed timeouts or
 *     select() never waking.
 *
 *  3. SELECT PORT BEHAVIOUR
 *
 *     u.u_select_mp may receive stray DOS packets. These must be drained
 *     (dp_Port = 0) or Wait() may block indefinitely.
 *
 * --------------------------------------------------------------------
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <strings.h>

#include "select.h"

#if UINT_MAX != 0xFFFFFFFFU
# error "fdword_first_set De Bruijn-plus scanner requires 32-bit u_int"
#endif

#define FDSET_WORD_SHIFT  5
#define FDSET_WORD_MASK   31

#define __time_req (u.u_time_req)
#define __tport    (u.u_sync_mp)

static void handle_select_port(void)
{
	usetup;

	struct StandardPacket *prw;

	while ((prw = GetPacket(u.u_select_mp)))
		prw->sp_Pkt.dp_Port = 0;
}

#if defined(__mc68060__)
/*
 * 68060 De Bruijn LSB lookup table.
 *
 * The isolated least-significant set bit is multiplied by the
 * 32-bit De Bruijn constant and reduced to a five-bit index into
 * this 32-entry table.  The table maps that index to bit 0..31.
 */
static const unsigned char debruijn32_idx[32] = {
	0, 1, 28, 2, 29, 14, 24, 3,
	30, 22, 20, 15, 25, 17, 4, 8,
	31, 27, 13, 23, 21, 19, 16, 7,
	26, 12, 18, 6, 11, 5, 10, 9
};
#endif

#if defined(__mc68000__) && !defined(__mc68020__) && \
    !defined(__mc68030__) && !defined(__mc68040__) && \
    !defined(__mc68060__)
/*
 * 68000/68010 byte-table lookup.
 *
 * firstbit[byte] gives the index, 0..7, of the least-significant
 * set bit in the byte, or -1 if the byte is zero.
 *
 * fdword_first_set() performs the lookup only after locating a
 * non-zero byte, so firstbit[0] is never used as a valid result.
 */
static const signed char firstbit[256] = {
	-1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};
#endif

/*
 * SPARSE FD_SET WORD SCANNING
 *
 * fd_set is assumed to be represented as a dense array of 32-bit
 * u_int words, matching the representation used by FD_SET(), FD_CLR()
 * and setcopy(). This is also an implicit assumption in the historical
 * ixemul select implementation. Changing the fd_set layout requires all
 * direct word-access helpers and scans to be updated together.
 *
 * UINT_MAX is checked above because the CPU-specific scanners depend on
 * 32-bit u_int words. In particular, the 68060 De Bruijn constant and
 * table are defined for exactly 32 bits.
 *
 * fdword_first_set() is called only with a non-zero word and returns the
 * index, 0..31, of its least-significant set bit. It does not modify the
 * caller's fd_set.
 *
 * The caller keeps a local copy of each fd_set word and removes the bit
 * just processed with:
 *
 *     pending &= pending - 1U;
 *
 * Each fd_set word is therefore loaded once per scan pass.
 *
 * CPU-specific implementations:
 *
 *   68060
 *	  De Bruijn multiply-and-table LSB scan implemented in C. The least-
 *	  significant set bit is isolated, multiplied by the De Bruijn constant
 * 	  and converted to a table index. The compiler is left free to allocate
 * 	  registers and schedule the resulting instructions within ix_select().
 *
 *   68020/68030/68040
 *     BFFFO fast path. The least-significant set bit is isolated with
 *     w & -w and BFFFO determines its position. The optimized inline
 *     assembly uses one scratch/result D register in addition to w.
 *
 *   68000/68010
 *     Byte-table C path. The four bytes are examined from least to
 *     most significant and the first non-zero byte is resolved through
 *     firstbit[].
 *
 *   Other targets
 *     Portable shift-based C scanner.
 */

static __inline__ int
fdword_first_set(u_int w)
{
#if defined(__mc68060__)
	u_int low;
	u_int idx;

	low = w & (0U - w);
	idx = (low * 0x077CB531U) >> 27;
	return (int)debruijn32_idx[idx];

#elif defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__)
	u_int bit;

   /*
 	* Use only the input word and one early-clobber result D register.
 	* BFFFO may overwrite the isolated-bit source after reading it.
 	* For a BFFFO result in the range 0..31, x ^ 31 is exactly
 	* 31 - x, yielding the LSB bit index expected by the callers.
 	*/
	asm volatile (
		"move.l %1,%0\n\t"
		"neg.l  %0\n\t"
		"and.l  %1,%0\n\t"
		"bfffo  %0{0:32},%0\n\t"
		"eori.l #31,%0\n\t"
		: "=&d"(bit)
		: "d"(w)
		: "cc"
	);

	return (int)bit;

#elif defined(__mc68000__)
	unsigned char b;

	b = (unsigned char)(w & 0xFFU);
	if (b)
		return (int)firstbit[b];

	b = (unsigned char)((w >> 8) & 0xFFU);
	if (b)
		return 8 + (int)firstbit[b];

	b = (unsigned char)((w >> 16) & 0xFFU);
	if (b)
		return 16 + (int)firstbit[b];

	b = (unsigned char)((w >> 24) & 0xFFU);
	return 24 + (int)firstbit[b];
	
#else
	int bit;

	bit = 0;
	while (!(w & 1U))
	{
		w >>= 1;
		bit++;
	}

	return bit;
#endif
}

/*
 * Clear only the fd_set words that can be copied back to the caller.
 * Unused trailing words are never observed by setcopy().
 */
static void
fdset_zero_n(int nfd, fd_set *set)
{
	u_int *p;
	int nwords;

	p = (u_int *)set;
	nwords = (nfd + FDSET_WORD_MASK) >> FDSET_WORD_SHIFT;

	while (nwords-- > 0)
		*p++ = 0;
}

static void
setcopy(int nfd, u_int *ifd, u_int *ofd)
{
	nfd = (nfd + FDSET_WORD_MASK) >> FDSET_WORD_SHIFT;
	while (nfd--) *ofd++ = *ifd++;
}

/*
 * Undo caller-specific state that may have been installed by
 * SELCMD_PREPARE in the current select cycle.
 *
 * SELCMD_CANCEL is required to be idempotent. Therefore this error-only
 * rescan may also visit callbacks whose PREPARE was not reached, or whose
 * state was already removed by SELCMD_CHECK before a later callback failed.
 */
static void
select_cancel_fds(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd,
                  struct user *p)
{
	u_int *inwords;
	u_int *outwords;
	u_int *excwords;
	int nwords;
	int word;

	inwords = ifd ? (u_int *)ifd : NULL;
	outwords = ofd ? (u_int *)ofd : NULL;
	excwords = efd ? (u_int *)efd : NULL;
	nwords = (nfd + FDSET_WORD_MASK) >> FDSET_WORD_SHIFT;

	for (word = 0; word < nwords; word++)
	{
		u_int inword;
		u_int outword;
		u_int excword;
		u_int pending;

		inword = inwords ? inwords[word] : 0;
		outword = outwords ? outwords[word] : 0;
		excword = excwords ? excwords[word] : 0;
		pending = inword | outword | excword;

		while (pending)
		{
			struct file *f;
			u_int bitmask;
			int bit;
			int fd;

			bit = fdword_first_set(pending);
			bitmask = 1U << bit;
			fd = (word << FDSET_WORD_SHIFT) + bit;

			if (fd >= nfd)
				break;

			if ((inword & bitmask)
			    && (f = p->u_ofile[fd]) != NULL && f->f_select)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_IN,
				                  NULL, NULL);

			if ((outword & bitmask)
			    && (f = p->u_ofile[fd]) != NULL && f->f_select)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_OUT,
				                  NULL, NULL);

			if ((excword & bitmask)
			    && (f = p->u_ofile[fd]) != NULL && f->f_select)
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_EXC,
				                  NULL, NULL);

			pending &= pending - 1U;
		}
	}

	/*
	 * Legacy FASYNC preparation is separate from the caller's descriptor
	 * sets. Mirror the normal FASYNC range and read-set exclusion here.
	 */
	if (!(p->p_sigignore & sigmask(SIGIO)))
	{
		int maxfd;
		int fd;

		maxfd = p->u_lastfile + 1;
		if (maxfd > nfd)
			maxfd = nfd;

		for (fd = 0; fd < maxfd; fd++)
		{
			struct file *f;

			f = p->u_ofile[fd];
			if (f && f->f_select && (f->f_flags & FASYNC)
			    && !(ifd && FD_ISSET(fd, ifd)))
				(void)f->f_select(f, SELCMD_CANCEL, SELMODE_IN,
				                  NULL, NULL);
		}
	}
}

int
ix_select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd, struct timeval *timeout, long *mask)
{
	usetup;
	struct file *f;
	int i, dotout, nwords;
	int result = 0, ostat, sigio = 0;
	int select_error = 0;
	u_int wait_sigs;
	u_int origmask = mask ? *mask : 0;
	int skipped_wait;
	struct user *p = &u;
	u_long recv_wait_sigs = 0;
	u_long net_nfds;

	if (CURSIG (p))
	{
		*(p->u_errno) = EINTR;
		return -1;
	}

	if (nfd < 0)
	{
		*(p->u_errno) = EINVAL;
		return -1;
	}

	if (nfd > NOFILE)
		nfd = NOFILE;

	if (nfd > FD_SETSIZE)
		nfd = FD_SETSIZE;

	nwords = (nfd + FDSET_WORD_MASK) >> FDSET_WORD_SHIFT;

	if (ifd)
	{
		u_int *words;
		int word;

		words = (u_int *)ifd;

		for (word = 0; word < nwords; word++)
		{
			u_int pending;

			pending = words[word];

			while (pending)
			{
				int bit;

				bit = fdword_first_set(pending);
				i = (word << FDSET_WORD_SHIFT) + bit;

				if (i >= nfd)
					break;

				f = p->u_ofile[i];
				if (f == NULL)
				{
					*(p->u_errno) = EBADF;
					return -1;
				}

				if (!f->f_read || !f->f_select)
					FD_CLR(i, ifd);

				pending &= pending - 1U;
			}
		}
	}

	if (ofd)
	{
		u_int *words;
		int word;

		words = (u_int *)ofd;

		for (word = 0; word < nwords; word++)
		{
			u_int pending;

			pending = words[word];

			while (pending)
			{
				int bit;

				bit = fdword_first_set(pending);
				i = (word << FDSET_WORD_SHIFT) + bit;

				if (i >= nfd)
					break;

				f = p->u_ofile[i];
				if (f == NULL)
				{
					*(p->u_errno) = EBADF;
					return -1;
				}

				if (!f->f_write || !f->f_select)
					FD_CLR(i, ofd);

				pending &= pending - 1U;
			}
		}
	}

	if (efd)
	{
		u_int *words;
		int word;

		words = (u_int *)efd;

		for (word = 0; word < nwords; word++)
		{
			u_int pending;

			pending = words[word];

			while (pending)
			{
				int bit;

				bit = fdword_first_set(pending);
				i = (word << FDSET_WORD_SHIFT) + bit;

				if (i >= nfd)
					break;

				f = p->u_ofile[i];
				if (f == NULL)
				{
					*(p->u_errno) = EBADF;
					return -1;
				}

				if (!f->f_read || !f->f_select)
					FD_CLR(i, efd);

				pending &= pending - 1U;
			}
		}
	}

	dotout = (timeout && timerisset(timeout));

	ostat = p->p_stat;
	p->p_stat = SSLEEP;
	p->p_wchan = (caddr_t) select;
	p->p_wmesg = "select";

	for (skipped_wait = 0; ; skipped_wait=1)
	{
		fd_set readyin, readyout, readyexc;
		fd_set netin, netout, netexc;
		int tout, readydesc, cmd;
		int selret;
		int timer_active;

		if (ifd)
			fdset_zero_n(nfd, &readyin);
		if (ofd)
			fdset_zero_n(nfd, &readyout);
		if (efd)
			fdset_zero_n(nfd, &readyexc);

		if (u.u_ixnetbase)
		{
			FD_ZERO(&netin);
			FD_ZERO(&netout);
			FD_ZERO(&netexc);
			net_nfds = 0;
		}

		tout = readydesc = 0;
		timer_active = 0;

		wait_sigs = SIGBREAKF_CTRL_C | (1 << p->u_sleep_sig) | origmask;

		handle_select_port();

		if (skipped_wait)
		{
			cmd = SELCMD_CHECK;

			if (dotout)
			{
				__time_req->tr_time.tv_sec = timeout->tv_sec;
				__time_req->tr_time.tv_usec = timeout->tv_usec;
				__time_req->tr_node.io_Command = TR_ADDREQUEST;

				/* Clear a stale timer signal before the new request can
				 * complete; clearing it after SendIO() loses short timeouts.
				 */
				SetSignal (0, 1U << __tport->mp_SigBit);
				wait_sigs |= 1U << __tport->mp_SigBit;

				SendIO((struct IORequest *)__time_req);
				timer_active = 1;
			}
		   /*
 			* PREPARE must not modify the caller's fd_sets.
 			*
 			* The requested descriptor sets are scanned read-only.  Local word
 			* copies are used to build wait_sigs and the netin/netout/netexc sets
 			* required by the upcoming Wait()/NET_waitselect().
 			*/
			{
				u_int *inwords;
				u_int *outwords;
				u_int *excwords;
				int word;

				inwords = ifd ? (u_int *)ifd : NULL;
				outwords = ofd ? (u_int *)ofd : NULL;
				excwords = efd ? (u_int *)efd : NULL;

				for (word = 0; word < nwords; word++)
				{
					u_int inword;
					u_int outword;
					u_int excword;
					u_int pending;

					inword = inwords ? inwords[word] : 0;
					outword = outwords ? outwords[word] : 0;
					excword = excwords ? excwords[word] : 0;
					pending = inword | outword | excword;

					while (pending)
					{
						u_int bitmask;
						int bit;

						bit = fdword_first_set(pending);
						bitmask = 1U << bit;
						i = (word << FDSET_WORD_SHIFT) + bit;

						if (i >= nfd)
							break;

						if ((inword & bitmask)
						    && (f = p->u_ofile[i]) != NULL
						    && f->f_select != NULL)
						{
							selret = f->f_select(f, SELCMD_PREPARE,
							                     SELMODE_IN, &netin,
							                     &net_nfds);
							if (selret == -1)
							{
								i = SELMODE_IN;
								goto select_callback_error;
							}
							wait_sigs |= (u_int)selret;
						}

						if ((outword & bitmask)
						    && (f = p->u_ofile[i]) != NULL
						    && f->f_select != NULL)
						{
							selret = f->f_select(f, SELCMD_PREPARE,
							                     SELMODE_OUT, &netout,
							                     &net_nfds);
							if (selret == -1)
							{
								i = SELMODE_OUT;
								goto select_callback_error;
							}
							wait_sigs |= (u_int)selret;
						}

						if ((excword & bitmask)
						    && (f = p->u_ofile[i]) != NULL
						    && f->f_select != NULL)
						{
							selret = f->f_select(f, SELCMD_PREPARE,
							                     SELMODE_EXC, &netexc,
							                     &net_nfds);
							if (selret == -1)
							{
								i = SELMODE_EXC;
								goto select_callback_error;
							}
							wait_sigs |= (u_int)selret;
						}

						pending &= pending - 1U;
					}
				}
			}

			if (!(u.p_sigignore & sigmask (SIGIO)))
			{
				struct file **files = u.u_ofile;
				int maxfd;

				maxfd = u.u_lastfile + 1;
				if (maxfd > nfd)
					maxfd = nfd;

				for (i = 0; i < maxfd; i++)
				{
					f = files[i];
					if (f && (f->f_flags & FASYNC) && f->f_select
					    && !(ifd && i < nfd && FD_ISSET(i, ifd)))
					{
						selret = f->f_select(f, SELCMD_PREPARE, SELMODE_IN,
						                     &netin, &net_nfds);
						if (selret == -1)
						{
							i = SELMODE_IN;
							goto select_callback_error;
						}
						wait_sigs |= (u_int)selret;
					}
				}
			}

			if (u.u_ixnetbase)
				recv_wait_sigs = netcall(NET_waitselect, wait_sigs,
					&netin, &netout, &netexc, net_nfds);
			else
				while (!(recv_wait_sigs = Wait (wait_sigs))) ;
           /*
 			* Do not return EINTR here.  Preserve the historical SIGINT
 			* semantics by deferring signal handling until after timer cleanup.
 			* recv_wait_sigs is processed later, including CTRL-C -> SIGINT
 			* delivery, in the original select() ordering.
 			*/
			if (mask)
				*mask = recv_wait_sigs & origmask;

			if (dotout)
			{
				if (!CheckIO ((struct IORequest *)__time_req))
					AbortIO ((struct IORequest *)__time_req);
				else
					recv_wait_sigs |= 1 << __tport->mp_SigBit;
				WaitIO ((struct IORequest *)__time_req);
				timer_active = 0;
			}

			handle_select_port();
		}
		else
			cmd = SELCMD_POLL;
	   /*
 		* POLL/CHECK scans the caller's requested descriptor sets read-only.
 		* Readiness is recorded only in readyin/readyout/readyexc; the caller's
 		* fd_sets are copied back only when select() is ready to return.
 		*/
		readydesc = 0;

		{
			u_int *inwords;
			u_int *outwords;
			u_int *excwords;
			int word;

			inwords = ifd ? (u_int *)ifd : NULL;
			outwords = ofd ? (u_int *)ofd : NULL;
			excwords = efd ? (u_int *)efd : NULL;

			for (word = 0; word < nwords; word++)
			{
				u_int inword;
				u_int outword;
				u_int excword;
				u_int pending;

				inword = inwords ? inwords[word] : 0;
				outword = outwords ? outwords[word] : 0;
				excword = excwords ? excwords[word] : 0;
				pending = inword | outword | excword;

				while (pending)
				{
					u_int bitmask;
					int bit;

					bit = fdword_first_set(pending);
					bitmask = 1U << bit;
					i = (word << FDSET_WORD_SHIFT) + bit;

					if (i >= nfd)
						break;

					if ((inword & bitmask)
					    && (f = p->u_ofile[i]) != NULL
					    && f->f_select != NULL)
					{
						selret = f->f_select(f, cmd, SELMODE_IN,
						                     &netin, NULL);
						if (selret == -1)
						{
							i = SELMODE_IN;
							goto select_callback_error;
						}
						if (selret)
						{
							FD_SET(i, &readyin);
							readydesc++;
						}
					}

					if ((outword & bitmask)
					    && (f = p->u_ofile[i]) != NULL
					    && f->f_select != NULL)
					{
						selret = f->f_select(f, cmd, SELMODE_OUT,
						                     &netout, NULL);
						if (selret == -1)
						{
							i = SELMODE_OUT;
							goto select_callback_error;
						}
						if (selret)
						{
							FD_SET(i, &readyout);
							readydesc++;
						}
					}

					if ((excword & bitmask)
					    && (f = p->u_ofile[i]) != NULL
					    && f->f_select != NULL)
					{
						selret = f->f_select(f, cmd, SELMODE_EXC,
						                     &netexc, NULL);
						if (selret == -1)
						{
							i = SELMODE_EXC;
							goto select_callback_error;
						}
						if (selret)
						{
							FD_SET(i, &readyexc);
							readydesc++;
						}
					}

					pending &= pending - 1U;
				}
			}
		}

		if (!readydesc && dotout)
			tout = recv_wait_sigs & (1 << __tport->mp_SigBit);

		sigio = 0;
		if (!(u.p_sigignore & sigmask (SIGIO)))
		{
			struct file **files = u.u_ofile;
			int maxfd;

			maxfd = u.u_lastfile + 1;
			if (maxfd > nfd)
				maxfd = nfd;

			for (i = 0; i < maxfd; i++)
			{
				f = files[i];
				if (f && (f->f_flags & FASYNC) && f->f_select
				    && !(ifd && i < nfd && FD_ISSET(i, ifd)))
				{
					selret = f->f_select(f, cmd, SELMODE_IN, &netin, NULL);
					if (selret == -1)
					{
						i = SELMODE_IN;
						goto select_callback_error;
					}
					if (selret)
						sigio = 1;
				}
			}
		}

		if (readydesc || tout || (timeout && !timerisset(timeout)))
		{
			if (ifd) setcopy(nfd, (u_int *)&readyin,  (u_int *)ifd);
			if (ofd) setcopy(nfd, (u_int *)&readyout, (u_int *)ofd);
			if (efd) setcopy(nfd, (u_int *)&readyexc, (u_int *)efd);
			result = readydesc;
			break;
		}

		if (sigio || (recv_wait_sigs & (SIGBREAKF_CTRL_C | (1 << p->u_sleep_sig) | origmask)))
		{
			result = -1;
			break;
		}

		continue;

select_callback_error:
		/*
		 * Preserve the callback errno before cancellation and cleanup.
		 * i has been repurposed as the failing callback's SELMODE value;
		 * f still identifies the file whose callback returned -1.
		 */
		result = *(p->u_errno);
		(void)f->f_select(f, SELCMD_CANCEL, i, NULL, NULL);

		/*
		 * SELCMD_CANCEL is required to be idempotent, so always rescan.
		 * This also avoids keeping skipped_wait live into the error path.
		 */
		select_cancel_fds(nfd, ifd, ofd, efd, p);

		if (timer_active)
		{
			if (!CheckIO ((struct IORequest *)__time_req))
				AbortIO ((struct IORequest *)__time_req);
			WaitIO ((struct IORequest *)__time_req);
			timer_active = 0;
		}
		handle_select_port();
		*(p->u_errno) = result;
		select_error = 1;
		result = -1;
		break;
	}

	p->p_wchan = 0;
	p->p_wmesg = 0;
	p->p_stat = ostat;

	/* If NET_waitselect/Wait returned error sentinel, propagate error. */
	if (recv_wait_sigs == (u_long)-1)
		return -1;

	/* Restore historical behavior: translate CTRL-C (SIGBREAKF_CTRL_C)
	 * into a SIGINT delivered to the process, as original implementation did.
	 */
	if (recv_wait_sigs & SIGBREAKF_CTRL_C)
		_psignal(FindTask(0), SIGINT);

	if (sigio)
		_psignal(FindTask(0), SIGIO);
	setrun(FindTask(0));

	/* Preserve historical EINTR handling for signal-driven failures only.
	 * An f_select() callback returning -1 has already supplied its own errno.
	 */
	if (result == -1 && !select_error)
		*(p->u_errno) = EINTR;

	return result;
}

int
select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd, struct timeval *timeout)
{
	return ix_select(nfd, ifd, ofd, efd, timeout, NULL);
}
