/*	$NetBSD: scandir.c,v 1.6 1995/02/25 08:51:42 cgd Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Revision 1.8  2026/06/18  ChatGPT/Deepseek  (JJ)
 * - Harden scandir() error handling.
 * - Close the directory stream if the initial namelist allocation fails.
 * - Free all previously duplicated dirent entries and the namelist array
 * 	 if entry allocation fails while scanning.
 * - Use a temporary pointer for realloc() so the original namelist is not
 * 	 lost on allocation failure.
 * - Clean up the current dirent copy, previous entries, namelist array,
 * 	 and directory stream on realloc() failure.
 * - Use size_t for the namelist capacity to match nitems and allocation sizes.
 * - Check capacity growth and realloc() size multiplication for overflow.
 * - Use sizeof *names for namelist allocation size calculations.
 *
 *  Revision 1.7  2026/06/07  Copilot modifications (JJ)
 *  - Copy d_type from readdir() result when duplicating struct dirent.
 *    scandir() now preserves file type information (DT_*).
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)scandir.c	8.3 (Berkeley) 1/2/94";
#else
static char rcsid[] = "$NetBSD: scandir.c,v 1.6 1995/02/25 08:51:42 cgd Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct dirent (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

/*
 * The DIRSIZ macro is the minimum record length which will hold the directory
 * entry.  This requires the amount of space in struct dirent without the
 * d_name field, plus enough space for the name and a terminating nul byte
 * (dp->d_namlen + 1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp)							\
	((sizeof(struct dirent) - sizeof(dp)->d_name) +			\
	    (((dp)->d_namlen + 1 + 3) &~ 3))

int
scandir(dirname, namelist, select, dcomp)
	const char *dirname;
	struct dirent ***namelist;
	int (*select) __P((struct dirent *));
	int (*dcomp) __P((const void *, const void *));
{
	register struct dirent *d, *p, **names;
	register size_t nitems;
	size_t arraysz;
	size_t newarraysz;
	struct dirent **newnames;
	DIR *dirp;
	size_t i;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = 50;
	names = (struct dirent **)malloc(arraysz * sizeof *names);
	if (names == NULL) {
		closedir(dirp);
		return(-1);
	}

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc(DIRSIZ(d));
		if (p == NULL) {
			closedir(dirp);
			for (i = 0; i < nitems; ++i)
				free(names[i]);
			free(names);
			return(-1);
		}
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		p->d_namlen = d->d_namlen;
		p->d_type = d->d_type;
		bcopy(d->d_name, p->d_name, p->d_namlen + 1);
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (arraysz > ((size_t)-1) - 50) {
				closedir(dirp);
				free(p);
				for (i = 0; i < nitems - 1; ++i)
					free(names[i]);
				free(names);
				return(-1);
			}
			newarraysz = arraysz + 50;
			if (newarraysz > ((size_t)-1) / sizeof *names) {
				closedir(dirp);
				free(p);
				for (i = 0; i < nitems - 1; ++i)
					free(names[i]);
				free(names);
				return(-1);
			}
			newnames = (struct dirent **)realloc((char *)names,
				newarraysz * sizeof *names);
			if (newnames == NULL) {
				closedir(dirp);
				free(p);
				for (i = 0; i < nitems - 1; ++i)
					free(names[i]);
				free(names);
				return(-1);
			}
			names = newnames;
			arraysz = newarraysz;
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), dcomp);
	*namelist = names;
	return(nitems);
}

/*
 * Alphabetic order comparison routine for those who want it.
 */
int
alphasort(d1, d2)
	const void *d1;
	const void *d2;
{
	return(strcmp((*(struct dirent **)d1)->d_name,
	    (*(struct dirent **)d2)->d_name));
}
