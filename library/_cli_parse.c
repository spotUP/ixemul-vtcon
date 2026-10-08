/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions (C) 1995 Jeff Shepherd
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
 *
 * Revision 1.4  2026/06/06  JJ
 *
 * Added inline filename handling to CLI file initialization.
 *
 *   - Introduced f_name_inline initialization.
 *   - Use f_name_buf for short NameFromFH() results.
 *   - Heap-allocate only when filename exceeds inline buffer.
 *   - Mark default CLI names as external (FEXTNAME).
 *
 * No changes to CLI parsing semantics or standard descriptor behavior.
 *
 *  _cli_parse.c,v 1.1.1.1 1994/04/04 04:29:41 amiga Exp
 *
 *  _cli_parse.c,v
 * Revision 1.1.1.1  1994/04/04  04:29:41  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/08/09  20:41:17  amiga
 *  change to use 2.x header files by default
 *
 *  Revision 1.2  1992/07/04  19:09:27  mwild
 *  make stderr (desc 2) *really* read/write, don't just say so...
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

/*
 *	This routine is called from the _main() routine and is used to
 *	parse the arguments passed from the CLI to the program. It sets
 *	up an array of pointers to arguments in the global variables and
 *	and sets up _argc and _argv which will be passed by _main() to
 *	the main() procedure. If no arguments are ever going to be
 *	parsed, this routine may be replaced by a stub routine to reduce
 *	program size.
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <dos/var.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <glob.h>
#include "cli_args.h"

extern int __read(), __write(), __ioctl(), __fselect(), __close();

// Initialize file structure
/* RunCommand (and the Shell, which runs every command through it) puts a
 * copy of the command's argument line into its Input() handle's buffer,
 * for ReadArgs to read. A program that takes its arguments from the
 * registers, as ixemul's startup does, leaves that copy unread, and
 * dos.library counts unread buffered bytes as read ahead from the file:
 * the first Seek() of standard input (fstat, lseek) or the Flush() at
 * exit moved a file input BACK by the argument line's length. In a shell
 * reading its script from that same file (vsh -s <script), `head -n 1`
 * printed the end of its own command line ("-n 1") instead of the next
 * line. Read the copy out of the buffer, as ReadArgs would, when the
 * buffer holds exactly the argument line: a program started any other
 * way (vfork, Workbench, a handle with real read-ahead) is left alone. */
static void consume_runcommand_args(BPTR fh, long alen, const char *aptr)
{
  struct FileHandle *fhp = (struct FileHandle *)BTOCPTR(fh);
  const char *buf;
  long i;

  if (alen <= 0 || !aptr || !fhp->fh_Buf || fhp->fh_Pos < 0
      || fhp->fh_End - fhp->fh_Pos != alen)
    return;
  buf = (const char *)BTOCPTR(fhp->fh_Buf) + fhp->fh_Pos;
  if (memcmp(buf, aptr, alen))
    return;
  for (i = 0; i < alen; i++)
    FGetC(fh);
}

static void init_file(struct file *f, BPTR fh, char *defname)
{
  f->f_fh = (struct FileHandle *)BTOCPTR(fh);

  __init_std_packet(&f->f_sp);
  __init_std_packet((void *)&f->f_select_sp);
  __fstat(f);

  f->f_flags = FEXTOPEN;
  f->f_type  = DTYPE_FILE;
  f->f_read  = __read;
  f->f_write = __write;
  f->f_ioctl = __ioctl;
  f->f_close = __close;
  f->f_select= __fselect;

  /* initialize filename ownership state */
  f->f_name = 0; 
  f->f_name_inline = 0;

  if (!IsInteractive(fh))
    {
      char buf[256];

      if (NameFromFH(fh, buf, sizeof(buf)))
        {
          size_t len = strlen(buf) + 1;

          /* Prefer inline buffer when it fits */
          if (len <= sizeof(f->f_name_buf))
            {
              f->f_name = f->f_name_buf;
              f->f_name_inline = 1;
              strcpy(f->f_name, buf);
            }
          else
            {
              f->f_name = kmalloc(len);
              f->f_name_inline = 0;
              if (f->f_name)
                strcpy(f->f_name, buf);
            }
        }
    }
  if (f->f_name == NULL)
    {
      f->f_name = defname;
      f->f_name_inline = 0;   /* external name */
      f->f_flags |= FEXTNAME;   // don't free f_name
    }
}

/* We will store all arguments in this double linked list, the list
 * is always sorted according to elements, ie. order of given arguments
 * is preserved, but all elements, that get expanded will be sorted
 * alphabetically in this list. */

struct ArgList {
  struct ixlist  al_list;  /* the list - head */
  long		 al_num;   /* number of arguments in the whole list */
};

struct Argument {
  struct ixnode a_node;   /* the link in the arg-list */
  char		*a_arg;	   /* a malloc'd string, the argument */
};

/* insert a new argument into the argument vector, we have to keep the
 * vector sorted, but only element-wise, otherwise we would break the
 * order of arguments, and "copy b a" is surely not the same as "copy a b"..
 * so we don't scan the whole list, but start with element "start",
 * if set, else we start at the list head */

static void 
AddArgument (struct ArgList *ArgList,
	     struct Argument *start, struct Argument *arg, long size)
{
  register struct Argument *el;

  /* depending on "start", start scan for right position in list at
   * successor of start or at head of list  */
  for (el = (struct Argument *)
	    (start ? start->a_node.next : ArgList->al_list.head);
       el;
       el = (struct Argument *)el->a_node.next)
    if (strcmp (el->a_arg, arg->a_arg) > 0) break;
  if (el == NULL)
    el = (struct Argument *)ArgList->al_list.tail;

  ixinsert ((struct ixlist *)ArgList, (struct ixnode *)arg, (struct ixnode *)el);

  /* and bump up the argument counter once */
  ++ArgList->al_num;
}

/* if an argument contains one or more of these characters, we have to
 * call the glob() stuff, else don't bother expanding and
 * quickly append to list. Here's the meaning of all these characters, some
 * seem to be not widely used:
 * *	match any number (incl. zero) of characters
 * #?	match any number (incl. zero) of characters (for Amiga compatibility)
 * []   match any character that's contained in the set inside the brackets
 * ?	match any character (exactly one)
 * !	negate the following expression
 */

#define iswild(ch) (index ("*[!?#", ch) ? 1 : 0)

void
__ix_cli_parse(struct Process *this_proc, long alen, char *_aptr,
	   int *argc, char ***argv)
{
  usetup;
  char *arg0;
  struct CommandLineInterface *cli;
  struct ix_cli_line cl;
  char *ixvar = 0;
  long ixvarlen = 0;
  struct Argument *arg, *narg;
  char *line, **cpp, *cp;
  int do_expand, quoted;
  int arglen;
  char *aptr;
  struct ArgList ArgList;
  int expand_cmd_line = u.u_expand_cmd_line;
  struct file *fin, *fout;
  int fd;  
  BPTR fh;
  int omask;

  KPRINTF (("entered __ix_cli_parse()\n"));
  KPRINTF (("command line length = %ld\n", alen));
  KPRINTF (("command line = '%s'\n", _aptr));

  /* this stuff has been in ix_open before, but it really belongs here, since
   * I don't want it to happen by default on OpenLibrary, since it would
   * disturb any vfork() that wants to inherit files from its parent 
   */

  omask = syscall (SYS_sigsetmask, ~0);

  if (! falloc (&fin, &fd))
    {
      /*
       * NOTE: if there's an error creating one of the standard
       *       descriptors, we just go on, the descriptor in
       *       question will then not be set up, no problem ;-)
       */
      if (fd != 0)
	ix_warning("allocated stdin is not fd #0!");
		       
      if (! falloc (&fout, &fd))
	{
	  if (fd != 1)
	    ix_warning("allocated stdout is not fd #1!");

	  if ((fh = Input ()))
	    {
	      consume_runcommand_args(fh, alen, _aptr);
	      init_file(fin, fh, "<Standard Input>");
	      /* a console is a terminal: on Unix its fd 0, 1 and 2 are one
	         read/write open. tmux draws on a dup of stdin, and select()
	         dropped a descriptor without f_write from its write set, so
	         it waited for ever (UP-Term). A file or a pipe keeps its
	         one direction. */
	      if (IsInteractive (fh))
	        {
	          fin->f_flags |= FREAD|FWRITE;
	          fin->f_ttyflags = IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;
	        }
	      else
	        {
	          fin->f_flags |= FREAD;
	          fin->f_ttyflags = IXTTY_ICRNL;
	          fin->f_write = 0;
	        }
	    }
	  else
	    {
	      u.u_ofile[0] = 0;
	      fin->f_count--;
	    }
			        
	  if ((fh = Output ()))
	    {
	      init_file(fout, fh, "<Standard Output>");
	      if (IsInteractive (fh))
	        {
	          fout->f_flags |= FREAD|FWRITE;
	          fout->f_ttyflags = IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;
	        }
	      else
	        {
	          fout->f_flags |= FWRITE;
	          fout->f_ttyflags = IXTTY_OPOST | IXTTY_ONLCR;
	          fout->f_read  = 0;
	        }
	    }
	  else
	    {
	      u.u_ofile[1] = 0;
	      fout->f_count--;
	    }

	  /* deal with stderr. Seems this was a last minute addition to 
	     dos 2, it's hardly documented, there are no access functions,
	     nobody seems to know what to do with pr_CES...
	     If pr_CES is valid, then we use it, otherwise we open the
	     console. */

	  fd = -1;
	  if ((fh = this_proc->pr_CES))
	    {
	      struct file *fp;

	      if (!falloc (&fp, &fd))
	        {
	          init_file(fp, fh, "<Standard Error>");
		  fp->f_flags |= FREAD|FWRITE;
	          fp->f_ttyflags = IXTTY_OPOST | IXTTY_ONLCR;
	        }
	    }
	    /* Apparently use of CONSOLE: gave problems with
               Emacs, so we continue to use "*" instead. */

            /* Here is some more information on this from Joerg Hoehle:
             *
	     * While writing fifolib38_1 I found that console handlers are sent
	     * ACTION_FIND* packets with names of either "*" or "Console:", depending
	     * on what the user typed. Old handlers that do not recognize "CONSOLE:"
	     * will produce strange results which could explain the above problems.
             *
	     * That's the reason why
	     * 	echo foo >*	(beware of * expansion in a non-AmigaOS shell)
	     * works in an Emacs shell buffer, whereas
	     * 	echo foo >console:
	     * won't with fifolib prior to version 38.1.
             *
	     * I believe that programs opening stderr should continue to open "*" for
	     * compatibility reasons. Opening "CONSOLE:" first and "*" if it fails is
	     * _not_ a solution: for example, FIFO: (prior to 38.1) accepts the
	     * Open("CONSOLE:") call, giving a FIFO that can be neither read nor
	     * written to :-(
	     */
	  if (fd == -1)
	    fd = syscall(SYS_open, "*", 2);
	  if (fd > -1 && fd != 2)
	    {
	      syscall(SYS_dup2, fd, 2);
	      syscall(SYS_close, fd);
	    }

	} /* falloc (&fout, &fd) */
    } /* falloc (&fin, &fd) */

  aptr = alloca (alen + 1);
  memcpy(aptr, _aptr, alen + 1);

  cli = (struct CommandLineInterface *) BTOCPTR (this_proc->pr_CLI);
  arg0 = (char *) BTOCPTR (cli->cli_CommandName);

  /* init our argument list */
  ixnewlist ((struct ixlist *)&ArgList);

  /* lets start humble.. no arguments at all:-)) */
  ArgList.al_num = 0;

  /* the out-of-band argv vsh gives an ixemul program, else the argument
   * line split as dos.library's ReadItem splits it, without its * escapes
   * in quotes (cli_args.c). Out-of-band arguments are never globbed: the
   * shell that wrote them has expanded them. */
  /* the argv vsh left for the program a Shell runs for it (cli_args.c),
   * taken out of this process: never in environ, never copied to a child */
  {
    struct LocalVar *lv = FindVar ("__ixargv", LV_VAR);

    if (lv && lv->lv_Len > 0)
      {
        ixvarlen = lv->lv_Len;
        ixvar = alloca (ixvarlen + 1);
        memcpy (ixvar, lv->lv_Value, ixvarlen);
        ixvar[ixvarlen] = 0;
      }
    if (lv)
      DeleteVar ("__ixargv", GVF_LOCAL_ONLY);
  }
  __ix_cli_begin (&cl, aptr, alen, ixvar, ixvarlen);

  /* loop over all arguments, expand all */
  for (narg = arg = 0; __ix_cli_next (&cl, &line, &quoted); )
    {
      KPRINTF (("got arg '%s'\n", line));
      /* a quoted argument is never expanded */
      do_expand = 0;
      if (!quoted)
        for (cp = line; *cp; cp++)
          do_expand |= iswild (*cp);

      if (expand_cmd_line && do_expand)
	{
          glob_t g;
          char **p;
          
          syscall (SYS_sigsetmask, omask);
          syscall (SYS_glob, line,
                   ((ix.ix_flags & ix_unix_pattern_matching_case_sensitive) ? 0 : GLOB_NOCASE) |
                   ((ix.ix_flags & ix_allow_amiga_wildcard) ? GLOB_AMIGA : 0) |
	           GLOB_NOCHECK, NULL, &g);
          omask = syscall (SYS_sigsetmask, ~0);
          for (p = g.gl_pathv; *p; p++)
            {
              arg = (struct Argument *)syscall(SYS_malloc, sizeof(*arg));
              arg->a_arg = *p;
              AddArgument(&ArgList, narg, arg, strlen(*p));
              narg = (struct Argument *) ArgList.al_list.tail;
            }
          syscall(SYS_free, g.gl_pathv);
	}
      else  /* ! do_expand */
	{
	  /* just add the argument "as is" */
	  arg = (struct Argument *) syscall (SYS_malloc, sizeof (*arg));
	  arglen = strlen (line);
	  arg->a_arg = (char *) syscall (SYS_malloc, arglen + 1);
	  strcpy (arg->a_arg, line);
	  AddArgument (&ArgList, narg, arg, arglen);
	}

      narg = (struct Argument *) ArgList.al_list.tail;
    } /* for */

  /* prepend the program name */
  arg = (struct Argument *) syscall (SYS_malloc, sizeof (*arg));

  /* some stupid shells (like Wsh...) pass the WHOLE path of the
   * started program. We simply cut off what we don't want ;-)) */
  for (arglen = 1; arglen <= arg0[0]; arglen++)
    if (arg0[arglen] == ' ' || arg0[arglen] == '\t')
      break;

  line = arg0 + 1;

  arg->a_arg = (char *) syscall (SYS_malloc, arglen);

  strncpy (arg->a_arg, line, arglen - 1);
  arg->a_arg[arglen - 1] = 0;
  ixaddhead ((struct ixlist *)&ArgList, (struct ixnode *)arg);
  ++ ArgList.al_num;

  /* build _argv array */
  *argv = (char **) syscall (SYS_malloc, (ArgList.al_num+1) * sizeof(char *));
  for (cpp = *argv, arg = (struct Argument *) ArgList.al_list.head;
       arg;
       arg = (struct Argument *)arg->a_node.next)
    *cpp++ = arg->a_arg;

  /* guarantee last element == 0 */
  *cpp = 0;
  KPRINTF_ARGV ("argv", *argv);
  *argc = ArgList.al_num;
  
  if (u.u_ixnetbase)
    {
      int daemon = netcall(NET_init_inet_daemon, argc, argv);

      if (daemon >= 0)
        set_socket_stdio(daemon);
    }

  KPRINTF (("argc = %ld\n", *argc));
  KPRINTF (("leaving __ix_cli_parse()\n"));

  syscall (SYS_setsid); /* setup new session */
  syscall (SYS_sigsetmask, omask);
}
