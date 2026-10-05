/*	$NetBSD: inet.h,v 1.4 1994/10/26 00:56:44 cgd Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
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
 *
 *	@(#)inet.h	5.7 (Berkeley) 4/3/91
 */

/*
 * Revision 1.1  2026/08/01  ChatGPT modification (JJ)
 *
 *  Add IPv4 inet_ntop() and inet_pton() declarations, INET_ADDRSTRLEN,
 *  and a guarded socklen_t definition for standalone <arpa/inet.h> use.
 */

#ifndef _INET_H_
#define	_INET_H_

/* External definitions for functions in inet(3) */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/cdefs.h>

#if !defined(_IXEMUL_SOCKLEN_T_DEFINED) && !defined(_SOCKLEN_T_DECLARED)
typedef int socklen_t;
#endif
#define _IXEMUL_SOCKLEN_T_DEFINED
#define _SOCKLEN_T_DECLARED	/* the BSD guard, <sys/socket.h> tests it too */

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

__BEGIN_DECLS
unsigned long	inet_addr __P((const char *));
int		inet_aton __P((const char *, struct in_addr *));
unsigned long	inet_lnaof __P((struct in_addr));
struct in_addr	inet_makeaddr __P((u_long, u_long));
unsigned long	inet_netof __P((struct in_addr));
unsigned long	inet_network __P((const char *));
char		*inet_ntoa __P((struct in_addr));
const char	*inet_ntop __P((int, const void *, char *, socklen_t));
int		inet_pton __P((int, const char *, void *));
__END_DECLS

#endif /* !_INET_H_ */
