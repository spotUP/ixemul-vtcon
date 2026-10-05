/*	$NetBSD: netdb.h,v 1.5 1995/07/03 22:59:12 jtc Exp $	*/

/*-
 * Copyright (c) 1980, 1983, 1988 Regents of the University of California.
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
 *	@(#)netdb.h	5.15 (Berkeley) 4/3/91
 */

/*
 * Revision 1.1  2026/08/01  ChatGPT modification (JJ)
 *
 *  Add EAI_OVERFLOW for getnameinfo() result-buffer overflow.
 */

#ifndef _NETDB_H_
#define _NETDB_H_

#define	_PATH_HEQUIV		"/etc/hosts.equiv"
#define	_PATH_HOSTS		"/etc/hosts"
#define _TCP_PATH_NETWORKS  	"/AmiTCP/db/networks"
#define	_PATH_NETWORKS		"/etc/networks"
#define	_PATH_PROTOCOLS		"/etc/protocols"
#define _TCP_PATH_PROTOCOLS 	"/AmiTCP/db/protocols"
#define	_PATH_SERVICES		"/etc/services"
#define _TCP_PATH_SERVICES 	"/AmiTCP/db/services"

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

/*
 * RFC2553 / POSIX getaddrinfo() and getnameinfo() support.
 *
 * This is a minimal ixemul-compatible addition to the original NetBSD
 * 1995 header.  It intentionally keeps the existing hostent/servent/
 * protoent ABI and only adds the addrinfo interface needed by the
 * getaddrinfo.c/getnameinfo.c compatibility sources.
 */
#ifndef _IXEMUL_SOCKLEN_T_DEFINED
typedef int socklen_t;
#define _IXEMUL_SOCKLEN_T_DEFINED
#endif

struct addrinfo {
	int	ai_flags;	/* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
	int	ai_family;	/* PF_xxx */
	int	ai_socktype;	/* SOCK_xxx */
	int	ai_protocol;	/* 0 or IPPROTO_xxx */
	socklen_t ai_addrlen;	/* length of ai_addr */
	char	*ai_canonname;	/* canonical name for hostname */
	struct sockaddr *ai_addr;	/* binary address */
	struct addrinfo *ai_next;	/* next structure in linked list */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */

extern int h_errno;

#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */

/*
 * Error return codes from getaddrinfo().
 */
#define	EAI_ADDRFAMILY	 1	/* address family for hostname not supported */
#define	EAI_AGAIN	 2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	 3	/* invalid value for ai_flags */
#define	EAI_FAIL	 4	/* non-recoverable failure in name resolution */
#define	EAI_FAMILY	 5	/* ai_family not supported */
#define	EAI_MEMORY	 6	/* memory allocation failure */
#define	EAI_NODATA	 7	/* no address associated with hostname */
#define	EAI_NONAME	 8	/* hostname nor servname provided, or not known */
#define	EAI_SERVICE	 9	/* servname not supported for ai_socktype */
#define	EAI_SOCKTYPE	10	/* ai_socktype not supported */
#define	EAI_SYSTEM	11	/* system error returned in errno */
#define	EAI_BADHINTS	12	/* invalid value for hints */
#define	EAI_PROTOCOL	13	/* resolved protocol is unknown */
#define	EAI_OVERFLOW	14	/* argument buffer overflow */
#define	EAI_MAX		14

/*
 * Flag values for getaddrinfo().
 */
#define	AI_PASSIVE	0x00000001	/* get address to use bind() */
#define	AI_CANONNAME	0x00000002	/* fill ai_canonname */
#define	AI_NUMERICHOST	0x00000004	/* prevent name resolution */
#define	AI_NUMERICSERV	0x00000008	/* prevent service name resolution */
#define	AI_MASK		(AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | \
				 AI_NUMERICSERV)

/*
 * Constants and flag values for getnameinfo().
 */
#define	NI_MAXHOST	1025
#define	NI_MAXSERV	32

#define	NI_NOFQDN	0x00000001
#define	NI_NUMERICHOST	0x00000002
#define	NI_NAMEREQD	0x00000004
#define	NI_NUMERICSERV	0x00000008
#define	NI_DGRAM	0x00000010
#define	NI_WITHSCOPEID	0x00000020

#define	SCOPE_DELIMITER '%'

#include <sys/cdefs.h>

__BEGIN_DECLS
void		endhostent __P((void));
void		endnetent __P((void));
void		endprotoent __P((void));
void		endservent __P((void));
struct hostent	*gethostbyaddr __P((const char *, int, int));
struct hostent	*gethostbyname __P((const char *));
struct hostent	*gethostent __P((void));
struct netent	*getnetbyaddr __P((long, int)); /* u_long? */
struct netent	*getnetbyname __P((const char *));
struct netent	*getnetent __P((void));
struct protoent	*getprotobyname __P((const char *));
struct protoent	*getprotobynumber __P((int));
struct protoent	*getprotoent __P((void));
struct servent	*getservbyname __P((const char *, const char *));
struct servent	*getservbyport __P((int, const char *));
struct servent	*getservent __P((void));
void		herror __P((const char *));
char		*hstrerror __P((int));
void		sethostent __P((int));
/* void		sethostfile __P((const char *)); */
void		setnetent __P((int));
void		setprotoent __P((int));
int		getaddrinfo __P((const char *, const char *,
				 const struct addrinfo *, struct addrinfo **));
int		getnameinfo __P((const struct sockaddr *, socklen_t, char *,
				 socklen_t, char *, socklen_t, int));
void		freeaddrinfo __P((struct addrinfo *));
char		*gai_strerror __P((int));
void		setservent __P((int));
__END_DECLS

#endif /* !_NETDB_H_ */
