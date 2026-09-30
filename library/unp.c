/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996 by Hans Verkuil
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
   Missing features:

   datagram support
   setsockopt/getsockopt are dummy functions
   no timeout while connecting

*/

#define _KERNEL
#include "ixemul.h"
#include "unp.h"
#include "kprintf.h"

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unix_socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <machine/param.h>
#include <string.h>
#include <sys/stat.h>
#include "select.h"

#define can_read(ss)  (ss->reader != ss->writer)
#define can_write(ss) (!(ss->reader == ss->writer + 1 \
	                 || (ss->reader == ss->buffer \
	                     && ss->writer == ss->buffer + UNIX_SOCKET_SIZE - 1)))

struct ix_unix_name *
find_unix_name(const char *path)
{
  struct ix_unix_name *un;

  for (un = ix.ix_unix_names; un; un = un->next)
    if (!strcmp(un->path, path))
      break;
  return un;
}

struct sock_stream *
init_stream(void)
{
  struct sock_stream *s = kmalloc(sizeof(struct sock_stream));

  if (s)
    {
      s->reader = s->writer = s->buffer;
      s->flags = 0;
      s->task = 0;
      s->wtask = 0;
      s->nrights = 0;
      s->written = s->readn = 0;
    }
  return s;
}

static struct ix_unix_name *
add_unix_name(const char *path, int queue_size)
{
  struct ix_unix_name *un = kmalloc(sizeof(struct ix_unix_name));
  usetup;

  if (un == NULL)
    errno_return(ENOMEM, NULL);

  if (queue_size == 0)
    queue_size = 1;
  un->queue = kmalloc(queue_size * 4);
  if (un->queue == NULL)
    {
      kfree(un);
      errno_return(ENOMEM, NULL);
    }
  strcpy(un->path, path);
  un->next = ix.ix_unix_names;
  ix.ix_unix_names = un;
  un->queue_size = queue_size;
  un->queue_index = 0;
  un->task = 0;
  return un;
}

struct sock_stream *
find_stream(struct file *f, int read_stream)
{
  struct unix_socket *us;

  if (f->f_type == DTYPE_PIPE)
    return f->f_ss;

  us = f->f_sock;
  if (us->server != f)
    read_stream = !read_stream;
  return (read_stream ? us->to_server : us->from_server);
}

struct sock_stream *
get_stream(struct file *f, int read_stream)
{
  struct sock_stream *ss = find_stream(f, read_stream);

  if (ss == NULL)
    return ss;

  Forbid();
  for (;;)
    {
      if (!(ss->flags & UNF_LOCKED))
        {
          ss->flags &= ~UNF_WANT_LOCK;
          ss->flags |= UNF_LOCKED;
          /* got it ! */
          break;
	}
      ss->flags |= UNF_WANT_LOCK;
      if (ix_sleep((caddr_t)&ss->flags, "get_sock") < 0)
        {
	  Permit();
	  setrun(FindTask(0));
	  Forbid();
        }
      /* have to always recheck whether we really got the lock */
    }
  Permit();
  return ss;
}

void
release_stream(struct sock_stream *ss)
{
  if (ss)
    {
      Forbid ();
      if (ss->flags & UNF_WANT_LOCK)
        ix_wakeup ((u_int)&ss->flags);
        
      ss->flags &= ~(UNF_WANT_LOCK | UNF_LOCKED);
      Permit ();
    }
}

static int stream_is_closed(struct sock_stream *ss)
{
  return (ss == NULL || (ss->flags & (UNF_NO_READER | UNF_NO_WRITER)) ==
                                     (UNF_NO_READER | UNF_NO_WRITER));
}

static void close_stream(struct file *f, int read_stream, int from_close)
{
  struct sock_stream **ss;
  struct unix_socket *us = f->f_sock;
  int to_server;
  usetup;

  if (us == NULL)
    return;
  to_server = (us->server != f) ? !read_stream : read_stream;
  ss = (to_server ? &us->to_server : &us->from_server);

  if (*ss)
    {
      (*ss)->flags |= (read_stream ? UNF_NO_READER : UNF_NO_WRITER);
      if (stream_is_closed(*ss))
        {
          /* descriptors sent and never received: their reference goes */
          while ((*ss)->nrights > 0)
            {
              struct file *r = (*ss)->rights[--(*ss)->nrights];
              if (r->f_close)
                (*r->f_close)(r);
            }
          kfree(*ss);
          *ss = NULL;
        }
      else
        {
          if ((*ss)->task)
            Signal((*ss)->task, 1UL << getuser((*ss)->task)->u_pipe_sig);
          if ((*ss)->wtask)
            Signal((*ss)->wtask, 1UL << getuser((*ss)->wtask)->u_pipe_sig);
          ix_wakeup ((u_int)*ss);
        }
    }
  if (read_stream && us->unix_name && f->f_count == 0)
    {
      struct ix_unix_name *un;
    
      if (us->unix_name == ix.ix_unix_names)
        {
          ix.ix_unix_names = ix.ix_unix_names->next;
        }
      else
        {
          for (un = ix.ix_unix_names; un; un = un->next)
            if (un->next == us->unix_name)
              {
                un->next = us->unix_name->next;
                break;
              }
        }
      kfree(us->unix_name->queue);
      kfree(us->unix_name);
      us->unix_name = NULL;
    }
  if (stream_is_closed(us->to_server) && stream_is_closed(us->from_server) &&
      us->unix_name == NULL && f->f_count == 0 && from_close)
    {
      f->f_sock = 0;
      kfree(us);
    }
}

int unp_socket(int domain, int type, int protocol, struct unix_socket *sock)
{
  int omask, err, fd;
  struct file *fp;
  struct unix_socket *us;
  usetup;

  if (type != SOCK_STREAM || protocol != 0)
    errno_return(EPROTONOSUPPORT, -1);

  if (sock)
    us = sock;
  else if ((us = kmalloc(sizeof(struct unix_socket))) == NULL)
    errno_return(ENOMEM, -1);
  else
    {
      us->path[0] = 0;
      us->from_server = us->to_server = NULL;
      us->unix_name = NULL;
      us->server = NULL;
      us->state = UNS_WAITING;
    }

  omask = syscall (SYS_sigsetmask, ~0);

  if ((err = falloc (&fp, &fd)))
    {
      errno = err;
      syscall (SYS_sigsetmask, omask);
      if (sock == NULL)
        kfree(us);
      return -1;
    }

  fp->f_sock = us;
  _set_socket_params(fp, domain, 0, 0);

  syscall (SYS_sigsetmask, omask);

  return fd;
}

int unp_bind(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *fp = u.u_ofile[s];
  struct ix_unix_name *un;
  int tmp;
  char *path = ((struct sockaddr_un *)name)->sun_path;

  if (fp->f_sock->path[0])
    errno_return(EINVAL, -1);

  ix_lock_base();
  un = find_unix_name(path);
  ix_unlock_base();
  if (un)  
    errno_return(EADDRINUSE, -1);

  tmp = syscall(SYS_creat, path, 0777);
  if (tmp < 0)
    return -1;
  syscall(SYS_close, tmp);
  strcpy(fp->f_sock->path, path);
  return 0;
}

int unp_listen(int s, int backlog)
{
  usetup;
  struct file *fp = u.u_ofile[s];
  struct unix_socket *us = fp->f_sock;

  if (!us->path[0] || us->unix_name || us->to_server || us->from_server)
    errno_return(EOPNOTSUPP, -1);
  
  ix_lock_base();
  us->unix_name = add_unix_name(us->path, backlog);
  ix_unlock_base();
  return (us->unix_name ? 0 : -1);
}

int unp_accept(int s, struct sockaddr *name, int *namelen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct unix_socket *client = NULL;
  struct ix_unix_name *un = f->f_sock->unix_name;
  int omask, err = 0, sleep_rc, fd;
  struct sockaddr_un *sa = (struct sockaddr_un *)name;

  if (un == NULL)
    errno_return(EOPNOTSUPP, -1);
  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);

  do {
    while (un->queue_index == 0)
      {
        if (f->f_flags & FNDELAY)
          {
            err = EWOULDBLOCK;
            goto error;
          }
        Forbid ();
        __release_file (f);
        syscall (SYS_sigsetmask, omask);
        sleep_rc = ix_sleep((caddr_t)un, "accept");
        Permit ();
        if (sleep_rc < 0)
          setrun (FindTask (0));
        omask = syscall (SYS_sigsetmask, ~0);
        __get_file (f);
      }
    ix_lock_base();
    client = NULL;
    if (un->queue_index)
      {
        client = (struct unix_socket *)un->queue[0];
        if (--un->queue_index)
          memcpy(un->queue, un->queue + 1, un->queue_index * 4);
        if (client)
          client->state = UNS_PROCESSING;
      }
    ix_unlock_base();
  } while (client == NULL);
    
error:
  __release_file (f);
  if (err)
    {
      syscall (SYS_sigsetmask, omask);
      errno_return(err, -1);
    }
  fd = unp_socket(PF_UNIX, SOCK_STREAM, 0, client);
  if (fd == -1)
    client->state = UNS_ERROR;
  else
    {
      f = u.u_ofile[fd];
      client->server = f;
      client->state = UNS_ACCEPTED;
      /* the peer's address only when asked for: accept(s, 0, 0) is legal,
         and writing through the NULL name put the path over exec's low
         memory (SysBase at 4) -- the machine froze (vtcon P7) */
      if (sa && namelen)
        {
          sa->sun_family = AF_UNIX;
          strcpy(sa->sun_path, un->path);
          sa->sun_len = *namelen = 3 + strlen(sa->sun_path);
        }
    }
  ix_wakeup((u_int)client);
  syscall (SYS_sigsetmask, omask);
  return fd;
}

/*
 * socketpair(AF_UNIX, SOCK_STREAM): two connected sockets at once, as
 * accept() makes them -- one unix_socket with its two streams, the first
 * descriptor the connecting end, the second the accepted one (us->server).
 * No name and no listener: connect() sleeps until an accept, so the pair
 * cannot be made that way in one process. (vtcon: libevent's signal pipe,
 * tmux's client and server)
 */
int unp_socketpair(int domain, int type, int protocol, int sv[2])
{
  usetup;
  struct unix_socket *us;
  int fd0, fd1;

  fd0 = unp_socket(domain, type, protocol, NULL);
  if (fd0 == -1)
    return -1;
  us = u.u_ofile[fd0]->f_sock;
  ix_lock_base();
  us->to_server = init_stream();
  if (us->to_server)
    us->from_server = init_stream();
  ix_unlock_base();
  if (!us->from_server)
    {
      syscall (SYS_close, fd0);
      errno_return(ENOMEM, -1);
    }
  strcpy(us->path, "(socketpair)");  /* marks both ends connected */
  fd1 = unp_socket(domain, type, protocol, us);
  if (fd1 == -1)
    {
      syscall (SYS_close, fd0);
      return -1;
    }
  us->server = u.u_ofile[fd1];
  us->state = UNS_ACCEPTED;
  sv[0] = fd0;
  sv[1] = fd1;
  return 0;
}

int unp_connect(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct unix_socket *us = f->f_sock;
  struct ix_unix_name *un;
  int sleep_rc, state;
  char *path = ((struct sockaddr_un *)name)->sun_path;

  if (us->unix_name)
    errno_return(EOPNOTSUPP, -1);
  if (us->to_server || us->from_server)
    errno_return(EISCONN, -1);
  /* the path marks a socket as connected or bound: only once it is
     (a failed connect left it set, and a bind of the same socket then
     failed with EINVAL -- GNU screen probes with connect, then binds) */
  ix_lock_base();
  un = find_unix_name(path);
  if (un == NULL)
    {
      struct stat st;

      ix_unlock_base();
      /* as POSIX says: no file there is ENOENT, a file (a socket left
         behind, its server gone) with nobody listening ECONNREFUSED --
         tmux starts its server on either, and on nothing else (UP-Term) */
      if (syscall(SYS_stat, path, &st) == 0)
        errno_return(ECONNREFUSED, -1);
      errno_return(ENOENT, -1);
    }
  if (un->queue_size == un->queue_index)
    {
      ix_unlock_base();
      errno_return(ECONNREFUSED, -1);
    }
  strcpy(us->path, path);
  us->to_server = init_stream();
  if (us->to_server)
    us->from_server = init_stream();
  if (!us->from_server)
    {
      if (us->to_server)
        kfree(us->to_server);
      us->to_server = NULL;
      us->path[0] = 0;
      ix_unlock_base();
      errno_return(ENOMEM, -1);
    }
  un->queue[un->queue_index++] = (int)us;
  ix_unlock_base();
  Forbid();
  if (un->task)
    Signal(un->task, 1 << getuser(un->task)->u_pipe_sig);
  ix_wakeup((u_int)un);

  state = us->state;
  if (state != UNS_ACCEPTED && state != UNS_ERROR)
    do {
      sleep_rc = ix_sleep((caddr_t)us, "connect");
      state = us->state;
      Permit ();
      if (sleep_rc < 0)
        setrun (FindTask (0));
      Forbid ();
    } while (sleep_rc < 0 || (state != UNS_ERROR && state != UNS_ACCEPTED));
  Permit();

  if (state == UNS_ERROR)
    {
      kfree(us->to_server);
      kfree(us->from_server);
      us->to_server = us->from_server = NULL;
      us->path[0] = 0;
      errno_return(ECONNREFUSED, -1);
    }
  return 0;
}

/*
 * sendmsg/recvmsg on a local socket: the data of the iovecs through the
 * stream, and SCM_RIGHTS -- descriptors passed to the other process (GNU
 * screen hands its backend the attaching terminal this way, tmux too).
 * An ixemul file is a shared struct file: passing one is a reference
 * queued on the stream, and a new descriptor for it in the receiver.
 * Network sockets (DTYPE_SOCKET) cannot be passed. (vtcon P7.1)
 */
int unp_sendmsg(int s, const struct msghdr *msg, int flags)
{
  usetup;
  struct file *f = u.u_ofile[s], *fr[UNIX_SOCKET_RIGHTS];
  int nr = 0, i, total = 0, n;
  struct cmsghdr *cm;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  /* the descriptors first: they must be there when the data is */
  if (msg->msg_control && msg->msg_controllen >= sizeof(struct cmsghdr))
    for (cm = CMSG_FIRSTHDR(msg); cm; cm = CMSG_NXTHDR((struct msghdr *)msg, cm))
      {
        int *fds = (int *)CMSG_DATA(cm);
        int k = (cm->cmsg_len - CMSG_LEN(0)) / sizeof(int);

        if (cm->cmsg_level != SOL_SOCKET || cm->cmsg_type != SCM_RIGHTS)
          continue;
        for (i = 0; i < k; i++)
          {
            struct file *p = (fds[i] >= 0 && fds[i] < NOFILE) ? u.u_ofile[fds[i]] : NULL;

            if (p == NULL || p->f_type == DTYPE_SOCKET || nr == UNIX_SOCKET_RIGHTS)
              errno_return(p == NULL ? EBADF : EINVAL, -1);
            fr[nr++] = p;
          }
      }
  if (nr)
    {
      struct sock_stream *ss = get_stream(f, FALSE);

      if (ss == NULL || (ss->flags & UNF_NO_READER))
        {
          release_stream(ss);
          errno_return(EPIPE, -1);
        }
      if (ss->nrights + nr > UNIX_SOCKET_RIGHTS)
        {
          release_stream(ss);
          errno_return(ETOOMANYREFS, -1);
        }
      ix_lock_base();
      for (i = 0; i < nr; i++)
        {
          fr[i]->f_count++;
          ss->right_at[ss->nrights] = ss->written;  /* its message starts here */
          ss->rights[ss->nrights++] = fr[i];
        }
      ix_unlock_base();
      release_stream(ss);
    }
  for (i = 0; i < msg->msg_iovlen; i++)
    {
      if (msg->msg_iov[i].iov_len == 0)
        continue;
      n = stream_write(f, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
      if (n < 0)
        return total ? total : -1;
      total += n;
    }
  return total;
}

int unp_recvmsg(int s, struct msghdr *msg, int flags)
{
  usetup;
  struct file *f = u.u_ofile[s];
  int i, n = 0, total = 0, limit = -1, deliver, fit, k = 0;
  struct sock_stream *ss;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  /* the data (one read, as a stream socket gives what is there), but not
     past the start of the next passed descriptor's message: that message
     comes with its descriptor in a later recvmsg */
  ss = get_stream(f, TRUE);
  if (ss)
    for (i = 0; i < ss->nrights; i++)
      if (ss->right_at[i] > ss->readn)
        {
          limit = ss->right_at[i] - ss->readn;
          break;
        }
  release_stream(ss);
  for (i = 0; i < msg->msg_iovlen; i++)
    if (msg->msg_iov[i].iov_len)
      {
        int len = msg->msg_iov[i].iov_len;

        if (limit >= 0 && len > limit)
          len = limit;
        n = stream_read(f, msg->msg_iov[i].iov_base, len);
        if (n < 0)
          return -1;
        total = n;
        break;
      }
  msg->msg_flags = 0;
  /* the descriptors whose message this read began (at EOF: all of them) */
  ss = get_stream(f, TRUE);
  deliver = 0;
  if (ss)
    while (deliver < ss->nrights &&
           (ss->right_at[deliver] < ss->readn || (n == 0 && ss->right_at[deliver] <= ss->readn)))
      deliver++;
  fit = 0;
  if (msg->msg_control && msg->msg_controllen >= CMSG_LEN(sizeof(int)))
    fit = (msg->msg_controllen - CMSG_LEN(0)) / sizeof(int);
  if (deliver)
    {
      struct cmsghdr *cm = CMSG_FIRSTHDR(msg);
      int *fds = fit ? (int *)CMSG_DATA(cm) : NULL;

      while (k < deliver && k < fit)
        {
          int fd;

          if (ufalloc(0, &fd))
            break;
          u.u_ofile[fd] = ss->rights[k];  /* its reference comes with it */
          u.u_pofile[fd] = 0;
          if (fd > u.u_lastfile)
            u.u_lastfile = fd;
          fds[k++] = fd;
        }
      /* what did not fit is closed, as BSD does (MSG_CTRUNC) */
      for (i = k; i < deliver; i++)
        if (ss->rights[i]->f_close)
          (*ss->rights[i]->f_close)(ss->rights[i]);
      if (k < deliver)
        msg->msg_flags |= MSG_CTRUNC;
      for (i = deliver; i < ss->nrights; i++)
        {
          ss->rights[i - deliver] = ss->rights[i];
          ss->right_at[i - deliver] = ss->right_at[i];
        }
      ss->nrights -= deliver;
    }
  if (k)
    {
      struct cmsghdr *cm = CMSG_FIRSTHDR(msg);

      cm->cmsg_level = SOL_SOCKET;
      cm->cmsg_type = SCM_RIGHTS;
      cm->cmsg_len = CMSG_LEN(k * sizeof(int));
      msg->msg_controllen = cm->cmsg_len;
    }
  else
    msg->msg_controllen = 0;
  release_stream(ss);
  return total;
}

int unp_send(int s, const void *buf, int len, int flags)
{
  usetup;
  if (flags)
    errno_return(EOPNOTSUPP, -1);
  return unp_write(u.u_ofile[s], buf, len);
}

int unp_recv(int s, void *buf, int len, int flags)
{
  usetup;
  if (flags)
    errno_return(EOPNOTSUPP, -1);
  return unp_read(u.u_ofile[s], buf, len);
}

int unp_shutdown(int s, int how)
{
  usetup;
  struct file *f = u.u_ofile[s];

  ix_lock_base();
  switch (how)
  {
    case 0:
      close_stream(f, TRUE, FALSE);
      break;
    case 1:
      close_stream(f, FALSE, FALSE);
      break;
    case 2:
      close_stream(f, TRUE, FALSE);
      close_stream(f, FALSE, FALSE);
      break;
  }
  ix_unlock_base();
  return 0;
}

int unp_setsockopt(int s, int level, int name, const void *val, int valsize)
{
  return 0;
}

int unp_getsockopt(int s, int level, int name, void *val, int *valsize)
{
  *valsize = 0;
  return 0;
}

int unp_getsockname(int s, struct sockaddr *asa, int *alen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct sockaddr_un *un = (struct sockaddr_un *)asa;

  strcpy(un->sun_path, f->f_sock->path);
  un->sun_family = AF_UNIX;
  un->sun_len = *alen = 5 + strlen(un->sun_path);
  return 0;
}

int unp_getpeername(int s, struct sockaddr *asa, int *alen)
{
  return unp_getsockname(s, asa, alen);
}

int
stream_read (struct file *f, char *buf, int len)
{
  usetup;
  int omask = syscall (SYS_sigsetmask, ~0);
  int err = errno;
  int really_read = 0;
  int sleep_rc;
  struct sock_stream *ss = get_stream(f, TRUE);

  while (len)
    {
      if (ss == NULL || !can_read(ss))
	{
	  if (really_read || ss == NULL || (ss->flags & UNF_NO_WRITER))
	    {
	      err = 0;
	      break;
	    }
	
	  if (f->f_flags & FNDELAY)
	    {
	      if (!really_read)
		{
		  really_read = -1;
		  err = EAGAIN;
		}
	      break;
	    }

	  /* wait for something to be read or all readers to close */
	  Forbid ();
	  /* sigh.. Forbid() is necessary, or the other end may change
	     the pipe, and in the worst case also settle for sleep(), and
	     there it is.. deadlock.. */
	  release_stream(ss);

	  /* make read interruptible */
	  syscall (SYS_sigsetmask, omask);
	  sleep_rc = ix_sleep ((caddr_t)ss, "sockread");
	  Permit ();
	  if (sleep_rc < 0)
	    setrun (FindTask (0));
	  omask = syscall (SYS_sigsetmask, ~0);

	  ss = get_stream(f, TRUE);
	  continue;		/* retry */
	}

      /* okay, there's something to read from the pipe */
      if (ss->reader > ss->writer)
        {
	  /* read till end of buffer and wrap around */
	  int avail = UNIX_SOCKET_SIZE - (ss->reader - ss->buffer);
	  int do_read = len < avail ? len : avail;

	  really_read += do_read;
	  ss->readn += do_read;
	  bcopy (ss->reader, buf, do_read);
	  ss->reader += do_read;
	  len -= do_read;
	  buf += do_read;
	  if (ss->reader - ss->buffer == UNIX_SOCKET_SIZE)
	    /* wrap around */
	    ss->reader = ss->buffer;
	}
      if (len && ss->reader < ss->writer)
        {
	  int avail = ss->writer - ss->reader;
	  int do_read = len < avail ? len : avail;

	  really_read += do_read;
	  ss->readn += do_read;
	  bcopy (ss->reader, buf, do_read);
	  ss->reader += do_read;
	  len -= do_read;
	  buf += do_read;
	}
      Forbid();
      if (ss->wtask)
        Signal(ss->wtask, 1 << getuser(ss->wtask)->u_pipe_sig);
      Permit();

      ix_wakeup((u_int)ss);
    }

  release_stream(ss);
 
  syscall (SYS_sigsetmask, omask);
  errno = err;
  return really_read;
}

int unp_read(struct file *f, char *buf, int len)
{
  usetup;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_read(f, buf, len);
}

int
stream_write (struct file *f, const char *buf, int len)
{
  usetup;
  int omask = syscall (SYS_sigsetmask, ~0);
  int err = errno;
  int sleep_rc;
  int really_written = 0;
  struct sock_stream *ss = get_stream(f, FALSE);

  while (len)
    {
      if (ss == NULL || (ss->flags & UNF_NO_READER))
	{
	  really_written = -1;
	  err = EPIPE;
	  /* this is something no `real' Amiga pipe handler will do ;-)) */
	  _psignal (FindTask (0), SIGPIPE);
	  break;
        }
	
      /* buffer full ?? */
      if (!can_write(ss))
	{
	  if (f->f_flags & FNDELAY)
	    {
	      if (! really_written)
	        {
	          really_written = -1;
	          err = EAGAIN;
	        }
	      break;
	    }

	  /* wait for something to be read or all readers to close */
	  Forbid ();
	  /* sigh.. Forbid() is necessary, or the other end may change
	     the pipe, and in the worst case also settle for sleep(), and
	     there it is.. deadlock.. */
	  release_stream(ss);

	  /* make write interruptible */
	  syscall (SYS_sigsetmask, omask);
	  sleep_rc = ix_sleep ((caddr_t)ss, "sockwrite");
	  Permit ();
	  if (sleep_rc < 0)
	    setrun (FindTask (0));
	  omask = syscall (SYS_sigsetmask, ~0);

	  ss = get_stream(f, FALSE);
	  continue;		/* retry */
	}

      /* okay, there's some space left to write to the pipe */

      if (ss->writer >= ss->reader)
        {
          /* write till end of buffer */
	  int avail = UNIX_SOCKET_SIZE - 1 - (ss->writer - ss->buffer);
	  int do_write;

	  if (ss->reader > ss->buffer)
	    avail++;
	  do_write = len < avail ? len : avail;

	  really_written += do_write;
	  ss->written += do_write;
	  bcopy (buf, ss->writer, do_write);
	  len -= do_write;
	  buf += do_write;
	  ss->writer += do_write;
	  if (ss->writer - ss->buffer == UNIX_SOCKET_SIZE)
	    ss->writer = ss->buffer;
	}

      if (ss->writer < ss->reader - 1)
        {
	  int avail = ss->reader - ss->writer - 1;
	  int do_write = len < avail ? len : avail;

	  really_written += do_write;
	  ss->written += do_write;
	  bcopy (buf, ss->writer, do_write);
	  ss->writer += do_write;
	  len -= do_write;
	  buf += do_write;
	}
      Forbid();
      if (ss->task)
        Signal(ss->task, 1 << getuser(ss->task)->u_pipe_sig);
      Permit();
	
      ix_wakeup((u_int)ss);
    }

  release_stream(ss);

  syscall (SYS_sigsetmask, omask);
  errno = err;
  return really_written;
}

int unp_write(struct file *f, const char *buf, int len)
{
  usetup;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_write(f, buf, len);
}

int unp_ioctl(struct file *f, int cmd, int inout, int arglen, caddr_t arg)
{
  int omask;
  int result = 0;
  struct sock_stream *ss;
  
  omask = syscall (SYS_sigsetmask, ~0);
  ss = get_stream(f, TRUE);

  switch (cmd)
    {
    case FIONREAD:
      {
	unsigned int *pt = (unsigned int *)arg;

        if (ss == NULL)
	  *pt = 0;
	else if (ss->reader < ss->writer)
	  *pt = ss->writer - ss->reader;
	else if (ss->reader > ss->writer)
	  *pt = UNIX_SOCKET_SIZE - (ss->reader - ss->writer);
	else
	  *pt = 0;
	result = 0;
        break;
      }

    case FIONBIO:
      {
	result = f->f_flags & FNDELAY ? 1 : 0;
	if (*(unsigned int *)arg)
	  f->f_flags |= FNDELAY;
	else
	  f->f_flags &= ~FNDELAY;
	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	break;
      }

    case FIOASYNC:
      {
	/* DOESN'T WORK YET */

	int flags = *(unsigned long*)arg;
	result = f->f_flags & FASYNC ? 1 : 0;
	if (flags)
	  f->f_flags |= FASYNC;
	else
	  f->f_flags &= ~FASYNC;

	/* ATTENTION: have to call some function here in the future !!! */

	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	break;
      }

    case FIOCLEX:
    case FIONCLEX:
    case FIOSETOWN:
    case FIOGETOWN:
      /* this is no error, but nevertheless we don't take any actions.. */      
      result = 0;
      break;
    }

  release_stream(ss);
  syscall (SYS_sigsetmask, omask);
  return result;
}

int unp_select(struct file *f, int select_cmd, int io_mode, fd_set *ignored, u_long *also_ignored)
{
  struct sock_stream *ss = find_stream(f, io_mode == SELMODE_IN);
  usetup;

  if (f->f_type != DTYPE_PIPE && ss == NULL)
    {
      struct ix_unix_name *un = f->f_sock->unix_name;
      int result = 1UL << u.u_pipe_sig;

      if (un == NULL)
        return 0;
      ix_lock_base();
      un->task = NULL;
      if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
        {
          if (io_mode == SELMODE_IN)
    	    result = un->queue_index != 0;
          else
            result = 0;
        }
      else
	un->task = FindTask(0);
      ix_unlock_base();
      return result;
    }
  if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
    {
      if (select_cmd == SELCMD_CHECK && io_mode == SELMODE_IN && ss->task == FindTask(0))
        ss->task = NULL;
      if (select_cmd == SELCMD_CHECK && io_mode == SELMODE_OUT && ss->wtask == FindTask(0))
        ss->wtask = NULL;
      /* we support both, read and write checks (hey, something new ;-)) */
      if (io_mode == SELMODE_IN)
	return can_read(ss) || (ss->flags & UNF_NO_WRITER);
      if (io_mode == SELMODE_OUT)
	return can_write(ss) || (ss->flags & UNF_NO_READER);
      return 0;
    }
  if (io_mode == SELMODE_IN)
    {
      ss->task = FindTask(0);
      if (can_read(ss) || (ss->flags & UNF_NO_WRITER))
        Signal(ss->task, 1 << u.u_pipe_sig);
    }
  if (io_mode == SELMODE_OUT)
    {
      ss->wtask = FindTask(0);
      if (can_write(ss) || (ss->flags & UNF_NO_READER))
        Signal(ss->wtask, 1 << u.u_pipe_sig);
    }
  return 1 << u.u_pipe_sig;
}

int unp_close(struct file *f)
{
  ix_lock_base();

  f->f_count--;
  if (f->f_count == 0 && f->f_sock)
    {
      close_stream(f, TRUE, FALSE);
      close_stream(f, FALSE, FALSE);
    }

  ix_unlock_base();
  return 0;
}
