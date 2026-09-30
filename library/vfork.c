/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 * vfork.c,v
 *
 * Revision 1.13  2026/08/14  ChatGPT modifications (JJ)
 *
 *    Restore the 1.11 normal wait4() zombie-reaping path and process-chain
 *    traversal, relying on the existing vfork death-message sleep/wakeup
 *    lifetime invariant instead of defensive stale-pointer validation in the
 *    normal reaping and sibling-management paths.
 *
 *    Keep the 1.12 death-message preallocation, balanced exit handshake,
 *    startup-error handling and reply-pointer lifetime fixes unchanged.
 *    Retain the guarded pid < -1 process-group test in wait4().
 *
 * Revision 1.12  2026/08/10  ChatGPT modifications (JJ)
 *
 *    Preallocate each vfork child's death message in the parent before
 *    CreateNewProc(), pass ownership through a per-process pending pointer,
 *    and keep the vfork_msg layout unchanged. This removes allocator work
 *    from the Forbid-protected exit handshake without moving existing
 *    struct user member offsets.
 *
 *    Make send_death_msg() own and balance its Forbid()/Permit() section,
 *    validate the parent user area before dereferencing it, clear the vfork
 *    reply pointer immediately after ReplyMsg(), and release an unused death
 *    message when no valid parent remains.
 *
 *    Harden sibling traversal, process reparenting and wait4() against stale
 *    process pointers returned by safe_getuser(). A queued death message is
 *    treated as authoritative exit information even if the child's user area
 *    is no longer available.
 *
 *    Return -1 correctly when child startup reports an error through vm_rc,
 *    and clear the child-side vfork reply pointer before ReplyMsg() in the
 *    vfork_resume path so the parent cannot free a still-referenced message.
 *
 * Revision 1.11  2026/08/05  ChatGPT modifications (JJ)
 *
 *    Copy the parent's h_errno pointer together with errno while a vfork
 *    child borrows the parent's runtime variables.
 *
 *    Rebind the child's private SocketBase after selecting borrowed or
 *    private errno and h_errno storage.
 *
 *    Rebind the network error pointers again when vfork_own_malloc() moves
 *    borrowed error storage into the child's private allocation pool.
 * 
 * Revision 1.10  2026/06/11  ChatGPT modifications  (JJ)
 *
 * Fixes in vfork support code:
 *  - possible_childs(): avoid shadowing of u_ptr; correct process-group test
 *    so wait4(0,...) matches the caller's pgrp as intended.
 *  - ruadd(): correct off-by-one and use portable field count calculation
 *    to ensure all rusage counters are accumulated.
 *
 * No functional changes beyond these correctness fixes.
 */
  
/*
 *   $Id: vfork.c,v 1.9 1994/06/19 15:18:29 rluebbert Exp $
 *
 *  $Log: vfork.c,v $
 *  Revision 1.9  1994/06/19  15:18:29  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.7  1992/10/20  16:29:49  mwild
 *  allow a vfork'd process to use the parents memory pool. The new function
 *  vfork2() continues to use the old semantics.
 *
 *  Revision 1.6  1992/09/14  01:48:11  mwild
 *  move kmalloc() out of Forbid() (since the allocator is now Semaphore-based).
 *  move errno assignment after sigsetmask (thanks Niklas!)
 *  remove dead code
 *
 *  Revision 1.5  1992/08/09  21:01:43  amiga
 *  change to 2.x header files
 *  duplicate calling stack frame in vfork_resume() instead of just doing rts.
 *  temporary abort calling 1.3 vfork, until that's fixed again (when???).
 *
 *  Revision 1.4  1992/07/04  19:24:12  mwild
 *  get passing of environment right.
 *  change ix_sleep() calls to new semantics.
 *
 * Revision 1.3  1992/05/18  12:26:25  mwild
 * fixed bad typo that didn't close files before sending wait message.
 * Set childs Input()/Output() to NIL:, we only keep the files in our
 * own filetable.
 *
 * Revision 1.2  1992/05/18  01:02:31  mwild
 * add temporary Delay(100) before CloseLibrary() in the child after
 * vfork(), there seem to arrive some late packets (don't know why..)
 * pass NIL: filehandles as Input()/Output() to the child, so that the
 * real I/O-handles only depend on ix-filetable for usage-count
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <utility/tagitem.h>
#include <dos/dostags.h>

#include "version.h"

void vfork_own_malloc ();
void mcache_empty (void);
void volatile vfork_longjmp (jmp_buf, int);
void ruadd(struct rusage *ru, struct rusage *ru2);
void send_death_msg(struct user *mu);

struct death_msg {
  struct ixnode	        dm_node;
  struct Process	*dm_child;
  int			dm_pgrp;
  int			dm_status;
  struct rusage		dm_rusage;
};

/* this is the new process generated by vfork () ! */
static void
launcher (void)
{
  void *ixb = OpenLibrary ("ixemul.library", IX_VERSION);
  struct Process *me = (struct Process *) FindTask (0);
  struct user *u_ptr = getuser(me);
  struct vfork_msg *vm = NULL;
  int omask;

  do
    {
      if (!vm) 
        WaitPort (& me->pr_MsgPort);

      vm = (struct vfork_msg *) GetMsg (&me->pr_MsgPort);
    }
  while (!vm || vm->vm_self != me);
      
  if (ixb)
    {
      /* get parents user area */
      volatile struct user *pu = getuser(vm->vm_pptr);
      /* `my' user area. This way we don't have to recalculate it too often */
      volatile struct user *mu = &u;
      /* reaping the dup function of execve() ;-)) */
      int fd;
      int a4_size = pu->u_a4_pointers_size * 4;

      /* link ourselves into the parents process lists. Guarantee single
       * threaded access to those lists by locking out any other users of
       * the library (nicer than to just call Forbid()) */
      ix_lock_base ();

      vm->vm_clower = (int)me->pr_Task.tc_SPLower;
      vm->vm_cupper = (int)me->pr_Task.tc_SPUpper;
      /* our older sybling is the last recently created child of the parent */
      mu->p_osptr = pu->p_cptr;
      /* we have no younger sybling */
      mu->p_ysptr = 0;
      /* if we have an older sybling, point its `younger sybling' field at us */
      if (mu->p_osptr)
	safe_getuser(mu->p_osptr)->p_ysptr = me;
      /* set the parents `last recently created child' field at us */
      pu->p_cptr = me;

      /* inherit the session of our parent */
      mu->u_session = pu->u_session;
      if (mu->u_session)
        mu->u_session->s_count++; /* and increase use count */

      /* inherit the process group of our parent */
      mu->p_pgrp = pu->p_pgrp;
      mu->p_pptr = vm->vm_pptr;

      /* inherit the uid/gid information from parent */
      mu->u_ruid = pu->u_ruid;
      mu->u_euid = pu->u_euid;
      mu->u_rgid = pu->u_rgid;
      mu->u_egid = pu->u_egid;

      if ((mu->u_ngroups = pu->u_ngroups)) 
        bcopy((char *)pu->u_grouplist, (char *)mu->u_grouplist, pu->u_ngroups * sizeof(int));

      if ((mu->u_logname_valid = pu->u_logname_valid))
        strcpy((char *)mu->u_logname, (char *)pu->u_logname);

      shmfork((struct user *)pu, (struct user *)mu);
            
      /* if we got our own malloc list already, it is safe to call malloc here.
         If not, the stuff done here is postponed to either vfork_resume, or
         execve */
      if (vm->vm_own_malloc)
	{
	  /* inherit these global variables. */
	  mu->u_environ = (char ***) malloc (4);
	  * mu->u_environ = dupvec (* pu->u_environ);
	  mu->u_errno = (int *) malloc (4);
	  *mu->u_errno = 0;
	  mu->u_h_errno = (int *) malloc (4);
	  *mu->u_h_errno = 0;
	}
      else
	{
	  /* borrow the variables of the parent */
	  mu->u_environ = pu->u_environ;
	  mu->u_errno = pu->u_errno;
	  mu->u_h_errno = pu->u_h_errno;
	  
	  /* tell malloc to use the parents malloc lists */
	  mu->u_mdp = pu->u_mdp;
	}

      if (mu->u_ixnetbase)
	netcall(NET_set_errno, mu->u_errno, mu->u_h_errno);

      
      /* and inherit several other things as well, upto not including u_md */
      bcopy ((void *)&pu->u_a4_pointers_size, (void *)&mu->u_a4_pointers_size,
	     offsetof (struct user, u_md) - offsetof (struct user, u_a4_pointers_size));
      bcopy ((char *)pu - a4_size, (char *)mu - a4_size, a4_size);

      /* some things have been copied that should be reset */      
      mu->p_flag &= ~(SFREEA4 | STRC);
      mu->p_xstat = 0;
      bzero ((void *)&mu->u_ru, sizeof (struct rusage));
      bzero ((void *)&mu->u_prof, sizeof (struct uprof));
      mu->u_prof_last_pc = 0;
      bzero ((void *)&mu->u_cru, sizeof (struct rusage));
      bzero ((void *)&mu->u_timer[0], sizeof (struct itimerval)); /* just the REAL timer! */
      syscall (SYS_gettimeofday, & mu->u_start, 0);
      omask = vm->vm_rc;	/* signal mask to restore at the end */

      /* and adjust the open count of each of the copied filedescriptors */
      for (fd = 0; fd < NOFILE; fd++)
        if (mu->u_ofile[fd])
	{
	  /* obtain (and create a new fd) for INET sockets */
	  if (mu->u_ofile[fd]->f_type == DTYPE_SOCKET)
	  {
	    /* Was this socket released? */
            if (mu->u_ofile[fd]->f_socket_id)
            {
              /* Yes, it was. So we now obtain a new socket from the underlying TCP/IP stack */
	      int newfd = syscall(SYS_ix_obtain_socket, mu->u_ofile[fd]->f_socket_id,
		  mu->u_ofile[fd]->f_socket_domain,
		  mu->u_ofile[fd]->f_socket_type,
		  mu->u_ofile[fd]->f_socket_protocol);

	      mu->u_ofile[fd]->f_socket_id = 0;
	      /* move the newly created fd to the old one */
	      if (newfd != -1)
	        {
	          /* Tricky bit: we have to remember that for dupped sockets all file descriptors
	             point to the same file structure: obtaining one released file descriptor will
	             obtain them all. So we have to scan for file descriptors that point to the
	             same file structure for which we just obtained the socket and copy the new
	             file structure into the dupped file descriptors. 
	             
	             Note that the newly created file structure has the field f_socket_id set to 0,
	             so we won't come here again because of the if-statement above that tests whether
                     the socket was released. */
	          int fd2;

                  /* scan the file descriptors */
		  for (fd2 = fd + 1; fd2 < NOFILE; fd2++)
		    {
		      /* do they point to the same file structure? */
		      if (mu->u_ofile[fd] == mu->u_ofile[fd2])
		      {
		        /* in that case copy the newly obtained socket into this file descriptor 
		           and increase the open count */
		        mu->u_ofile[fd2] = mu->u_ofile[newfd];
		        mu->u_ofile[newfd]->f_count++;
		      }
		    }
		  /* and finally do the same for fd */
		  mu->u_ofile[fd] = mu->u_ofile[newfd];
		  mu->u_ofile[newfd] = 0;
	        }
	    }
	  }
	  else
	    mu->u_ofile[fd]->f_count++;
	}
      /* also copy u_segs, after all, the child will run initially in the
	 same SegList as the parent. */
      mu->u_segs = pu->u_segs;
      mu->u_start_pc = pu->u_start_pc;
      mu->u_end_pc = pu->u_end_pc;
      
      mu->u_is_root = pu->u_is_root;
      mu->u_a4 = pu->u_a4;

      /*
       * The parent reserved this death message before CreateNewProc().
       * Transfer it while the process lists are still serialized. The parent
       * remains blocked in vfork(), so clearing its pending pointer completes
       * the ownership handoff without changing struct vfork_msg.
       */
      mu->p_death_msg = pu->p_pending_death_msg;
      pu->p_pending_death_msg = 0;

      /* copying finished, allow other processes to vfork() as well ;-)) */
      ix_unlock_base ();
      
      /* remember the message we have to reply when either _exit() or 
       * execve() is called */
      mu->p_vfork_msg = vm;
      
      vm->vm_rc = 0;

      mu->u_save_sp = (void *) get_sp ();
      /* we get here when the user does an _exit() 
       * (so as well after execve() terminates !) */
      if (_setjmp ((void *)mu->u_jmp_buf))
        {
	  int i;

	  /* reset `mu' in here, setjmp() might have clobbered it */
          me = (struct Process *)FindTask(0);
          u_ptr = getuser(me);
	  mu = &u;

          if (mu->p_vfork_msg)
            {
              me->pr_Task.tc_SPLower = (void *)mu->p_vfork_msg->vm_clower;
              me->pr_Task.tc_SPUpper = (void *)mu->p_vfork_msg->vm_cupper;
            }

	  /* overkill? */
	  vfork_own_malloc ();

	  /* although this is done in CloseLibrary(), files should 
	     really be closed *before* a death-message is sent to
	     the parent. */
	  for (i = 0; i < NOFILE; i++) 
	    if (u.u_ofile[i])
              syscall (SYS_close, i);

	  /* free memory (look that most, best all memory is freed here, as
             long as we're not inside Forbid. If memory is freed in CloseLibrary,
	     it may potentially have to wait for the memory semaphore in buddy-alloc.c,
	     thus breaking the Forbid! */
	  all_free ();

	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

          /*
           * send_death_msg() owns the complete scheduler-critical handshake
           * and returns with task switching enabled. CloseLibrary() must not
           * inherit a Forbid() because library teardown may need to wait.
           */
          send_death_msg((struct user *)mu);

	  KPRINTF (("vforked: now closing library\n"));
	  CloseLibrary (ixb);

	  KPRINTF (("vforked: falling off the edge of the world.\n"));
	  /* just fall off the edge of the world, this is a process */
	  return;
        }

      syscall (SYS_sigsetmask, omask);

      KPRINTF (("vforked: jumping back\n"));

      vm = *&vm;
      me->pr_Task.tc_SPLower = (void *)vm->vm_plower;
      me->pr_Task.tc_SPUpper = (void *)vm->vm_pupper;
      /* jump into nevereverland ;-) */
      vfork_longjmp (vm->vm_regs->jb, 0);
      /* NOTREACHED */
    }

  vm->vm_rc = ENOMEM; /* can't imagine any other reason why the OpenLib should fail */
  ReplyMsg ((struct Message *) vm);
  /* fall off the edge of the world ;-) */
}

void
send_death_msg(struct user *mu)
{
  struct death_msg *dm;
  struct Process *parent;
  struct user *pu;
  struct vfork_msg *vm;

  /*
   * The parent allocated the death message before CreateNewProc(). Taking it
   * out of the user structure here makes ownership explicit: from this point
   * it is either queued on the parent's zombie list or freed here. No
   * allocator call is required inside the Forbid()-protected handshake.
   */
  dm = mu->p_death_msg;
  mu->p_death_msg = 0;
  parent = mu->p_pptr;
  vm = mu->p_vfork_msg;

  if (dm)
    {
      dm->dm_status = mu->p_xstat;
      dm->dm_rusage = mu->u_ru;
      ruadd (&dm->dm_rusage, (struct rusage *)&mu->u_cru);
      dm->dm_child = (struct Process *)FindTask (0);
      dm->dm_pgrp = mu->p_pgrp;
    }
  else
    KPRINTF (("vfork-exit: missing preallocated death message\n"));

  /*
   * Forbid() closes the lifetime race between validating the parent user area
   * and linking the death message into that parent's zombie list. This
   * function owns the matching Permit() on every path.
   */
  Forbid ();

  if (!parent || parent == (struct Process *)1)
    {
      mu->p_vfork_msg = 0;
      Permit ();

      if (dm)
        kfree (dm);

      KPRINTF (("vforked: no live parent for death message\n"));
      return;
    }

  pu = safe_getuser(parent);
  if (!pu)
    {
      /*
       * The Process pointer is no longer backed by a valid ixemul user area.
       * Do not dereference it or reply through a possibly stale message port.
       */
      mu->p_vfork_msg = 0;
      Permit ();

      if (dm)
        kfree (dm);

      KPRINTF (("vforked: parent user area disappeared before death message\n"));
      return;
    }

  if (dm)
    {
      KPRINTF (("vfork-exit: Adding child $%lx to $%lx\n",
                dm->dm_child, parent));
      ixaddtail ((struct ixlist *)&pu->p_zombies, (struct ixnode *)dm);
    }

  _psignal ((struct Task *)parent, SIGCHLD);

  /*
   * Wake wait4() explicitly even when SIGCHLD is masked or ignored. The state
   * test and wakeup are protected by the same Forbid() as the zombie-list
   * insertion, preventing a lost-wakeup window.
   */
  if (pu->p_stat == SSLEEP && pu->p_wchan == (caddr_t)pu)
    ix_wakeup ((u_int)pu);

  if (vm)
    {
      /*
       * ReplyMsg() makes the parent runnable and allows it to free the vfork
       * message immediately. Clear the child-side pointer before replying and
       * never dereference vm again afterwards.
       */
      mu->p_vfork_msg = 0;
      ReplyMsg ((struct Message *)vm);
    }

  /*
   * A normal vfork child remains alive until wait4() consumes the queued
   * death message. wait4() wakes this exact address after unlinking it.
   */
  if (dm)
    ix_sleep ((caddr_t)dm, "vfork-dm");

  Permit ();
}

/* This function is used by vfork_resume and execve. Perhaps it should be made
   externally available? It causes the process to switch to its own malloc
   list, and copies errno and environ into private space. */
void
vfork_own_malloc (void)
{
  usetup;
  /* use volatile here, or the compiler might do wrong `optimization' .. */
  volatile struct user *p = &u;

  if (p->u_mdp != &p->u_md)
    {
      char **parent_environ = *p->u_environ;
      
      /* switch to our memory list (which is initialized by OpenLibrary);
         the blocks cached so far are the parent's list's */
      p->u_mdp = (void *)&p->u_md;
      mcache_empty ();
      /* dupvec now uses malloc() on our list */
      p->u_environ = (char ***) malloc (4);
      *p->u_environ = dupvec (parent_environ);
      p->u_errno = (int *) malloc (4);
      *p->u_errno = 0;
      p->u_h_errno = (int *) malloc (4);
      *p->u_h_errno = 0;

      if (p->u_ixnetbase)
        netcall(NET_set_errno, p->u_errno, p->u_h_errno);
    }
}


asm ("
	.globl _vfork
	.globl _ix_vfork
	.globl _ix_vfork_resume
_vfork:
	| store a setjmp () compatible frame on the stack to pass to _vfork ()
	lea	sp@(-18*4),sp		| _JBLEN (17) longs on the stack
	pea	sp@
	jbsr	_setjmp
	addqw	#4,sp
	| now patch sp and pc, since they differ
	addl	#20*4,sp@(8)		| account for buffer space
	movel	sp@(18*4),sp@(20)	| insert real PC (return addr on stack)
	| tell _vfork *not* yet to switch to own malloc-list
	pea	0:W
	bsr	__vfork
	lea	sp@(18*4 + 4),sp
	rts

_ix_vfork:
	| this is the vfork used in older versions of the library
	lea	sp@(-18*4),sp		| _JBLEN (17) longs on the stack
	pea	sp@
	jbsr	_setjmp
	addqw	#4,sp
	| now patch sp and pc, since they differ
	addl	#20*4,sp@(8)		| account for buffer space
	movel	sp@(18*4),sp@(20)	| insert real PC (return addr on stack)
	| tell _vfork to have the child run with own malloc-list.
	pea	1:W
	bsr	__vfork
	lea	sp@(18*4 + 4),sp
	rts


	| the following is longjmp(), with the subtle difference that this
	| thing doesn't insist in returning something non-zero... 
_vfork_longjmp:
	movel	sp@(4),a0	/* save area pointer */
	tstl	a0@(8)		/* ensure non-zero SP */
	jeq	Lbotch		/* oops! */
	movel	sp@(8),d0	/* grab return value */
	moveml	a0@(28),d2-d7/a2-a4/a6	/* restore non-scratch regs */
	movel	a0,sp@-		/* let sigreturn */
	jbsr	_sigreturn	/*   finish for us */

Lbotch:
	jsr	_longjmperror
	stop	#0

_ix_vfork_resume:
	pea	sp@		| pass the sp containing the return address
	bsr	__vfork_resume
	addqw	#4,sp
	rts
");

/*
 * this is an implementation extension to the `real' vfork2(). Normally you
 * can only cause the parent to resume by calling _exit() or execve() from
 * the child. Since I can't provide a real fork() on the Amiga, this function
 * is a third possibility to make the parent resume. You have then two
 * concurrent processes sharing the same frame and global data... Please be
 * EXTREMELY careful what you may do and what not. vfork2() itself is a hack,
 * this is an even greater one...
 *
 * DO NOT use this function in combination with vfork(), or you'll get a big
 * memory leak. Only use it with vfork2().
 */

/*
 * don't make this function static, gcc doesn't notice that it's used from the
 * assembly code above, so make sure it isn't optimized away...
 */
void
_vfork_resume (u_int *copy_from_sp)
{
  struct Process *me = (struct Process *) FindTask (0);
  struct user *u_ptr = getuser(me);
  struct vfork_msg **vm = &u.p_vfork_msg;

  if (*vm)
    {
      u_int *sp = (u_int *)u.u_save_sp;
      u_int *copy_till_sp = (u_int *)get_sp ();

      /* be sure to switch to our memory list */
      vfork_own_malloc ();

      /* copy the stack frame */
      copy_from_sp++;
      do
	*--sp = *--copy_from_sp;
      while (copy_from_sp > copy_till_sp);

      me->pr_Task.tc_SPLower = (void *)(*vm)->vm_clower;
      me->pr_Task.tc_SPUpper = (void *)(*vm)->vm_cupper;
      set_sp ((u_int) sp);

      /*
       * The parent may free the message as soon as ReplyMsg() makes it
       * runnable. Clear our pointer before the reply and retain the message
       * only in a local variable until ReplyMsg() has consumed it.
       */
      {
        struct vfork_msg *reply_vm = *vm;

        *vm = 0;
        ReplyMsg ((struct Message *)reply_vm);
      }
    }
}

#define STACKNAME ("IXSTACK")	/* Environment variable for stack size */

static int get_stack_size(struct Process *proc)
{
  int stack_size;
  char *tmp;
  struct CommandLineInterface *CLI = BTOCPTR (proc->pr_CLI);

  if ((tmp = getenv (STACKNAME)))
    stack_size = atoi (tmp);
  else
    stack_size = CLI ? CLI->cli_DefaultStack * 4 : proc->pr_StackSize;
  if (stack_size < STACKSIZE)
    return STACKSIZE;
  return stack_size;
}

/*
 * don't make this function static, gcc doesn't notice that it's used from the
 * assembly code above, so make sure it isn't optimized away...
 */
int
_vfork (int own_malloc, struct reg_parms rp)
{
  struct Process *me = (struct Process *) FindTask(0);
  struct user *u_ptr = getuser(me);
  struct CommandLineInterface *CLI = (void *)(me->pr_CLI);
  u_int stack_size = get_stack_size(me);
  u_int plower, pupper;
  /* those *have* to be in registers to survive the stack deallocation */
  register struct vfork_msg *vm asm ("a2");
  struct death_msg *dm;
  struct Process *child;
  int i;

  struct TagItem tags [] = {
    { NP_Entry, (ULONG) launcher },
    { NP_Name, (ULONG) "vfork()'d process" },	/* to be overridden by execve() */
    { NP_StackSize, stack_size },		/* same size we use */
    { NP_Cli, (ULONG) (CLI ? -1 : 0) },	        /* same thing we are */
    { TAG_END, 0 }
  };

  KPRINTF (("vfork: creating child with stacksize = $%lx[$%lx,$%lx]\n", stack_size, tags[3]));
  vm = (struct vfork_msg *) kmalloc (sizeof (struct vfork_msg));
  if (!vm)
    {
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  /*
   * Reserve the child's eventual wait4() notification before signals are
   * blocked, sockets are released, or CreateNewProc() is attempted. This
   * guarantees that a successfully started child never has to allocate its
   * death message from the scheduler-critical exit path.
   */
  dm = (struct death_msg *)kmalloc (sizeof (struct death_msg));
  if (!dm)
    {
      kfree (vm);
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  bzero(vm, sizeof(*vm));
  vm->vm_msg.mn_ReplyPort = u.u_sync_mp;
  vm->vm_msg.mn_Node.ln_Type = NT_MESSAGE;
  vm->vm_msg.mn_Length = sizeof (struct vfork_msg);
  vm->vm_pptr = me;
  vm->vm_own_malloc = own_malloc;
  vm->vm_regs = &rp;
  vm->vm_plower = (int)me->pr_Task.tc_SPLower;
  vm->vm_pupper = (int)me->pr_Task.tc_SPUpper;

  /* we have to block all signals as long as the child uses our resources.
   * but since the child needs to start with the signal mask BEFORE this
   * general blocking, we have to pass it the old signal mask. This is a
   * way to do it */
  vm->vm_rc = vm->vm_omask = syscall (SYS_sigsetmask, ~0);

  /*
   * Publish the reserved notification only after signals are blocked. This
   * avoids exposing a half-created vfork operation to a signal handler that
   * might itself call vfork(). The child takes this pointer while the process
   * lists are serialized in launcher().
   */
  u.p_pending_death_msg = dm;

  /* save the passed frame in our user structure, since the child will
     deallocate it from the stack when it `returns' to user code */
  bcopy (&rp, &u.u_vfork_frame, sizeof (rp));

  /* if some fd's are sockets, release them so the child can obtain them */
  for (i = 0; i < NOFILE; i++)
    {
      if (u.u_ofile[i] && u.u_ofile[i]->f_type == DTYPE_SOCKET)
	u.u_ofile[i]->f_socket_id = syscall(SYS_ix_release_socket, i);
    }

  KPRINTF (("vfork: creating child with stacksize = $%lx[$%lx,$%lx]\n", stack_size, tags[3]));
  vm->vm_self = CreateNewProc (tags);

  if (!vm->vm_self)
    {
      /* do I have to close input/output here? Or does the startup close
         them no matter whether it succeeds or not ? */
      syscall (SYS_sigsetmask, vm->vm_omask);
      if (u.p_pending_death_msg)
        {
          kfree (u.p_pending_death_msg);
          u.p_pending_death_msg = 0;
        }
      kfree (vm);
      errno = EPROCLIM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  /* As soon as this message is dispatched, the child will `return' and 
     deallocate the stack we're running on. So afterwards, *only* use
     register variables and then longjmp () back.
     Since we don't have a stack until after the longjmp(), temporarily
     switch to our mini-stack */
  me->pr_Task.tc_SPLower = (void *)u.u_mini_stack;
  me->pr_Task.tc_SPUpper = (void *)(((int)u.u_mini_stack) + sizeof(u.u_mini_stack));
  set_sp((u_int)me->pr_Task.tc_SPUpper);

  PutMsg (&vm->vm_self->pr_MsgPort, (struct Message *) vm);
  /* wait until the child does execve() or _exit() */
  WaitPort (getuser(vm->vm_pptr)->u_sync_mp);
  GetMsg (getuser(vm->vm_pptr)->u_sync_mp);

  syscall (SYS_sigsetmask, vm->vm_omask);

  plower = vm->vm_plower;
  pupper = vm->vm_pupper;
  me = vm->vm_pptr;
  
  /* restore u_ptr */
  u_ptr = getuser(me);

  if (vm->vm_rc)
    {
      errno = (int)vm->vm_rc;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      /*
       * A launcher startup failure is reported as vfork() == -1. If startup
       * never completed, the child never cleared the parent's pending death
       * message, so reclaim it here before freeing the vfork message.
       */
      child = (struct Process *)-1;
      if (u.p_pending_death_msg)
        {
          kfree (u.p_pending_death_msg);
          u.p_pending_death_msg = 0;
        }
    }
  else
    child = vm->vm_self;

  /* this is the parent return, so we pass the id of the new child */
  kfree (vm);
  
  me->pr_Task.tc_SPLower = (void *)plower;
  me->pr_Task.tc_SPUpper = (void *)pupper;

  /* could use longjmp() here, but since we already *have* the local one.. */
  vfork_longjmp (u.u_vfork_frame, (int) child);
  return 0;  /* not reached */
}

void ruadd(struct rusage *ru, struct rusage *ru2)
{
	register long *ip, *ip2;
	register int i;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	i = (int)((offsetof(struct rusage, ru_last) -
	    offsetof(struct rusage, ru_first)) / sizeof(long)) + 1;
	for (; i > 0; i--)
		*ip++ += *ip2++;
}

/*
 * make process 'parent' the new parent of process 'child'.
 */
void
proc_reparent(struct Process *child, struct Process *parent)
{
	register struct Process *o;
	register struct Process *y;
	struct user *cu = safe_getuser(child);
	struct user *pu = (parent != (struct Process *)1 ? safe_getuser(parent) : 0);

	if (cu->p_pptr == parent)
	  return;

	/* fix up the child linkage for the old parent */
	o = cu->p_osptr;
	y = cu->p_ysptr;
	if (y)
	  safe_getuser(y)->p_osptr = o;
	if (o)
	  safe_getuser(o)->p_ysptr = y;
	if (cu->p_pptr && cu->p_pptr != (struct Process *)1)
	  {
	    struct user *tu = safe_getuser(cu->p_pptr);
	    if (tu->p_cptr == child)
	      tu->p_cptr = o;
	  }

        if (pu)
          {
	    /* fix up child linkage for new parent, if there is one */
	    o = pu->p_cptr;
	    if (o)
	      safe_getuser(o)->p_ysptr = child;
	    cu->p_osptr = o;
	    cu->p_ysptr = NULL;
	    pu->p_cptr = child;
	    cu->p_pptr = parent;
	  }
}

static int
possible_childs(int pid, struct Process *cptr)
{
  usetup;

  while (cptr)
  {
    struct user *cu = safe_getuser(cptr);

    if (cu->p_stat != SZOMB)
      if (pid == -1 ||
	  (pid == 0 && cu->p_pgrp == u.p_pgrp) ||
	  (pid < -1 && cu->p_pgrp == -pid) ||
	  (pid == (int)cptr))
        return TRUE;
    cptr = cu->p_osptr;
  }
  return FALSE;
}

/* This function is in desperate need of redesign !!!! */

int
wait4 (int pid, int *status, int options, struct rusage *rusage)
{
  struct Process *me = (struct Process *) FindTask (0);
  struct user *u_ptr = getuser(me);

  for (;;)
    {
      int err = 0;
      struct death_msg *dm;
      int got_node = 0;

      Forbid ();
      
      for (dm = (struct death_msg *)u.p_zombies.head;
           dm;
  	   dm = (struct death_msg *)dm->dm_node.next)
        {
  	  if (pid == -1 ||
  	      (pid == 0 && dm->dm_pgrp == u.p_pgrp) ||
	      (pid < -1 && dm->dm_pgrp == - pid) ||
	      (pid == (int) dm->dm_child))
	    {
	      got_node = 1;
	      break;
	    }
	}

      if (!got_node && !possible_childs(pid, u.p_cptr))
	err = ECHILD;

      if (got_node)
	/* Handle exited children first.  */
        {
	  struct Process *child = dm->dm_child;
	  struct user *cu = getuser(child);
	  struct Task *t;

	  /*
	   * If we got the child through a ptrace 'attach',
	   * we need to give it back to the old parent.
	   */
	  if (cu->p_opptr && (t = pfind((pid_t)cu->p_opptr)))
	    {
	      proc_reparent(child, (struct Process *)t);
	      cu->p_opptr = 0;
	      _psignal(t, SIGCHLD);
	      ix_wakeup((u_int)t);
	      Permit();
	      return (int)child;
	    }
	  KPRINTF (("wait4: unlinking child $%lx\n", dm->dm_child));
	  ixremove ((struct ixlist *)&u.p_zombies, (struct ixnode *) dm);

          cu->p_stat = SZOMB;

	  ix_wakeup ((u_int)dm);
	  Permit ();

          if (status)
          {
            *status = dm->dm_status;
          }  
          if (rusage)
            *rusage = dm->dm_rusage;

	  kfree (dm);

          return (int) child;
	}
      else
	/* No child processes have died for now.
	   Do we have a traced child process to handle?  */
	{
	  struct Process *p;
	  struct user *pu;

	  KPRINTF(("wait4: checking traced children, pid=%lx, u.p_cptr=%lx\n",
		   pid, u.p_cptr));

	  for (p = u.p_cptr; p; p = pu->p_osptr)
	    {
	      KPRINTF(("wait4: checking pid p=%lx\n", p));
	      pu = safe_getuser(p);
              if (pid == -1
	          || ((int) p) == pid
                  || (pid < -1 && pu->p_pgrp == -pid)
	          || (pid == 0
	              && u.p_pgrp == pu->p_pgrp))
	        {	      
	          KPRINTF(("wait4: pu->p_stat=%lx, pu->flag=%lx\n",
	              pu->p_stat, pu->p_flag));
	          if (pu->p_stat == SSTOP
	              && (pu->p_flag & SWTED) == 0
	              && (pu->p_flag & STRC || options & WUNTRACED))
	            {
	              KPRINTF(("wait4: SSTOPed; p_xstat=0x%lx, W_STOPCODEd=0x%lx\n",
		          pu->p_xstat, W_STOPCODE (pu->p_xstat)));
	              pu->p_flag |= SWTED;
	              if (status)
	                {
		          *status = W_STOPCODE (pu->p_xstat);
	                  KPRINTF(("wait4: status was set to %ld\n", *status));
	                }
	              Permit();
	              return (int)p;
	            }
	        }
	    }
	}

      if (!got_node && err)
	{
	  Permit();
	  errno = err;
	  KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
	  return -1;
	}

      if (options & WNOHANG)
	{
	  Permit();
	  return 0;
	}

      KPRINTF(("wait4: waiting for SIGCHLD\n"));
      ix_sleep((caddr_t)&u, "wait4");
      if (u.p_sig)
	err = EINTR;

      Permit();
      if (CURSIG(&u))
        setrun((struct Task *)me);
    }
}
