/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 by Jeff Shepherd
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
 *  $Id:$
 *
 *  $Log:$
 *
 * Revision 1.1.1.9  2026/09/16  ChatGPT modifications  (JJ)
 *
 *  Keep _tcp_poll() independent of per-user errno state by performing
 *  POLL mode validation in _tcp_select(), which already has user setup.
 *  Remove the duplicate socket descriptor range check from _tcp_poll();
 *  _tcp_select() validates the descriptor before dispatching POLL.
 *
 * Revision 1.1.1.8  2026/09/16  ChatGPT modifications  (JJ)
 *
 *  Removed duplicate user setup from the socket poll path.
 *  Reused the ixnet context already established by _tcp_select().
 *  Reduced _tcp_poll() from three local fd_sets to one and zero only that set.
 *
 * Revision 1.1.1.7  2026/09/16  ChatGPT modifications  (JJ)
 *
 *  Protected Exec public-port lookup/use sequences in the inetd paths.
 *  Removed the replied inetd message before deleting its reply port.
 *  Preserved AS225 release and inherit errors across cleanup/control flow.
 *  Restored the NULL terminator after removing AS225 inetd arguments.
 *  Made socket select signal-mask shifts unsigned.
 *
 * Revision 1.1.1.6  2026/08/09  ChatGPT modifications  (JJ)
 *
 *  Fixed the AmiTCP/Roadshow release_socket() ownership path.  A duplicated
 *  socket is no longer closed after a successful TCP_ReleaseSocket(), since
 *  ownership has been transferred to the public socket list.  If release
 *  fails, the duplicate is closed and the release error is preserved.
 *
 * Revision 1.1.1.5  2026/08/07  ChatGPT modifications  (JJ)
 *
 *  Added small inline send/receive backend dispatch helpers shared by the
 *  public socket entry points and the hot _tcp_write()/_tcp_read() file
 *  callbacks.  The callbacks now reuse their existing user setup instead
 *  of calling through _send()/_recv() and repeating that setup.  Backend
 *  selection, arguments, return values and EINTR handling are unchanged.
 *
 * Revision 1.1.1.4  2026/08/02  ChatGPT modifications  (JJ)
 *
 *  Reworked accept(), getsockname(), and getpeername() address handling to
 *  receive into a local IPv4 sockaddr and copy back only the caller's
 *  advertised capacity.  Invalid address/length combinations are rejected,
 *  while accept() with no requested peer address is passed to the backend as
 *  NULL, NULL.
 *
 * Revision 1.1.1.3  2026/07/22  ChatGPT modifications  (JJ)
 *
 *  Reworked recvfrom() address handling to receive into a local IPv4
 *  sockaddr and copy back only the caller's advertised capacity.  Invalid
 *  address/length combinations are rejected and a NULL address request is
 *  passed to the backend as NULL, NULL.
 *
 *  Preserved AS225 const-sockaddr protection while restoring sendto() with
 *  a NULL destination and zero length for already-connected sockets.
 *
 *  Added fd_set range validation and backend-error propagation to the TCP
 *  select callbacks.  Callback returns of -1 are handled by select.c.
 *
 *  Fixed AS225 inetd initialization to obtain SO_TYPE before installing the
 *  inherited socket, delayed daemon state publication until success, and
 *  closed inherited/obtained sockets when subsequent setup fails.
 *
 *  Limited EINTR wakeup handling to backend calls that actually failed.
 *
 * Revision 1.1.1.2  2026/06/23  ChatGPT modifications  (JJ)
 *
 *  Added conservative sockaddr overflow protection for recvfrom().
 *  If a caller supplies a source address buffer that is smaller than
 *  struct sockaddr_in, recvfrom() now fails with EINVAL instead of
 *  allowing the backend to overwrite the caller's buffer.
 *
 *  Fixed AS225 bind/connect/sendto handling to avoid modifying caller
 *  supplied const sockaddr storage.  The AS225 sa_len == 0 requirement
 *  is now applied to a local sockaddr_in copy before calling the backend.
 *
 *  Other socket operations and backend calls are otherwise unchanged.
 */

#define _KERNEL
#include "ixnet.h"

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <machine/param.h>
#include <string.h>
#include <errno.h>
#include <inetd.h>
#include <stdlib.h>
#include "select.h"
#include "ixprotos.h"

int _tcp_read	(struct file *fp, char *buf, int len);
int _tcp_write	(struct file *fp, char *buf, int len);
int _tcp_ioctl	(struct file *fp, int cmd, int inout, int arglen, caddr_t data);
int _tcp_select (struct file *fp, int select_cmd, int io_mode, fd_set *, u_long *);
int _tcp_close	(struct file *fp);

int
_socket (int domain, int type, int protocol)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    err = SOCK_socket(domain, type, protocol);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_Socket(domain, type, protocol);
	break;
    }
    return err;
}


int
_bind (struct file *fp, const struct sockaddr *name, int namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    {
		struct sockaddr_in tmp;

		if (name == NULL || namelen < 0 || namelen > (int)sizeof(tmp)) {
		    errno = EINVAL;
		    return -1;
		}

		memset(&tmp, 0, sizeof(tmp));
		memcpy(&tmp, name, (size_t)namelen);
		((struct sockaddr *)&tmp)->sa_len = 0;

		error = SOCK_bind(fp->f_so, (struct sockaddr *)&tmp, namelen);
	    }
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Bind(fp->f_so, name, namelen);
	break;
    }
    return error;
}

int
_listen (struct file *fp, int backlog)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    error = SOCK_listen(fp->f_so, backlog);
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Listen(fp->f_so, backlog);
	break;
    }
    return error;
}

int
_accept (struct file *fp, struct sockaddr *name, int *namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;

    if (name == NULL) {
	switch (network_protocol) {
	case IX_NETWORK_AS225:
	    return SOCK_accept(fp->f_so, NULL, NULL);

	case IX_NETWORK_AMITCP:
	    return TCP_Accept(fp->f_so, NULL, NULL);
	}

	return -1;
    }

    if (namelen == NULL || *namelen < 0) {
	errno = EINVAL;
	return -1;
    }

    {
	struct sockaddr_in tmp;
	int actual;
	int callerlen;
	int copylen;

	callerlen = *namelen;
	actual = sizeof(tmp);
	memset(&tmp, 0, sizeof(tmp));

	switch (network_protocol) {
	case IX_NETWORK_AS225:
	    err = SOCK_accept(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_Accept(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;
	}

	if (err >= 0) {
	    if (actual < 0)
		actual = 0;

	    copylen = callerlen;
	    if (copylen > actual)
		copylen = actual;
	    if (copylen > (int)sizeof(tmp))
		copylen = sizeof(tmp);

	    if (copylen > 0)
		memcpy(name, &tmp, (size_t)copylen);

	    *namelen = actual;
	}
    }

    return err;
}


int
_dup(struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
        case IX_NETWORK_AS225:
            /* only INET-225 has dup */
            if (((struct Library *)p->u_SockBase)->lib_Version >= 8)
        	error = SOCK_dup(fp->f_so);
            else
        	error = fp->f_so;
            break;

        case IX_NETWORK_AMITCP:
            error = TCP_Dup2Socket(fp->f_so, -1);
            break;
    }
    return error;
}

int release_socket(struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;
    int saved_errno;
    /* dup the socket first, since for AmiTCP, we can only release once */
    int s2 = _dup(fp);

    if (s2 != -1) {
        switch (network_protocol) {
            case IX_NETWORK_AS225:
        	error = (int)SOCK_release(s2);
                if (error == -1) {
                    saved_errno = errno;
                    SOCK_close(s2);
                    errno = saved_errno;
                }
                else
        	    SOCK_close(s2);
                break;

            case IX_NETWORK_AMITCP:
        	error = TCP_ReleaseSocket(s2, -1);
                if (error == -1) {
                    saved_errno = errno;
                    TCP_CloseSocket(s2);
                    errno = saved_errno;
                }
                break;
        }
    }
    return error;
}

int obtain_socket(long id, int inet, int stream, int protocol)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
        case IX_NETWORK_AS225:
            error = SOCK_inherit((void *)id);
            break;

        case IX_NETWORK_AMITCP:
            error = TCP_ObtainSocket(id, inet, stream, protocol);
            break;
    }
    return error;
}

int
_connect (struct file *fp, const struct sockaddr *name, int namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    {
		struct sockaddr_in tmp;

		if (name == NULL || namelen < 0 || namelen > (int)sizeof(tmp)) {
		    errno = EINVAL;
		    return -1;
		}

		memset(&tmp, 0, sizeof(tmp));
		memcpy(&tmp, name, (size_t)namelen);
		((struct sockaddr *)&tmp)->sa_len = 0;

		error = SOCK_connect(fp->f_so, (struct sockaddr *)&tmp, namelen);
	    }
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Connect(fp->f_so, name,namelen);
	break;
    }
    return error;
}

int
_sendto (struct file *fp, const void *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    {
		struct sockaddr_in tmp;

		if (to == NULL) {
		    if (tolen != 0) {
			errno = EINVAL;
			return -1;
		    }

		    rc = SOCK_sendto(fp->f_so, buf, len, flags, NULL, 0);
		    break;
		}

		if (tolen < 0 || tolen > (int)sizeof(tmp)) {
		    errno = EINVAL;
		    return -1;
		}

		memset(&tmp, 0, sizeof(tmp));
		memcpy(&tmp, to, (size_t)tolen);
		((struct sockaddr *)&tmp)->sa_len = 0;

		rc = SOCK_sendto(fp->f_so, buf, len, flags,
				 (struct sockaddr *)&tmp, tolen);
	    }
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_SendTo(fp->f_so,buf,len,flags,to,tolen);
	break;
    }

    return rc;
}


static __inline__ int
socket_send_dispatch(struct ixnet *p, struct file *fp,
                     const void *buf, int len, int flags)
{
    switch (p->u_networkprotocol) {

	case IX_NETWORK_AS225:
	    return SOCK_send(fp->f_so,buf,len,flags);

	case IX_NETWORK_AMITCP:
	    return TCP_Send(fp->f_so,buf,len,flags);
    }

    return -1;
}

static __inline__ int
socket_recv_dispatch(struct ixnet *p, struct file *fp,
                     void *buf, int len, int flags)
{
    switch (p->u_networkprotocol) {

	case IX_NETWORK_AS225:
	    return SOCK_recv(fp->f_so,buf,len,flags);

	case IX_NETWORK_AMITCP:
	    return TCP_Recv(fp->f_so,buf,len,flags);
    }

    return -1;
}

int
_send (struct file *fp, const void *buf, int len, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    return socket_send_dispatch(p, fp, buf, len, flags);
}


int
_sendmsg (struct file *fp, const struct msghdr *msg, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_sendmsg(fp->f_so,msg,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_SendMsg(fp->f_so,msg,flags);
	break;
    }

    return rc;
}


int
_recvfrom (struct file *fp, void *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    if (from == NULL) {
	switch (network_protocol) {
	case IX_NETWORK_AS225:
	    return SOCK_recvfrom(fp->f_so, buf, len, flags, NULL, NULL);

	case IX_NETWORK_AMITCP:
	    return TCP_RecvFrom(fp->f_so, buf, len, flags, NULL, NULL);
	}

	return -1;
    }

    if (fromlen == NULL || *fromlen < 0) {
	errno = EINVAL;
	return -1;
    }

    {
	struct sockaddr_in tmp;
	int actual;
	int callerlen;
	int copylen;

	callerlen = *fromlen;
	actual = sizeof(tmp);
	memset(&tmp, 0, sizeof(tmp));

	switch (network_protocol) {
	case IX_NETWORK_AS225:
	    rc = SOCK_recvfrom(fp->f_so, buf, len, flags,
	                       (struct sockaddr *)&tmp, &actual);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_RecvFrom(fp->f_so, buf, len, flags,
	                      (struct sockaddr *)&tmp, &actual);
	break;
	}

	if (rc >= 0) {
	    if (actual < 0)
		actual = 0;

	    copylen = callerlen;
	    if (copylen > actual)
		copylen = actual;
	    if (copylen > (int)sizeof(tmp))
		copylen = sizeof(tmp);

	    if (copylen > 0)
		memcpy(from, &tmp, (size_t)copylen);

	    *fromlen = actual;
	}
    }

    return rc;
}


int
_recv (struct file *fp, void *buf, int len, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    return socket_recv_dispatch(p, fp, buf, len, flags);
}


int
_recvmsg (struct file *fp, struct msghdr *msg, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_recvmsg(fp->f_so,msg,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_RecvMsg(fp->f_so,msg,flags);
	break;
    }

    return rc;
}

int _socketpair(int d, int type, int protocol, int sv[2])
{
    usetup;
    errno = ENOSYS;
    return -1;
}

int
_shutdown (struct file *fp, int how)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_shutdown(fp->f_so,how);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_ShutDown(fp->f_so,how);
	break;
    }
    return err;
}


int
_setsockopt (struct file *fp, int level, int name, const void *val, int valsize)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_setsockopt(fp->f_so,level,name,val, valsize);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_SetSockOpt(fp->f_so,level,name,val, valsize);
	break;
    }

    return err;
}

int
_getsockopt (struct file *fp, int level, int name, void *val, int *valsize)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getsockopt(fp->f_so,level,name,val, valsize);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetSockOpt(fp->f_so,level,name,val, valsize);
	break;
    }

    return err;
}


/*
 * Get socket name.
 */
int
_getsockname (struct file *fp, struct sockaddr *asa, int *alen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;
    struct sockaddr_in tmp;
    int actual;
    int callerlen;
    int copylen;

    if (asa == NULL || alen == NULL || *alen < 0) {
	errno = EINVAL;
	return -1;
    }

    callerlen = *alen;
    actual = sizeof(tmp);
    memset(&tmp, 0, sizeof(tmp));

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getsockname(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetSockName(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;
    }

    if (err >= 0) {
	if (actual < 0)
	    actual = 0;

	copylen = callerlen;
	if (copylen > actual)
	    copylen = actual;
	if (copylen > (int)sizeof(tmp))
	    copylen = sizeof(tmp);

	if (copylen > 0)
	    memcpy(asa, &tmp, (size_t)copylen);

	*alen = actual;
    }

    return err;
}

/*
 * Get name of peer for connected socket.
 */
int
_getpeername (struct file *fp, struct sockaddr *asa, int *alen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;
    struct sockaddr_in tmp;
    int actual;
    int callerlen;
    int copylen;

    if (asa == NULL || alen == NULL || *alen < 0) {
	errno = EINVAL;
	return -1;
    }

    callerlen = *alen;
    actual = sizeof(tmp);
    memset(&tmp, 0, sizeof(tmp));

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getpeername(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetPeerName(fp->f_so, (struct sockaddr *)&tmp, &actual);
	break;
    }

    if (err >= 0) {
	if (actual < 0)
	    actual = 0;

	copylen = callerlen;
	if (copylen > actual)
	    copylen = actual;
	if (copylen > (int)sizeof(tmp))
	    copylen = sizeof(tmp);

	if (copylen > 0)
	    memcpy(asa, &tmp, (size_t)copylen);

	*alen = actual;
    }

    return err;
}

int
_tcp_read (struct file *fp, char *buf, int len)
{
    usetup;
    int ostat, rc;
    struct user *p = &u;
    struct ixnet *net = (struct ixnet *)p->u_ixnet;

    ostat = p->p_stat;
    p->p_stat = SWAIT;

    rc = socket_recv_dispatch(net, fp, buf, len, 0);

    if (CURSIG (p))
	SetSignal (0, SIGBREAKF_CTRL_C);

    p->p_stat = ostat;

    if (rc == -1 && errno == EINTR)
	setrun (FindTask (0));

    return rc;
}


int
_tcp_write (struct file *fp, char *buf, int len)
{
    usetup;
    struct user *p = &u;
    struct ixnet *net = (struct ixnet *)p->u_ixnet;
    int ostat, rc;

    ostat = p->p_stat;
    p->p_stat = SWAIT;

    rc = socket_send_dispatch(net, fp, buf, len, 0);

    if (CURSIG (p))
	SetSignal (0, SIGBREAKF_CTRL_C);

    p->p_stat = ostat;

    if (rc == -1 && errno == EINTR)
	setrun (FindTask (0));

    return rc;
}

int
_tcp_ioctl (struct file *fp, int cmd, int inout, int arglen, caddr_t data)
{
    usetup;
    register struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int ostat, err = 0;

    ostat = usr->p_stat;
    usr->p_stat = SWAIT;

    switch (network_protocol) {

	case IX_NETWORK_AS225:

	    /* _SIGH_... they left almost everything neatly as it was in the BSD kernel
	     *	code they used, but for whatever reason they decided they needed their
	     *	own kind of ioctl encoding :-((
	     *
	     *	Well then, here we go, and map `normal' cmds into CBM cmds:
	     */

	    switch (cmd) {
		case SIOCADDRT	     : cmd = ('r'<<8)|1; break;
		case SIOCDELRT	     : cmd = ('r'<<8)|2; break;
		case SIOCSIFADDR     : cmd = ('i'<<8)|3; break;
		case SIOCGIFADDR     : cmd = ('i'<<8)|4; break;
		case SIOCSIFDSTADDR  : cmd = ('i'<<8)|5; break;
		case SIOCGIFDSTADDR  : cmd = ('i'<<8)|6; break;
		case SIOCSIFFLAGS    : cmd = ('i'<<8)|7; break;
		case SIOCGIFFLAGS    : cmd = ('i'<<8)|8; break;
		case SIOCGIFCONF     : cmd = ('i'<<8)|9; break;
		case SIOCSIFMTU      : cmd = ('i'<<8)|10; break;
		case SIOCGIFMTU      : cmd = ('i'<<8)|11; break;
		case SIOCGIFBRDADDR  : cmd = ('i'<<8)|12; break;
		case SIOCSIFBRDADDR  : cmd = ('i'<<8)|13; break;
		case SIOCGIFNETMASK  : cmd = ('i'<<8)|14; break;
		case SIOCSIFNETMASK  : cmd = ('i'<<8)|15; break;
		case SIOCGIFMETRIC   : cmd = ('i'<<8)|16; break;
		case SIOCSIFMETRIC   : cmd = ('i'<<8)|17; break;
		case SIOCSARP	     : cmd = ('i'<<8)|18; break;
		case SIOCGARP	     : cmd = ('i'<<8)|19; break;
		case SIOCDARP	     : cmd = ('i'<<8)|20; break;
		case SIOCATMARK      : cmd = ('i'<<8)|21; break;
		case FIONBIO	     : cmd = ('m'<<8)|22; break;
		case FIONREAD	     : cmd = ('m'<<8)|23; break;
		case FIOASYNC	     : cmd = ('m'<<8)|24; break;
		case SIOCSPGRP	     : cmd = ('m'<<8)|25; break;
		case SIOCGPGRP	     : cmd = ('m'<<8)|26; break;

		default:
		/* we really don't have to bother the library with cmds we can't even
		 * map over...
		 */
	    }
	    err = SOCK_ioctl(fp->f_so,cmd,data);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_IoctlSocket(fp->f_so,cmd,data);
	break;
    }
    if (CURSIG (usr))
	SetSignal (0, SIGBREAKF_CTRL_C);

    usr->p_stat = ostat;

    if (err == -1 && errno == EINTR)
	setrun (FindTask (0));

    return err;
}

/* looks like ixemul.library can't grog ixnet.library calling ix_lock_base()
 * moved most of this code back into ixemul.library
 */
int
_tcp_close (struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

#if 0
    ix_lock_base ();
    fp->f_count--;

    if (fp->f_count == 0) {
	/* don't have the base locked for IN_close, this MAY block!! */
	ix_unlock_base ();
#endif
	switch (network_protocol) {

	    case IX_NETWORK_AS225:
		err = SOCK_close (fp->f_so);
	    break;

	    case IX_NETWORK_AMITCP:
		err = TCP_CloseSocket(fp->f_so);
	    break;
	}
#if 0
    }
    else
	ix_unlock_base ();
#endif
    return err;
}

static int
_tcp_poll(struct ixnet *p, struct file *fp, int io_mode)
{
    int rc = -1;
    fd_set fds;
    fd_set *in = NULL;
    fd_set *out = NULL;
    fd_set *exc = NULL;
    struct timeval tv = {0, 0};
    register int network_protocol = p->u_networkprotocol;

    switch (io_mode) {
	case SELMODE_IN:
	    in = &fds;
	break;

	case SELMODE_OUT:
	    out = &fds;
	break;

	case SELMODE_EXC:
	    exc = &fds;
	break;
    }

    FD_ZERO(&fds);
    FD_SET(fp->f_so,&fds);

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    rc = SOCK_selectwait(fp->f_so+1,in,out,exc,&tv,NULL);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_WaitSelect(fp->f_so+1,in,out,exc,&tv,NULL);
	break;
    }

    if (rc < 0)
	return -1;

    return rc ? 1 : 0;
}

int
_tcp_select (struct file *fp, int select_cmd, int io_mode, fd_set *set, u_long *nfds)
{
  usetup;

  if (fp->f_so < 0 || fp->f_so >= FD_SETSIZE)
    {
      errno = EBADF;
      return -1;
    }

  if (select_cmd == SELCMD_PREPARE)
    {
      register struct ixnet *p = (struct ixnet *)u.u_ixnet;

      if (set == NULL || nfds == NULL)
        {
          errno = EINVAL;
          return -1;
        }

      FD_SET(fp->f_so, set);
      if (fp->f_so > *nfds)
        *nfds = fp->f_so;
      return (1UL << p->u_sigurg | 1UL << p->u_sigio);
    }
  if (select_cmd == SELCMD_CHECK)
    {
      if (set == NULL)
        {
          errno = EINVAL;
          return -1;
        }
      return FD_ISSET(fp->f_so, set);
    }
  if (select_cmd == SELCMD_POLL)
    {
      register struct ixnet *p = (struct ixnet *)u.u_ixnet;

      if (io_mode != SELMODE_IN &&
          io_mode != SELMODE_OUT &&
          io_mode != SELMODE_EXC)
        {
          errno = EINVAL;
          return -1;
        }

      return _tcp_poll(p, fp, io_mode);
    }
  return 0;
}


u_long
waitselect(long wait_sigs, fd_set *in, fd_set *out, fd_set *exc, u_long nfds)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    int rc = -1;

    switch (p->u_networkprotocol) {
	case IX_NETWORK_AS225:
	    rc = SOCK_selectwait(nfds + 1, in, out, exc, NULL, &wait_sigs);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_WaitSelect(nfds + 1, in, out, exc, NULL, &wait_sigs);
	break;
    }
    return (rc == -1 ? -1 : wait_sigs);
}

/*
 *	init_inet_daemon.c - obtain socket accepted by the inetd
 *
 *	Copyright ? 1994 AmiTCP/IP Group,
 *			 Network Solutions Development Inc.
 *			 All rights reserved.
 *	Portions Copyright ? 1995 by Jeff Shepherd
 */

/* AS225 inet daemon stuff */
struct inetmsg {
    struct Message  msg;
    ULONG   id;
};

int
init_inet_daemon(int *argc, char ***argv)
{
    usetup;
    register struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    struct file *fp;
    register int network_protocol = p->u_networkprotocol;
    int sock;

    if (network_protocol == IX_NETWORK_AS225) {
	static int init_d(int *, char ***);
	return init_d(argc,argv);
    }
    else if (network_protocol == IX_NETWORK_AMITCP) {
	struct Process *me = (struct Process *)FindTask(0);
	struct DaemonMessage *dm = (struct DaemonMessage *)me->pr_ExitData;
	int fd,ostat;
	int err;

	if (dm == NULL) {
	    /*
	    * No DaemonMessage, return error code - probably not an inet daemon
	    */
	    return -1;
	}

	/*
	 * Obtain the server socket
	 */
	sock = TCP_ObtainSocket(dm->dm_Id, dm->dm_Family, dm->dm_Type, 0);
	if (sock < 0) {
	    /*
	    * If ObtainSocket fails we need to exit with this specific exit code
	    * so that the inetd knows to clean things up
	    */
	    exit(DERR_OBTAIN);
	}

	ostat = usr->p_stat;
	usr->p_stat = SWAIT;

	do {

	    if ((err = falloc(&fp, &fd))) {
		TCP_CloseSocket(sock);
		break;
	    }

	    fp->f_so = sock;
	    _set_socket_params(fp, dm->dm_Family, dm->dm_Type, 0);
	} while (0);

	if (CURSIG (usr))
	    SetSignal (0, SIGBREAKF_CTRL_C);

	usr->p_stat = ostat;

	if (err == EINTR)
	    setrun (FindTask (0));

	errno = err;
	return err ? -1 : fd;
    }
    else
	return -1;
}

/* code loosely derived from timed.c from AS225r2 */
/* this program was called from inetd if :
 * 1> the first arg is a valid protocol(call getprotobyname)
 * 2> inetd is started - FindPort("inetd") returns non-NULL
 * NOT 3> argv[0] is the program found in inetd.conf for the program (scan inetd.conf)
 */
#include <netdb.h>
#include <stdio.h>

static int init_d(int *argc, char ***argv)
{
    usetup;
    struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    int ostat;
    int err = 1;
    int fd = -1;
    int inetd_running;

    ostat = usr->p_stat;
    usr->p_stat = SWAIT;

    /* save a little time with this comparison */
    if (*argc >= 4) {
	struct servent *serv, *serv2;
	serv = SOCK_getservbyname((*argv)[1],"tcp");
	serv2 = SOCK_getservbyname((*argv)[1],"udp");
	if (serv || serv2) {
	    Forbid();
	    inetd_running = (FindPort("inetd") != NULL);
	    Permit();
	    if (inetd_running) {
#if 0 /* I think this isn't needed, SOCK_inherit should be enough */
		char daemon[MAXPATHLEN];
		char line[1024];
		char protocol[MAXPATHLEN];
		FILE *inetdconf;
		int founddaemon = 0;
		if (inetdconf = fopen("inet:db/inetd.conf","r")) {
		    while (!feof(inetdconf)) {
			fgets(line,sizeof(line),inetdconf);
			sscanf(line,"%s %*s %*s %*s %s",protocol,daemon);
			if (!strcmp(protocol,(*argv)[1])) {
			    founddaemon = 1;
			    break;
			}
		    }
		    fclose(inetdconf);

		    if (founddaemon)  {
			char *filename = FilePart(daemon);
#endif
		if (/*!stricmp((*argv)[0],filename)*/1) {
		    struct file *fp;
		    long sock_arg;
		    int daemon_id;
		    int sock;
		    int type;
		    int optlen;
		    int i;

		    sock_arg = atol((*argv)[2]);
		    daemon_id = atoi((*argv)[3]);
		    sock = SOCK_inherit((void *)sock_arg);
		    if (sock != -1) {
			type = 0;
			optlen = sizeof(type);

if (SOCK_getsockopt(sock, SOL_SOCKET, SO_TYPE,
                    (char *)&type, &optlen) < 0) {
			    err = errno ? errno : EIO;
			    SOCK_close(sock);
			}
			else {
			    fd = 0;
			    err = falloc(&fp, &fd);
			    if (err)
				SOCK_close(sock);
			    else {
				fp->f_so = sock;
				_set_socket_params(fp, AF_INET, type, 0);

				/* get rid of the args that AS225 put in */
				for (i = 1; i < (*argc)-3; i++)
				    (*argv)[i] = (*argv)[i+3];
				(*argc) -= 3;
				(*argv)[*argc] = NULL;

				p->sock_id = daemon_id;
				p->u_daemon = 1;
			    }
			}

			if (CURSIG (usr))
			    SetSignal (0, SIGBREAKF_CTRL_C);
		    }
		    else
			err = errno ? errno : EIO;
		}
#if 0
	    }
	}
#endif
	    }
	}
    }
    usr->p_stat = ostat;
    errno = err;
    return err ? -1 : fd;
}


/* This is only needed for AS225 */
void shutdown_inet_daemon(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    struct inetmsg inet_message;
    struct MsgPort *msgport, *replyport;
    int message_sent;

    if (p->u_networkprotocol != IX_NETWORK_AS225 || !p->u_daemon)
	return;

    if ((inet_message.id = p->sock_id)) {
	replyport = CreateMsgPort();
	if (replyport) {
	    inet_message.msg.mn_Node.ln_Type = NT_MESSAGE;
	    inet_message.msg.mn_Length = sizeof(struct inetmsg);
	    inet_message.msg.mn_ReplyPort = replyport;

	    message_sent = 0;
	    Forbid();
	    msgport = FindPort("inetd");
	    if (msgport) {
		PutMsg(msgport,(struct Message *)&inet_message);
		message_sent = 1;
	    }
	    Permit();

	    if (message_sent) {
		/* we can't exit until we received a reply */
		WaitPort(replyport);
		GetMsg(replyport);
	    }
	    DeleteMsgPort(replyport);
	}
    }
}
