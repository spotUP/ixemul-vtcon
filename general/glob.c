/*	$NetBSD: glob.c,v 1.5 1995/02/27 04:13:35 cgd Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
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
 * Revision 1.1.1.1  2026/06/16  JJ/ChatGPT
 *
 * Hardened glob.c against internal fixed-buffer overflows while preserving
 * the existing initial pattern truncation behavior.
 *
 * - Added bounds checks to brace expansion, tilde expansion, pattern
 *   compilation and pathname construction paths using MAXPATHLEN-sized
 *   internal buffers.
 * - Return GLOB_NOSPACE when an internal expansion or constructed path
 *   cannot fit in the destination buffer.
 * - Kept the initial glob() pattern copy behavior unchanged.
 * - Replaced the Amiga ':' path-conversion alloca(strlen(pattern) + 2)
 *   with checked heap allocation to avoid unbounded stack use.
 * - Added cleanup of the temporary Amiga ':' conversion buffer before
 *   returning from glob().
 * - Kept normal matching rules, sorting, alternate directory functions and
 *   external glob_t ABI unchanged.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)glob.c	8.3 (Berkeley) 10/13/93";
#else
static char rcsid[] = "$NetBSD: glob.c,v 1.5 1995/02/27 04:13:35 cgd Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * glob(3) -- a superset of the one defined in POSIX 1003.2.
 *
 * The [!...] convention to negate a range is supported (SysV, Posix, ksh).
 *
 * Optional extra services, controlled by flags not defined by POSIX:
 *
 * GLOB_QUOTE:
 *	Escaping convention: \ inhibits any special meaning the following
 *	character might have (except \ at end of string is retained).
 * GLOB_MAGCHAR:
 *	Set in gl_flags if pattern contained a globbing character.
 * GLOB_NOMAGIC:
 *	Same as GLOB_NOCHECK, but it will only append pattern if it did
 *	not contain any magic characters.  [Used in csh style globbing]
 * GLOB_ALTDIRFUNC:
 *	Use alternately specified directory access functions.
 * GLOB_TILDE:
 *	expand ~user/foo to the /home/dir/of/user/foo
 * GLOB_BRACE:
 *	expand {1,2}{a,b} to 1a 1b 2a 2b 
 * gl_matchc:
 *	Number of matches in the current invocation of glob.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <glob.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#undef NOT	/* in <intuition/intuition.h> on AmigaOS */

#define	DOLLAR		'$'
#define	DOT		'.'
#define	EOS		'\0'
#define	LBRACKET	'['
#define	NOT		'!'
#define	QUESTION	'?'
#define	QUOTE		'\\'
#define	RANGE		'-'
#define	RBRACKET	']'
#define	SEP		'/'
#define	STAR		'*'
#define	TILDE		'~'
#define	UNDERSCORE	'_'
#define	LBRACE		'{'
#define	RBRACE		'}'
#define	SLASH		'/'
#define	COMMA		','
#define HASH            '#'

#define	M_QUOTE		0x8000
#define	M_PROTECT	0x4000
#define	M_MASK		0xffff
#define	M_ASCII		0x00ff

typedef u_short Char;

#define	CHAR(c)		((Char)((c)&M_ASCII))
#define	META(c)		((Char)((c)|M_QUOTE))
#define	M_ALL		META('*')
#define	M_END		META(']')
#define	M_NOT		META('!')
#define	M_ONE		META('?')
#define	M_RNG		META('-')
#define	M_SET		META('[')
#define	ismeta(c)	(((c)&M_QUOTE) != 0)


static int	 compare __P((const void *, const void *));
static int	 icompare __P((const void *, const void *));
static void	 g_Ctoc __P((const Char *, char *));
static int	 g_lstat __P((Char *, struct stat *, glob_t *));
static DIR	*g_opendir __P((Char *, glob_t *));
static Char	*g_strchr __P((Char *, int));
#ifdef notdef
static Char	*g_strcat __P((Char *, const Char *));
#endif
static int	 g_stat __P((Char *, struct stat *, glob_t *));
static int	 glob0 __P((const Char *, glob_t *));
static int	 glob1 __P((Char *, glob_t *));
static int	 glob2 __P((Char *, Char *, Char *, glob_t *));
static int	 glob3 __P((Char *, Char *, Char *, Char *, glob_t *));
static int	 globextend __P((const Char *, glob_t *));
static const Char *	 globtilde __P((const Char *, Char *, glob_t *, int *));
static int	 globexp1 __P((const Char *, glob_t *));
static int	 globexp2 __P((const Char *, const Char *, glob_t *, int *));
static int	 match __P((Char *, Char *, Char *, int));

DIR *opendir __P((const char *));
struct dirent *readdir __P((DIR *));
int closedir __P((DIR *));

int
glob(pattern, flags, errfunc, pglob)
	const char *pattern;
	int flags, (*errfunc) __P((const char *, int));
	glob_t *pglob;
{
	const u_char *patnext;
	char *glob_pattern = (char *)pattern;
	char *glob_pattern_alloc = NULL;
	size_t glob_pattern_len;
	int c;
	int err;
	Char *bufnext, *bufend, patbuf[MAXPATHLEN+1];

        if (index(pattern, ':'))
          {
            char *colon;

            glob_pattern_len = strlen(pattern);
            if (glob_pattern_len + 2 < glob_pattern_len)
              return(GLOB_NOSPACE);
            glob_pattern_alloc = malloc(glob_pattern_len + 2);
            if (glob_pattern_alloc == NULL)
              return(GLOB_NOSPACE);
            glob_pattern = glob_pattern_alloc;

            strcpy(glob_pattern + 1, pattern);
            *glob_pattern = '/';
            colon = index(glob_pattern, ':');
            *colon = '/';
          }

	patnext = (u_char *) glob_pattern;
	if (!(flags & GLOB_APPEND)) {
		pglob->gl_pathc = 0;
		pglob->gl_pathv = NULL;
		if (!(flags & GLOB_DOOFFS))
			pglob->gl_offs = 0;
	}
	pglob->gl_flags = flags & ~GLOB_MAGCHAR;
	pglob->gl_errfunc = errfunc;
	pglob->gl_matchc = 0;

	bufnext = patbuf;
	bufend = bufnext + MAXPATHLEN;
	if (flags & GLOB_QUOTE) {
		/* Protect the quoted characters. */
		while (bufnext < bufend && (c = *patnext++) != EOS) 
			if (c == QUOTE) {
				if ((c = *patnext++) == EOS) {
					c = QUOTE;
					--patnext;
				}
				*bufnext++ = c | M_PROTECT;
			}
			else
				*bufnext++ = c;
	}
	else 
	    while (bufnext < bufend && (c = *patnext++) != EOS) 
		    *bufnext++ = c;
	*bufnext = EOS;

	if (flags & GLOB_BRACE)
	    err = globexp1(patbuf, pglob);
	else
	    err = glob0(patbuf, pglob);

	if (glob_pattern_alloc != NULL)
	    free(glob_pattern_alloc);

	return err;
}

/*
 * Expand recursively a glob {} pattern. When there is no more expansion
 * invoke the standard globbing routine to glob the rest of the magic
 * characters
 */
static int globexp1(pattern, pglob)
	const Char *pattern;
	glob_t *pglob;
{
	const Char* ptr = pattern;
	int rv;

	/* Protect a single {}, for find(1), like csh */
	if (pattern[0] == LBRACE && pattern[1] == RBRACE && pattern[2] == EOS)
		return glob0(pattern, pglob);

	while ((ptr = (const Char *) g_strchr((Char *) ptr, LBRACE)) != NULL)
		if (!globexp2(ptr, pattern, pglob, &rv))
			return rv;

	return glob0(pattern, pglob);
}


/*
 * Recursive brace globbing helper. Tries to expand a single brace.
 * If it succeeds then it invokes globexp1 with the new pattern.
 * If it fails then it tries to glob the rest of the pattern and returns.
 */
static int globexp2(ptr, pattern, pglob, rv)
	const Char *ptr, *pattern;
	glob_t *pglob;
	int *rv;
{
	int     i;
	Char   c, *lm, *ls, *patend;
	const Char *pe, *pm, *pl;
	Char    patbuf[MAXPATHLEN + 1];

	patend = patbuf + MAXPATHLEN;

	/* copy part up to the brace */
	for (lm = patbuf, pm = pattern; pm != ptr; pm++) {
		if (lm >= patend) {
			*rv = GLOB_NOSPACE;
			return 0;
		}
		*lm++ = *pm;
	}
	ls = lm;

	/* Find the balanced brace */
	for (i = 0, pe = ++ptr; *pe; pe++)
		if (*pe == LBRACKET) {
			/* Ignore everything between [] */
			for (pm = pe++; *pe != RBRACKET && *pe != EOS; pe++)
				continue;
			if (*pe == EOS) {
				/* 
				 * We could not find a matching RBRACKET.
				 * Ignore and just look for RBRACE
				 */
				pe = pm;
			}
		}
		else if (*pe == LBRACE)
			i++;
		else if (*pe == RBRACE) {
			if (i == 0)
				break;
			i--;
		}

	/* Non matching braces; just glob the pattern */
	if (i != 0 || *pe == EOS) {
		*rv = glob0(patbuf, pglob);
		return 0;
	}

	for (i = 0, pl = pm = ptr; pm <= pe; pm++)
		switch (*pm) {
		case LBRACKET:
			/* Ignore everything between [] */
			for (pl = pm++; *pm != RBRACKET && *pm != EOS; pm++)
				continue;
			if (*pm == EOS) {
				/* 
				 * We could not find a matching RBRACKET.
				 * Ignore and just look for RBRACE
				 */
				pm = pl;
			}
			break;

		case LBRACE:
			i++;
			break;

		case RBRACE:
			if (i) {
			    i--;
			    break;
			}
			/* FALLTHROUGH */
		case COMMA:
			if (i && *pm == COMMA)
				break;
			else {
				/* Append the current string */
				for (lm = ls; pl < pm; pl++) {
					if (lm >= patend) {
						*rv = GLOB_NOSPACE;
						return 0;
					}
					*lm++ = *pl;
				}
				/* 
				 * Append the rest of the pattern after the
				 * closing brace
				 */
				pl = pe + 1;
				do {
					c = *pl++;
					if (c != EOS && lm >= patend) {
						*rv = GLOB_NOSPACE;
						return 0;
					}
					*lm++ = c;
				} while (c != EOS);

				/* Expand the current pattern */
				*rv = globexp1(patbuf, pglob);

				/* move after the comma, to the next string */
				pl = pm + 1;
			}
			break;

		default:
			break;
		}
	*rv = 0;
	return 0;
}



/*
 * expand tilde from the passwd file.
 */
static const Char *
globtilde(pattern, patbuf, pglob, errp)
	const Char *pattern;
	Char *patbuf;
	glob_t *pglob;
	int *errp;
{
	struct passwd *pwd;
	char *h, *hend;
	const Char *p;
	Char c, *b, *bend;

	*errp = 0;
	if (*pattern != TILDE || !(pglob->gl_flags & GLOB_TILDE))
		return pattern;

	bend = patbuf + MAXPATHLEN;
	hend = (char *)patbuf + MAXPATHLEN;

	/* Copy up to the end of the string or / */
	for (p = pattern + 1, h = (char *)patbuf; *p && *p != SLASH; p++) {
		if (h >= hend) {
			*errp = GLOB_NOSPACE;
			return NULL;
		}
		*h++ = (char)*p;
	}

	*h = EOS;

	if (((char *) patbuf)[0] == EOS) {
		/* 
		 * handle a plain ~ or ~/ by expanding $HOME 
		 * first and then trying the password file
		 */
		if ((h = getenv("HOME")) == NULL) {
			if ((pwd = getpwuid(getuid())) == NULL)
				return pattern;
			else
				h = pwd->pw_dir;
		}
	}
	else {
		/*
		 * Expand a ~user
		 */
		if ((pwd = getpwnam((char*) patbuf)) == NULL)
			return pattern;
		else
			h = pwd->pw_dir;
	}

	/* Copy the home directory */
	for (b = patbuf; *h; h++) {
		if (b >= bend) {
			*errp = GLOB_NOSPACE;
			return NULL;
		}
		*b++ = (u_char)*h;
	}
	
	/* Append the rest of the pattern */
	do {
		c = *p++;
		if (c != EOS && b >= bend) {
			*errp = GLOB_NOSPACE;
			return NULL;
		}
		*b++ = c;
	} while (c != EOS);

	return patbuf;
}
	

/*
 * The main glob() routine: compiles the pattern (optionally processing
 * quotes), calls glob1() to do the real pattern matching, and finally
 * sorts the list (unless unsorted operation is requested).  Returns 0
 * if things went well, nonzero if errors occurred.  It is not an error
 * to find no matches.
 */
static int
glob0(pattern, pglob)
	const Char *pattern;
	glob_t *pglob;
{
	const Char *qpatnext;
	int c, err, oldpathc;
	Char *bufnext, *bufend, patbuf[MAXPATHLEN+1];

	qpatnext = globtilde(pattern, patbuf, pglob, &err);
	if (qpatnext == NULL)
		return(err);
	oldpathc = pglob->gl_pathc;
	bufnext = patbuf;
	bufend = patbuf + MAXPATHLEN;

	/* Compile the pattern, preserving room for the terminating EOS. */
	while ((c = *qpatnext++) != EOS) {
		switch (c) {
		case LBRACKET:
			c = *qpatnext;
			if (c == NOT)
				++qpatnext;
			if (*qpatnext == EOS ||
			    g_strchr((Char *) qpatnext+1, RBRACKET) == NULL) {
				if (bufnext >= bufend)
					return(GLOB_NOSPACE);
				*bufnext++ = LBRACKET;
				if (c == NOT)
					--qpatnext;
				break;
			}
			if (bufnext >= bufend)
				return(GLOB_NOSPACE);
			*bufnext++ = M_SET;
			if (c == NOT) {
				if (bufnext >= bufend)
					return(GLOB_NOSPACE);
				*bufnext++ = M_NOT;
			}
			c = *qpatnext++;
			do {
				if (bufnext >= bufend)
					return(GLOB_NOSPACE);
				*bufnext++ = CHAR(c);
				if (*qpatnext == RANGE &&
				    (c = qpatnext[1]) != RBRACKET) {
					if (bufnext + 2 > bufend)
						return(GLOB_NOSPACE);
					*bufnext++ = M_RNG;
					*bufnext++ = CHAR(c);
					qpatnext += 2;
				}
			} while ((c = *qpatnext++) != RBRACKET);
			pglob->gl_flags |= GLOB_MAGCHAR;
			if (bufnext >= bufend)
				return(GLOB_NOSPACE);
			*bufnext++ = M_END;
			break;
		case QUESTION:
			pglob->gl_flags |= GLOB_MAGCHAR;
			if (bufnext >= bufend)
				return(GLOB_NOSPACE);
			*bufnext++ = M_ONE;
			break;
		case STAR:
			pglob->gl_flags |= GLOB_MAGCHAR;
			/* collapse adjacent stars to one, 
			 * to avoid exponential behavior
			 */
			if (bufnext == patbuf || bufnext[-1] != M_ALL) {
				if (bufnext >= bufend)
					return(GLOB_NOSPACE);
				*bufnext++ = M_ALL;
			}
			break;
		case HASH:
			if ((pglob->gl_flags & GLOB_AMIGA) && *qpatnext == QUESTION)
			{
			  qpatnext++;
			  pglob->gl_flags |= GLOB_MAGCHAR;
			  if (bufnext == patbuf || bufnext[-1] != M_ALL) {
			    if (bufnext >= bufend)
			      return(GLOB_NOSPACE);
			    *bufnext++ = M_ALL;
			  }
			  break;
			}
			/* fall through */
		default:
			if (bufnext >= bufend)
				return(GLOB_NOSPACE);
			*bufnext++ = CHAR(c);
			break;
		}
	}
	*bufnext = EOS;

	if ((err = glob1(patbuf, pglob)) != 0)
		return(err);

	/*
	 * If there was no match we are going to append the pattern 
	 * if GLOB_NOCHECK was specified or if GLOB_NOMAGIC was specified
	 * and the pattern did not contain any magic characters
	 * GLOB_NOMAGIC is there just for compatibility with csh.
	 */
	if (pglob->gl_pathc == oldpathc && 
	    ((pglob->gl_flags & GLOB_NOCHECK) || 
	      ((pglob->gl_flags & GLOB_NOMAGIC) &&
	       !(pglob->gl_flags & GLOB_MAGCHAR))))
		return(globextend(pattern, pglob));
	/* POSIX: no match and no GLOB_NOCHECK is GLOB_NOMATCH. It returned 0
	 * with nothing added, and mandoc 1.14.6's man took that as a found
	 * page: "Cannot allocate memory", then a hang (rig 3, 2026-10-08) */
	else if (pglob->gl_pathc == oldpathc)
		return(GLOB_NOMATCH);
	else if (!(pglob->gl_flags & GLOB_NOSORT)) 
		qsort(pglob->gl_pathv + pglob->gl_offs + oldpathc,
		    pglob->gl_pathc - oldpathc, sizeof(char *), 
                    (pglob->gl_flags & GLOB_NOCASE) ? icompare : compare);
	return(0);
}

static int
compare(p, q)
	const void *p, *q;
{
	return(strcmp(*(char **)p, *(char **)q));
}

static int
icompare(p, q)
	const void *p, *q;
{
	return(strcasecmp(*(char **)p, *(char **)q));
}

static int
glob1(pattern, pglob)
	Char *pattern;
	glob_t *pglob;
{
	Char pathbuf[MAXPATHLEN+1];

	/* A null pathname is invalid -- POSIX 1003.1 sect. 2.4. */
	if (*pattern == EOS)
		return(0);
	return(glob2(pathbuf, pathbuf, pattern, pglob));
}

/*
 * The functions glob2 and glob3 are mutually recursive; there is one level
 * of recursion for each segment in the pattern that contains one or more
 * meta characters.
 */
static int
glob2(pathbuf, pathend, pattern, pglob)
	Char *pathbuf, *pathend, *pattern;
	glob_t *pglob;
{
	struct stat sb;
	Char *p, *q, *pathlim;
	int anymeta;

	pathlim = pathbuf + MAXPATHLEN;

	/*
	 * Loop over pattern segments until end of pattern or until
	 * segment with meta character found.
	 */
	for (anymeta = 0;;) {
		if (*pattern == EOS) {		/* End of pattern? */
			if (pathend > pathlim)
				return(GLOB_NOSPACE);
			*pathend = EOS;
			if (g_lstat(pathbuf, &sb, pglob))
				return(0);
		
			if (((pglob->gl_flags & GLOB_MARK) &&
			    pathend[-1] != SEP) && (S_ISDIR(sb.st_mode)
			    || (S_ISLNK(sb.st_mode) &&
			    (g_stat(pathbuf, &sb, pglob) == 0) &&
			    S_ISDIR(sb.st_mode)))) {
				if (pathend >= pathlim)
					return(GLOB_NOSPACE);
				*pathend++ = SEP;
				*pathend = EOS;
			}
			++pglob->gl_matchc;
			return(globextend(pathbuf, pglob));
		}

		/* Find end of next segment, copy tentatively to pathend. */
		q = pathend;
		p = pattern;
		while (*p != EOS && *p != SEP) {
			if (q >= pathlim)
				return(GLOB_NOSPACE);
			if (ismeta(*p))
				anymeta = 1;
			*q++ = *p++;
		}

		if (!anymeta) {		/* No expansion, do next segment. */
			pathend = q;
			pattern = p;
			while (*pattern == SEP) {
				if (pathend >= pathlim)
					return(GLOB_NOSPACE);
				*pathend++ = *pattern++;
			}
		} else			/* Need expansion, recurse. */
			return(glob3(pathbuf, pathend, pattern, p, pglob));
	}
	/* NOTREACHED */
}

static int
glob3(pathbuf, pathend, pattern, restpattern, pglob)
	Char *pathbuf, *pathend, *pattern, *restpattern;
	glob_t *pglob;
{
        usetup;
	register struct dirent *dp;
	DIR *dirp;
	Char *pathlim;
	int c, err;
	char buf[MAXPATHLEN];

	/*
	 * The readdirfunc declaration can't be prototyped, because it is
	 * assigned, below, to two functions which are prototyped in glob.h
	 * and dirent.h as taking pointers to differently typed opaque
	 * structures.
	 */
	struct dirent *(*readdirfunc)();

	pathlim = pathbuf + MAXPATHLEN;
	if (pathend > pathlim)
		return(GLOB_NOSPACE);
	*pathend = EOS;
	errno = 0;
	    
	if ((dirp = g_opendir(pathbuf, pglob)) == NULL) {
		/* TODO: don't call for ENOENT or ENOTDIR? */
		if (pglob->gl_errfunc) {
			g_Ctoc(pathbuf, buf);
			if (pglob->gl_errfunc(buf, errno) ||
			    pglob->gl_flags & GLOB_ERR)
				return (GLOB_ABEND);
		}
		return(0);
	}

	err = 0;

	/* Search directory for matching names. */
	if (pglob->gl_flags & GLOB_ALTDIRFUNC)
		readdirfunc = pglob->gl_readdir;
	else
		readdirfunc = readdir;
	while ((dp = (*readdirfunc)(dirp))) {
		register u_char *sc;
		register Char *dc;

		/* Initial DOT must be matched literally. */
		if (dp->d_name[0] == DOT && *pattern != DOT)
			continue;
		for (sc = (u_char *) dp->d_name, dc = pathend;;) {
			c = *sc++;
			if (c != EOS && dc >= pathlim) {
				err = GLOB_NOSPACE;
				*pathend = EOS;
				break;
			}
			*dc++ = c;
			if (c == EOS)
				break;
		}
		if (err)
			break;
		if (!match(pathend, pattern, restpattern, pglob->gl_flags & GLOB_NOCASE)) {
			*pathend = EOS;
			continue;
		}
		err = glob2(pathbuf, --dc, restpattern, pglob);
		if (err)
			break;
	}

	if (pglob->gl_flags & GLOB_ALTDIRFUNC)
		(*pglob->gl_closedir)(dirp);
	else
		closedir(dirp);
	return(err);
}


/*
 * Extend the gl_pathv member of a glob_t structure to accomodate a new item,
 * add the new item, and update gl_pathc.
 *
 * This assumes the BSD realloc, which only copies the block when its size
 * crosses a power-of-two boundary; for v7 realloc, this would cause quadratic
 * behavior.
 *
 * Return 0 if new item added, error code if memory couldn't be allocated.
 *
 * Invariant of the glob_t structure:
 *	Either gl_pathc is zero and gl_pathv is NULL; or gl_pathc > 0 and
 *	gl_pathv points to (gl_offs + gl_pathc + 1) items.
 */
static int
globextend(path, pglob)
	const Char *path;
	glob_t *pglob;
{
	register char **pathv;
	register int i;
	u_int newsize;
	char *copy;
	const Char *p;

	newsize = sizeof(*pathv) * (2 + pglob->gl_pathc + pglob->gl_offs);
	pathv = pglob->gl_pathv ? 
		    realloc((char *)pglob->gl_pathv, newsize) :
		    malloc(newsize);
	if (pathv == NULL)
		return(GLOB_NOSPACE);

	if (pglob->gl_pathv == NULL && pglob->gl_offs > 0) {
		/* first time around -- clear initial gl_offs items */
		pathv += pglob->gl_offs;
		for (i = pglob->gl_offs; --i >= 0; )
			*--pathv = NULL;
	}
	pglob->gl_pathv = pathv;

	for (p = path; *p++;)
		continue;
	if ((copy = malloc(p - path)) != NULL) {
		g_Ctoc(path, copy);
		pathv[pglob->gl_offs + pglob->gl_pathc++] = copy;
	}
	pathv[pglob->gl_offs + pglob->gl_pathc] = NULL;
	return(copy == NULL ? GLOB_NOSPACE : 0);
}


/*
 * pattern matching function for filenames.  Each occurrence of the *
 * pattern causes a recursion level.
 */
static int
match(name, pat, patend, nocase)
	register Char *name, *pat, *patend;
        int nocase;
{
	int ok, negate_range;
	Char c, k;

	while (pat < patend) {
		c = *pat++;
		switch (c & M_MASK) {
		case M_ALL:
			if (pat == patend)
				return(1);
			do 
			    if (match(name, pat, patend, nocase))
				    return(1);
			while (*name++ != EOS);
			return(0);
		case M_ONE:
			if (*name++ == EOS)
				return(0);
			break;
		case M_SET:
			ok = 0;
			if ((k = *name++) == EOS)
				return(0);
			if (nocase)
			  k = tolower(k);
			if ((negate_range = ((*pat & M_MASK) == M_NOT)) != EOS)
				++pat;
			while (((c = *pat++) & M_MASK) != M_END) {
			        if (nocase)
			                c = tolower(c);
				if ((*pat & M_MASK) == M_RNG) {
					if (c <= k && k <= (nocase ? tolower(pat[1]) : pat[1]))
						ok = 1;
					pat += 2;
				} else if (c == k)
					ok = 1;
			}
			if (ok == negate_range)
				return(0);
			break;
		default:
		        if (nocase)
		        {
		          k = *name++;
		          if (tolower(k) != tolower(c))
		            return 0;
		        }
			else if (*name++ != c)
				return(0);
			break;
		}
	}
	return(*name == EOS);
}

/* Free allocated data belonging to a glob_t structure. */
void
globfree(pglob)
	glob_t *pglob;
{
	register int i;
	register char **pp;

	if (pglob->gl_pathv != NULL) {
		pp = pglob->gl_pathv + pglob->gl_offs;
		for (i = pglob->gl_pathc; i--; ++pp)
			if (*pp)
				free(*pp);
		free(pglob->gl_pathv);
	}
}

static DIR *
g_opendir(str, pglob)
	register Char *str;
	glob_t *pglob;
{
	char buf[MAXPATHLEN];

	if (!*str)
		strcpy(buf, ".");
	else
		g_Ctoc(str, buf);

	if (pglob->gl_flags & GLOB_ALTDIRFUNC)
		return((*pglob->gl_opendir)(buf));

	return(opendir(buf));
}

static int
g_lstat(fn, sb, pglob)
	register Char *fn;
	struct stat *sb;
	glob_t *pglob;
{
	char buf[MAXPATHLEN];

	g_Ctoc(fn, buf);
	if (pglob->gl_flags & GLOB_ALTDIRFUNC)
		return((*pglob->gl_lstat)(buf, sb));
	return(lstat(buf, sb));
}

static int
g_stat(fn, sb, pglob)
	register Char *fn;
	struct stat *sb;
	glob_t *pglob;
{
	char buf[MAXPATHLEN];

	g_Ctoc(fn, buf);
	if (pglob->gl_flags & GLOB_ALTDIRFUNC)
		return((*pglob->gl_stat)(buf, sb));
	return(stat(buf, sb));
}

static Char *
g_strchr(str, ch)
	Char *str;
	int ch;
{
	do {
		if (*str == ch)
			return (str);
	} while (*str++);
	return (NULL);
}

#ifdef notdef
static Char *
g_strcat(dst, src)
	Char *dst;
	const Char* src;
{
	Char *sdst = dst;

	while (*dst++)
		continue;
	--dst;
	while((*dst++ = *src++) != EOS)
	    continue;

	return (sdst);
}
#endif

static void
g_Ctoc(str, buf)
	register const Char *str;
	char *buf;
{
	register char *dc = buf;
	register char *end = buf + MAXPATHLEN - 1; /* leave room for EOS */

	/* Copy until EOS or buffer end */
	while (*str && dc < end) {
		*dc++ = (char)*str++;
	}

	/* Always terminate */
	*dc = '\0';
}
