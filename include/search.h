#ifndef _SEARCH_H
#define _SEARCH_H

/* search.h -- declarations for POSIX/SVID-compatible search functions */

/* HSEARCH(3C) */
typedef struct entry { char *key, *data; } ENTRY;
typedef enum { FIND, ENTER } ACTION;

/* TSEARCH(3C) */
typedef enum { preorder, postorder, endorder, leaf } VISIT;

/* ixemul.library has no vectors for these: libixcompat carries newlib's
 * (compat/Makefile, SEARCH_FUNCS). ncurses 6.6 needs them for its colour
 * pairs (new_pair.c), measured on less 710. */
void *tsearch(const void *, void **, int (*)(const void *, const void *));
void *tfind(const void *, void *const *, int (*)(const void *, const void *));
void *tdelete(const void *, void **, int (*)(const void *, const void *));
void twalk(const void *, void (*)(const void *, VISIT, int));

#endif /* _SEARCH_H */
