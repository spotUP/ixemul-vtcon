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
 *
 * 
 * Revision 1.1.8  2026/08/29  ChatGPT modifications (JJ)
 *
 *   Drain residual process-private ACTION_WAIT_CHAR packets while the
 *   ixemul trap, task hooks and process select message port are still valid.
 *
 * Revision 1.1.7  2026/07/21  SIGWINCH teardown fix, ChatGPT (JJ)
 *
 * - ix_close: Remove the input.device SIGWINCH handler before process
 *   teardown can release the request, reply port, user structure or task
 *   data referenced by the handler.  The removal function is idempotent,
 *   so this is also safe when an earlier exit path already removed it.
 * 
 * Revision 1.1.6  2026/06/03  Robustness and maintainability improvements (JJ)
 *
 * - __ix_close_muFS: Replaced while(u_setuid--) with a local counter
 *   to prevent underflow if u_setuid becomes corrupted.
 *
 * - ix_stack_usage: Made stack marker scanning bounds-safe while
 *   preserving original scan semantics (first tested byte is
 *   tc_SPLower + 1). Added a defensive check to avoid dereferencing
 *   at or beyond tc_SPUpper.
 *
 * - ix_stack_usage: Improved program/path name construction to use
 *   length-aware concatenation and avoid buffer overruns.
 *
 * - __ix_close_muFS: Fixed indentation inconsistencies.
 *
 * - ix_close: Moved local variable declarations to the top for strict
 *   C89 compatibility.
 *
 * - ix_close: Reworked child traversal to use an explicit next pointer,
 *   eliminating dependence on traversal state updated inside the loop.
 *
 * - ix_close: Removed shadowing of u_ptr in the parent update path.
 *
 * - ix_close: Reworked zombie cleanup to remove each node with ixremove(),
 *   preserving ixlist head/tail invariants; wakeups are performed under
 *   Forbid(), matching wait4(), while kfree() is done outside Forbid().
 *
 * Notes:
 * - These changes focus on defensive checks, clearer teardown order,
 *   and avoiding off?by?one and buffer issues while keeping behavior
 *   compatible with existing semantics where intentional.
 *
 *
 * NULL guard update, Copilot (JJ)
 * 
 * Changes made:
 *
 * 1. Logout loop - replaced while(u_setuid--) with local counter
 * 2. FindTask(0) - added NULL checks in ix_close() and ix_stack_usage()
 * 3. u_time_req - added NULL guard before CloseDevice
 * 4. Port deletion - added NULL checks for u_select_mp and u_sync_mp
 * 5. Child loop - rewrote while loop with NULL guard on safe_getuser()
 * 6. p_ysptr - added NULL guard on safe_getuser()
 * 7. p_osptr - added NULL guard on safe_getuser()
 * 8. Parent pointer - fixed NULL check (moved before dereference)
 *
 * These 8 modification points result in 15 individual guard changes,
 * as some points (such as port deletion) cover multiple guards
 * (u_select_mp and u_sync_mp).
 *
 * Inline teardown comments added 2025-01-11 (JJ) with Copilot assistance.
 *
 * These comments document the teardown sequence inside ix_close(),
 * explaining why each step must occur in a specific order and what
 * assumptions are safe at this stage of process destruction.
 *
 * The goal is to make ix_close() self-explanatory and safer to
 * maintain, without altering any functional behaviour.
 * 
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "select.h"
#include <hardware/intbits.h>
#include "multiuser.h"
#include <string.h>

void
__ix_close_muFS( struct user *ix_u )
{
  if (muBase)
    {
      if (ix_u->u_UserInfo)
        {
          muFreeUserInfo(ix_u->u_UserInfo);
          ix_u->u_UserInfo = NULL;
        }

      if (ix_u->u_fileUserInfo)
        {
          muFreeUserInfo(ix_u->u_fileUserInfo);
          ix_u->u_fileUserInfo = NULL;
        }

      if (ix_u->u_GroupInfo)
        {
          muFreeGroupInfo(ix_u->u_GroupInfo);
          ix_u->u_GroupInfo = NULL;
        }

      if (ix_u->u_fileGroupInfo)
        {
          muFreeGroupInfo(ix_u->u_fileGroupInfo);
          ix_u->u_fileGroupInfo = NULL;
        }

      /* Log me out - use local counter to prevent underflow
       * if u_setuid is corrupted
       */
      {
        int n = ix_u->u_setuid;
        while (n-- > 0)
          muLogout(muT_Quiet, TRUE, TAG_DONE);
        ix_u->u_setuid = 0;
      }
    }
}

void ix_stack_usage(void)
{
  if (ix.ix_flags & ix_show_stack_usage)
    {
      struct Task *me;
      BPTR lock;
      struct SUMessage sum;
      struct MsgPort *port, *reply;
      u_char *tmp;
      size_t len;
      u_char *upper;

      me = FindTask(0);
      if (!me)
        return;

      lock = GetProgramDir();
      upper = (u_char *)me->tc_SPUpper;

      if ((reply = (struct MsgPort *)ix_create_port(0, 0)))
        {
          bzero(&sum, sizeof(sum));
          sum.name[0] = '\0';

          /* Base name on current program directory if available.
           * Ensure explicit NUL-termination even if NameFromLock()
           * fills the buffer completely.
           */
          if (lock)
            {
              NameFromLock(lock, sum.name, sizeof(sum.name) - 1);
              sum.name[sizeof(sum.name) - 1] = '\0';
            }

          tmp = (u_char *)me->tc_SPLower;
          /* Walk up from stack lower bound, preserving original semantics
           * (first tested byte is tc_SPLower + 1) but never dereference
           * at or beyond tc_SPUpper.
           */
          while (tmp + 1 < upper && *++tmp == 0xdb)
            ;

          /* Append "/" if we have a directory name and space for it. */
          len = strlen(sum.name);
          if (len < sizeof(sum.name) - 1)
            {
              if (lock && len > 0 && sum.name[len - 1] != '/')
                {
                  sum.name[len++] = '/';
                  sum.name[len] = '\0';
                }
            }

          /* Append program name, limited to remaining space. */
          len = strlen(sum.name);
          if (len < sizeof(sum.name) - 1)
            {
              GetProgramName(sum.name + len, sizeof(sum.name) - len - 1);
              /* Guard: enforce NUL-termination after GetProgramName()
               * regardless of its internal behaviour.
               */
              sum.name[sizeof(sum.name) - 1] = '\0';
            }

          sum.stack_size = (u_long)me->tc_SPUpper - (u_long)me->tc_SPLower;
          sum.stack_usage = (u_long)me->tc_SPUpper - (u_long)tmp;
          sum.msg.mn_Node.ln_Type = NT_MESSAGE;
          sum.msg.mn_Length = sizeof(sum);
          sum.msg.mn_ReplyPort = reply;
          Forbid();
          port = FindPort("ixstack port");
          if (port)
            PutMsg(port, (struct Message *)&sum);
          Permit();
          if (port)
            WaitPort(reply);
          ix_delete_port(reply);
        }
    }
}

void
ix_close (struct ixemul_base *ixbase)
{
  struct Task           *me;
  struct user           *u_ptr;
  struct user           *ix_u;
  struct Process        *child;
  struct user           *cu;
  struct ixnode         *dm;
  struct Process        *next;    /* for child loop traversal */

  /* Guard: Ensure current task is valid before proceeding */
  me = FindTask(0);
  if (!me)
    {
      KPRINTF(("ix_close: FindTask(0) returned NULL!\n"));
      return;
    }

  u_ptr = getuser(me);
  ix_u = &u;

  /* Remove this process from the global timer task list.
   * Must be done early to avoid callbacks into a dying process.
   */

  Disable();
  ixremove(&timer_task_list, &ix_u->u_user_node);
  Enable();
  RemIntServer(INTB_VERTB, &ix_u->u_itimerint);

  /*
   * Remove the input.device SIGWINCH handler before any process
   * resources referenced by the handler can be released.  Do not do
   * this under Forbid(), since DoIO() and device cleanup may wait.
   * __ix_remove_sigwinch() is safe when no handler is installed.
   */
  __ix_remove_sigwinch();

  Forbid();

  /* Release any semaphores held by this task.
   * Must be done under Forbid() to avoid scheduler interference.
   */
  semexit(me);
  Permit();

  /*
   * Normal descriptor close paths should already have released all
   * process-private select state. Drain anything left while the ixemul trap,
   * task hooks and u_select_mp are all still valid; packet cleanup may wait.
   */
  __fselect_cleanup_process();

#ifndef NOTRAP
  /* already reset the trap vector here. It's better to get an alert than
   * to loop infinitely if one of the following functions should crash */
  me->tc_TrapCode = ix_u->u_otrap_code;
#endif
  
  /* Free per-process stack resources and shared memory regions.
   * After this point, no code should assume stack extensions exist.
   */

  freestack();
  shmexit(ix_u);

  /* had to move this block after the SYS_close's, since close() might have
     to wait for a packet, and then it's essential that our switch/launch
     handlers are still active */
  me->tc_Flags    = ix_u->u_otask_flags;
  me->tc_Launch	  = ix_u->u_olaunch;
  me->tc_Switch   = ix_u->u_oswitch;
  FreeSignal (ix_u->u_sleep_sig);
  FreeSignal (ix_u->u_pipe_sig);

  /* Close per-process libraries and devices.
   * These may have pending IO, so they must be closed before
   * ports and message structures are torn down.
   */
  
  /* Guard: Only close ixnet library if it was actually opened */
  if (ix_u->u_ixnetbase)
    CloseLibrary (ix_u->u_ixnetbase);

  if (ix_u->u_time_req)
    {
      CloseDevice ((struct IORequest *) ix_u->u_time_req);
      ix_delete_extio((struct IORequest *)ix_u->u_time_req);
    }
  
  if (ix_u->u_startup_cd != (BPTR) -1)
    {
      __unlock (CurrentDir (ix_u->u_startup_cd));
      set_dir_name_from_lock(ix_u->u_startup_cd);
    }

  ix_u->u_trace_flags = 1;

  /* Delete per-process message ports if they exist.
   * These were previously unconditionally deleted, which was unsafe.
   */

  /* Guard: Only delete message ports if they exist */
  if (ix_u->u_select_mp)
    ix_delete_port(ix_u->u_select_mp);
  if (ix_u->u_sync_mp)
    ix_delete_port(ix_u->u_sync_mp);

  /* try to free it here */
  __ix_close_muFS(ix_u);

  Forbid();

  /* Detach all child processes. Some children may already be partially
   * torn down, so safe_getuser() must be checked for NULL. The traversal
   * uses a separate 'next' pointer to avoid undefined behaviour from
   * using cu before it has been initialised.
   */

  child = ix_u->p_cptr;
  while (child)
    {
      cu = safe_getuser(child);
      if (cu)
        {
          next = cu->p_osptr;
          cu->p_pptr = (struct Process *)1;
        }
      else
        next = NULL;
      child = next;
    }

  ix_u->p_cptr = 0;

  /* decrease session count and possibly free the session structure */
  if (ix_u->u_session)
    {
      if (ix_u->u_session->pgrp == (int)me)
        ix_u->u_session->pgrp = 0;
      if (ix_u->u_session->s_count-- <= 1)
        kfree(ix_u->u_session);
    }
  
  /* Update sibling links with NULL guards */
  if (ix_u->p_ysptr)
    {
      /* Guard: Verify sibling user structure is valid before linking */
      cu = safe_getuser(ix_u->p_ysptr);
      if (cu)
        cu->p_osptr = ix_u->p_osptr;
    }

  if (ix_u->p_osptr)
    {
      /* Guard: Verify sibling user structure is valid before linking */
      cu = safe_getuser(ix_u->p_osptr);
      if (cu)
        cu->p_ysptr = ix_u->p_ysptr;
    }

  if (ix_u->p_pptr && ix_u->p_pptr != (struct Process *)1)
    {
      /* Guard: Check parent user structure is valid before dereferencing */
      u_ptr = safe_getuser(ix_u->p_pptr);
      if (u_ptr)
        {
          if (u_ptr->p_cptr == (struct Process *)me)
            u_ptr->p_cptr = ix_u->p_osptr;
        }
    }
  Permit();

  if (ix_u->p_flag & SFREEA4)
    kfree ((void *)(ix_u->u_a4 - 0x7ffe));

  /*
   * Safe zombie cleanup: remove each node using ixremove(), wake under Forbid,
   * then free outside. This preserves head/tail invariants and follows wait4().
   */
  for (;;)
    {
      Forbid();

      dm = (struct ixnode *)ix_u->p_zombies.head;
      if (dm)
        {
          /* Remove dm so head/tail are updated correctly */
          ixremove((struct ixlist *)&ix_u->p_zombies, dm);
          /* Wake while still under Forbid, matching wait4() semantics */
          ix_wakeup((u_int)dm);
        }

      Permit();

      if (!dm)
        break;

      /* Clear linkage to be safe before freeing */
      dm->next = NULL;
      dm->prev = NULL;
      kfree(dm);
    }

  FreeSignal (ix_u->p_zombie_sig);

  /* Report stack usage if enabled. This is safe here because all
   * teardown-critical structures have already been released.
   */

  if ((ix_u->p_flag & SUSAGE) == 0)
    ix_stack_usage();

   /* Free all memory allocated to this process.
   * The user structure itself is freed separately below.
   */
  all_free ();

#ifndef NOTRAP
  /* delay this until here, since the above called functions need access
   * to the user area. */
  setuser(me, ix_u->u_otrap_data);
#endif

  /* Finally free the user structure itself.
   * No further access to ix_u is valid after this point.
   */

  kfree (((char *)ix_u) - ix_u->u_a4_pointers_size * 4);
}
