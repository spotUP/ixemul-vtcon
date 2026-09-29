/*	$NetBSD: cdefs.h,v 1.2 1995/03/23 20:10:33 jtc Exp $	*/

/*
 * Written by J.T. Conklin <jtc@wimsey.com> 01/17/95.
 * Public domain.
 */

#ifndef	_M68K_CDEFS_H_
#define	_M68K_CDEFS_H_

#ifdef __STDC__
#define _C_LABEL(x)	_STRING(_ ## x)
#else
#define _C_LABEL(x)	_STRING(_/**/x)
#endif

/* These are a.out stabs (N_INDR, N_WARNING). The old GG toolchain made
 * a.out objects; bebbo's gcc makes Amiga hunk objects, where the warning
 * stab left every symbol linked after it unbound (a whole ixemul link
 * failed on one __warn_references in gets.c). Hunk objects have neither,
 * so they are empty there. */
#if defined(__GNUC__) && __GNUC__ >= 3
#define __indr_reference(sym,alias)
#define __warn_references(sym,msg)
#elif defined(__GNUC__)
#ifdef __STDC__
#define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_" #alias "\",11,0,0,0");	\
	__asm__(".stabs \"_" #sym "\",1,0,0,0")
#define __warn_references(sym,msg)	\
	__asm__(".stabs \"" msg "\",30,0,0,0");		\
	__asm__(".stabs \"_" #sym "\",1,0,0,0")
#else
#define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_/**/alias\",11,0,0,0");	\
	__asm__(".stabs \"_/**/sym\",1,0,0,0")
#define __warn_references(sym,msg)	\
	__asm__(".stabs msg,30,0,0,0");			\
	__asm__(".stabs \"_/**/sym\",1,0,0,0")
#endif
#endif

#endif /* !_M68K_CDEFS_H_ */
