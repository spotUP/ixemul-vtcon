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
 *
 *
 *
 * UP-Term (ixemul-vtcon) 2026-10-05
 *
 *   socketpair(AF_UNIX); sendmsg/recvmsg with SCM_RIGHTS (descriptors ride
 *   with their message and are dropped with the stream if never received);
 *   separate select waiters for reading and writing a stream; connect to a
 *   name nobody listens on is ENOENT (no file) or ECONNREFUSED (a file left
 *   behind), as POSIX says. Carried over from the 48.2 fork onto 80.1's
 *   reference-counted streams.
 *
 * Revision 1.9  2026-08-01  ChatGPT modification  (JJ)
 *
 *   Stage 2 shared sock_stream lifetime support.  Each stream now has one
 *   owner reference and temporary references held by active I/O, ioctl() and
 *   select() operations.  Blocking read/write retains its reference while
 *   sleeping, preventing close from freeing the wait channel.  AF_UNIX final
 *   cleanup now releases stream ownership instead of freeing streams directly.
 *
 * Revision 1.8  2026-08-01  ChatGPT modification  (JJ)
 *
 * Reworked AF_UNIX object lifetime and endpoint state.
 *
 * - Added reference-counted unix_socket and ix_unix_name ownership.
 * - Listener close now fails and wakes queued connect() calls instead of
 *   freeing their queue out from under accept()/connect().
 * - AF_UNIX stream storage is retained until both endpoints and all
 *   transient queue/connect references are gone.
 * - Client and server pathname state is kept separately.
 * - Socket options are kept separately for the client and accepted endpoint.
 * - bind() now uses exclusive creation and cannot truncate an existing file.
 * - Added negative-length and NULL-buffer validation, partial-write
 *   preservation, correct peer task signalling, stricter shutdown/ioctl
 *   handling and owner-safe listener select cancellation.
 *
 * Internal pipe lifetime remains managed by pipe.c and is intentionally not
 * redesigned in this revision.
 *
 * Revision 1.7  2026-07-31  ChatGPT modification  (JJ)
 *
 * Hardened AF_UNIX address handling. bind() and connect() now validate
 * sockaddr_un length, address family and pathname termination before use.
 * accept(), getsockname() and getpeername() now use bounded address output
 * and report the full address length without overrunning caller storage.
 *
 * Added validation for shutdown(), listen backlog and stored SOL_SOCKET
 * values. Added missing socket-state checks in getsockopt(). Zero-length
 * stream I/O now returns zero without modifying errno.
 *
 * Revision 1.6  2026-07-28  ChatGPT modification  (JJ)
 *
 * Added SELCMD_CANCEL handling to unp_select().  Cancellation now
 * removes the current task registration established by SELCMD_PREPARE
 * for internal pipes, connected AF_UNIX sockets and listening sockets.
 *
 * Cancellation is idempotent, does not report readiness and only clears
 * a registration owned by the task performing the cancellation.
 *
 * Revision 1.5  2026-07-10  ChatGPT/Copilot modification  (JJ)
 *
 * Replaced the linear memcpy()-based accept queue in AF_UNIX
 * (unp_accept/unp_connect) with an index-based circular queue using
 * queue_head/queue_tail and queue_index as the element count.
 *
 * This removes O(n) shifting during accept() while preserving FIFO
 * semantics, backlog limits, socket state transitions, locking,
 * wakeups and blocking behaviour.
 *
 * Added limited SOL_SOCKET support to unp_setsockopt() and
 * unp_getsockopt(). SO_RCVBUF, SO_SNDBUF, SO_RCVTIMEO and SO_SNDTIMEO
 * can now be stored and retrieved. SO_TYPE and SO_ERROR can be queried
 * through unp_getsockopt().
 *
 * The stored buffer sizes and timeout values do not alter the fixed
 * stream buffers or waiting behaviour.
 *
 * Corrected unp_accept() address handling so callers may omit name
 * and namelen. Pathname output is now bounded by the supplied buffer
 * size and sockaddr_un lengths are calculated using offsetof().
 *
 * Failed connections now store ECONNREFUSED in so_error for retrieval
 * through getsockopt(SO_ERROR).
 *
 *
 * Revision 1.4  2026-04-17  Copilot modification  (JJ)
 *
 * Replaced all strcpy() calls with bounded strncpy() operations and
 * explicit NUL termination.
 *
 * Normal pathname handling is unchanged. Overlong pathnames are
 * truncated instead of writing beyond the destination buffer.
 *
 *
 * Revision 1.3  2026-04-10  Copilot modification  (JJ)
 *
 * Cleanup of ringbuffer operations in stream_read(), stream_write(),
 * unp_select() and unp_ioctl().
 *
 *   - Added POSIX-compliant len==0 fast paths to stream_read() and
 *     stream_write() so zero-length operations return immediately.
 *
 *   - Unified contiguous read/write handling into single blocks without
 *     altering normal wrap-around semantics.
 *
 *   - Removed duplicated readiness logic in unp_select().
 *
 *   - Expressed FIONREAD ringbuffer geometry consistently with
 *     stream_read().
 *
 * No changes were made to locking, Forbid/Permit ordering, signalling
 * or sock_stream lifetime management.
 */

/*
   Missing features:

   datagram support
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
#include <stddef.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "select.h"

#define can_read(ss)  (ss->reader != ss->writer)
#define can_write(ss) (!(ss->reader == ss->writer + 1 \
                 || (ss->reader == ss->buffer \
                     && ss->writer == ss->buffer + UNIX_SOCKET_SIZE - 1)))

#define UNIX_SOCKET_CAPACITY (UNIX_SOCKET_SIZE - 1)

static void
signal_select_task(task)
  struct Task *task;
{
  if (task != NULL)
    Signal(task, 1UL << getuser(task)->u_pipe_sig);
}

/*
 * Free a stream. A descriptor passed through it (sendmsg SCM_RIGHTS) and
 * never received holds a reference to its file: it goes with the stream.
 */
static void
stream_free(ss)
  struct sock_stream *ss;
{
  while (ss->nrights > 0)
    {
      struct file *r = ss->rights[--ss->nrights];

      if (r->f_close)
        (*r->f_close)(r);
    }
  kfree(ss);
}

/*
 * A stream starts with one owner reference.  Active operations take a
 * temporary reference before touching the object and retain it while sleeping.
 * Memory is freed only after the owner and every active operation have left.
 */
static void
stream_drop_reference(ss)
  struct sock_stream *ss;
{
  int free_stream;

  if (ss == NULL)
    return;

  free_stream = 0;
  Forbid();
  if (ss->refs > 0)
    {
      ss->refs--;
      if (ss->refs == 0)
        free_stream = 1;
    }
  Permit();

  if (free_stream)
    stream_free(ss);
}

static void
stream_owner_release(ss)
  struct sock_stream *ss;
{
  stream_drop_reference(ss);
}

static struct sock_stream *
hold_stream(f, read_stream)
  struct file *f;
  int read_stream;
{
  struct sock_stream *ss;

  Forbid();
  ss = find_stream(f, read_stream);
  if (ss != NULL)
    ss->refs++;
  Permit();
  return ss;
}

/* Caller must already be inside Forbid(). */
static __inline__ void
unlock_stream_locked(ss)
  struct sock_stream *ss;
{
  if (ss->flags & UNF_WANT_LOCK)
    ix_wakeup((u_int)&ss->flags);
  ss->flags &= ~(UNF_WANT_LOCK | UNF_LOCKED);
}

/* The caller already owns a temporary stream reference. */
static void
lock_held_stream(ss)
  struct sock_stream *ss;
{
  Forbid();
  for (;;)
    {
      if (!(ss->flags & UNF_LOCKED))
        {
          ss->flags &= ~UNF_WANT_LOCK;
          ss->flags |= UNF_LOCKED;
          break;
        }

      ss->flags |= UNF_WANT_LOCK;
      if (ix_sleep((caddr_t)&ss->flags, "get_sock") < 0)
        {
          Permit();
          setrun(FindTask(0));
          Forbid();
        }
    }
  Permit();
}

static void
init_socket_options(opt)
  struct unix_socket_options *opt;
{
  opt->so_error = 0;
  opt->so_rcvbuf = UNIX_SOCKET_CAPACITY;
  opt->so_sndbuf = UNIX_SOCKET_CAPACITY;
  opt->so_rcvtimeo.tv_sec = 0;
  opt->so_rcvtimeo.tv_usec = 0;
  opt->so_sndtimeo.tv_sec = 0;
  opt->so_sndtimeo.tv_usec = 0;
  opt->so_type = SOCK_STREAM;
}

static __inline__ struct unix_socket_options *
socket_options(f, us)
  struct file *f;
  struct unix_socket *us;
{
  if (us->server != NULL && us->server == f)
    return &us->server_options;
  return &us->client_options;
}

/*
 * The following reference helpers must be called while ix_lock_base()
 * is held.
 */
static __inline__ void
unix_socket_ref_locked(us)
  struct unix_socket *us;
{
  us->refs++;
}

static void
unix_socket_unref_locked(us)
  struct unix_socket *us;
{
  struct sock_stream *to_server;
  struct sock_stream *from_server;

  if (--us->refs != 0)
    return;

  to_server = us->to_server;
  from_server = us->from_server;
  us->to_server = NULL;
  us->from_server = NULL;
  kfree(us);

  stream_owner_release(to_server);
  stream_owner_release(from_server);
}


static __inline__ void
unix_name_ref_locked(un)
  struct ix_unix_name *un;
{
  un->refs++;
}

static void
unix_name_unref_locked(un)
  struct ix_unix_name *un;
{
  if (--un->refs == 0)
    {
      kfree(un->queue);
      kfree(un);
    }
}

static void
remove_unix_name_locked(un)
  struct ix_unix_name *un;
{
  struct ix_unix_name **link;

  for (link = &ix.ix_unix_names; *link != NULL; link = &(*link)->next)
    {
      if (*link == un)
        {
          *link = un->next;
          un->next = NULL;
          break;
        }
    }
}

/*
 * Remove one waiting client from the circular queue.  This is an uncommon
 * close/error path, so compacting the queue is preferable to leaving holes
 * in the O(1) normal enqueue/dequeue path.
 */
static int
remove_queued_client_locked(un, us)
  struct ix_unix_name *un;
  struct unix_socket *us;
{
  int pos;
  int found;
  int from;
  int to;

  found = -1;
  for (pos = 0; pos < un->queue_index; ++pos)
    {
      from = un->queue_head + pos;
      if (from >= un->queue_size)
        from -= un->queue_size;
      if (un->queue[from] == us)
        {
          found = pos;
          break;
        }
    }

  if (found < 0)
    return 0;

  for (pos = found; pos < un->queue_index - 1; ++pos)
    {
      to = un->queue_head + pos;
      if (to >= un->queue_size)
        to -= un->queue_size;

      from = un->queue_head + pos + 1;
      if (from >= un->queue_size)
        from -= un->queue_size;

      un->queue[to] = un->queue[from];
    }

  to = un->queue_head + un->queue_index - 1;
  if (to >= un->queue_size)
    to -= un->queue_size;
  un->queue[to] = NULL;

  un->queue_index--;
  un->queue_tail = un->queue_head + un->queue_index;
  if (un->queue_tail >= un->queue_size)
    un->queue_tail -= un->queue_size;

  us->connect_name = NULL;
  unix_name_unref_locked(un);       /* queued client's name reference */
  unix_socket_unref_locked(us);     /* queue reference */
  return 1;
}

static struct unix_socket *
dequeue_client_locked(un)
  struct ix_unix_name *un;
{
  struct unix_socket *client;

  if (un->queue_index == 0)
    return NULL;

  client = un->queue[un->queue_head];
  un->queue[un->queue_head] = NULL;

  un->queue_head++;
  if (un->queue_head == un->queue_size)
    un->queue_head = 0;

  un->queue_index--;
  if (un->queue_index == 0)
    un->queue_tail = un->queue_head;

  if (client != NULL)
    {
      client->connect_name = NULL;
      client->state = UNS_PROCESSING;
      unix_name_unref_locked(un);   /* queued client's name reference */
    }

  /*
   * The queue's unix_socket reference is deliberately retained and is
   * transferred to accept() until it either creates the server endpoint
   * or rejects the client.
   */
  return client;
}

static void
fail_queued_clients_locked(un, error)
  struct ix_unix_name *un;
  int error;
{
  struct unix_socket *client;

  while ((client = dequeue_client_locked(un)) != NULL)
    {
      client->state = UNS_ERROR;
      client->client_options.so_error = error;
      ix_wakeup((u_int)client);
      unix_socket_unref_locked(client);  /* release queue reference */
    }
}

static void
close_listener_locked(us)
  struct unix_socket *us;
{
  struct ix_unix_name *un;

  un = us->unix_name;
  if (un == NULL)
    return;

  remove_unix_name_locked(un);
  us->unix_name = NULL;

  un->closing = 1;
  signal_select_task(un->task);
  un->task = NULL;

  fail_queued_clients_locked(un, ECONNREFUSED);
  ix_wakeup((u_int)un);

  unix_name_unref_locked(un);       /* listener ownership */
}


/*
 * Copy and validate an AF_UNIX pathname supplied by the caller.
 * The pathname must contain a NUL byte within namelen and must fit in dst.
 */
static int
copy_unix_path(const struct sockaddr *name, int namelen,
               char *dst, int dstsize)
{
  const struct sockaddr_un *sun;
  int path_offset;
  int path_bytes;
  int len;

  path_offset = (int)offsetof(struct sockaddr_un, sun_path);

  if (name == NULL || dst == NULL || dstsize <= 0)
    return EINVAL;

  if (namelen <= path_offset || namelen > (int)sizeof(struct sockaddr_un))
    return EINVAL;

  sun = (const struct sockaddr_un *)name;
  if (sun->sun_family != AF_UNIX)
    return EAFNOSUPPORT;

  path_bytes = namelen - path_offset;
  len = 0;
  while (len < path_bytes && sun->sun_path[len] != '\0')
    len++;

  if (len == path_bytes)
    return EINVAL;
  if (len == 0)
    return EINVAL;
  if (len >= dstsize)
    return ENAMETOOLONG;

  memcpy(dst, sun->sun_path, (size_t)len);
  dst[len] = '\0';
  return 0;
}

/*
 * Return an AF_UNIX address without writing beyond the caller's capacity.
 * As in recvfrom(), *alen receives the full address length even if output
 * had to be truncated.
 */
static int
copyout_unix_address(struct sockaddr *asa, int *alen, const char *path)
{
  struct sockaddr_un sun;
  int pathlen;
  int actual;
  int copylen;

  if (asa == NULL || alen == NULL || path == NULL || *alen < 0)
    return EINVAL;

  pathlen = strlen(path);
  if (pathlen >= (int)sizeof(sun.sun_path))
    pathlen = sizeof(sun.sun_path) - 1;

  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  memcpy(sun.sun_path, path, (size_t)pathlen);
  sun.sun_path[pathlen] = '\0';

  actual = (int)offsetof(struct sockaddr_un, sun_path) + pathlen + 1;
  sun.sun_len = actual;

  copylen = *alen;
  if (copylen > actual)
    copylen = actual;
  if (copylen > (int)sizeof(sun))
    copylen = sizeof(sun);

  if (copylen > 0)
    memcpy(asa, &sun, (size_t)copylen);

  *alen = actual;
  return 0;
}

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
  struct sock_stream *s;

  s = kmalloc(sizeof(struct sock_stream));
  if (s != NULL)
    {
      s->reader = s->writer = s->buffer;
      s->flags = 0;
      s->task = NULL;
      s->wtask = NULL;
      s->nrights = 0;
      s->written = s->readn = 0;
      s->refs = 1;                 /* owning pipe or unix_socket */
    }
  return s;
}



static struct ix_unix_name *
add_unix_name(const char *path, int queue_size)
{
  struct ix_unix_name *un;
  usetup;

  un = kmalloc(sizeof(struct ix_unix_name));
  if (un == NULL)
    errno_return(ENOMEM, NULL);

  if (queue_size == 0)
    queue_size = 1;
  if (queue_size < 0 ||
      queue_size > INT_MAX / (int)sizeof(struct unix_socket *))
    {
      kfree(un);
      errno_return(EINVAL, NULL);
    }

  un->queue = kmalloc(queue_size * sizeof(struct unix_socket *));
  if (un->queue == NULL)
    {
      kfree(un);
      errno_return(ENOMEM, NULL);
    }

  memset(un->queue, 0, queue_size * sizeof(struct unix_socket *));
  strncpy(un->path, path, sizeof(un->path));
  un->path[sizeof(un->path) - 1] = '\0';

  un->next = ix.ix_unix_names;
  ix.ix_unix_names = un;
  un->queue_size = queue_size;
  un->queue_index = 0;
  un->queue_head = 0;
  un->queue_tail = 0;
  un->task = NULL;
  un->refs = 1;                    /* listener ownership */
  un->closing = 0;
  return un;
}


struct sock_stream *
find_stream(struct file *f, int read_stream)
{
  struct unix_socket *us;

  if (f == NULL)
    return NULL;

  if (f->f_type == DTYPE_PIPE)
    return f->f_ss;

  us = f->f_sock;
  if (us == NULL)
    return NULL;

  if (us->server != f)
    read_stream = !read_stream;

  return read_stream ? us->to_server : us->from_server;
}


struct sock_stream *
get_stream(struct file *f, int read_stream)
{
  struct sock_stream *ss;

  ss = hold_stream(f, read_stream);
  if (ss != NULL)
    lock_held_stream(ss);
  return ss;
}


void
release_stream(struct sock_stream *ss)
{
  int free_stream;

  if (ss == NULL)
    return;

  free_stream = 0;
  Forbid();
  unlock_stream_locked(ss);
  if (ss->refs > 0)
    {
      ss->refs--;
      if (ss->refs == 0)
        free_stream = 1;
    }
  Permit();

  if (free_stream)
    stream_free(ss);
}


static void
close_stream(struct file *f, int read_stream)
{
  struct sock_stream *ss;
  struct unix_socket *us;
  int to_server;
  int flag;

  us = f->f_sock;
  if (us == NULL)
    return;

  to_server = (us->server != f) ? !read_stream : read_stream;
  ss = to_server ? us->to_server : us->from_server;
  if (ss == NULL)
    return;

  flag = read_stream ? UNF_NO_READER : UNF_NO_WRITER;

  Forbid();
  if (!(ss->flags & flag))
    {
      ss->flags |= flag;
      signal_select_task(ss->task);
      signal_select_task(ss->wtask);
      ix_wakeup((u_int)ss);
    }
  Permit();

  /*
   * Do not free the stream here.  Both endpoints share it, and readers,
   * writers or select() may still hold its address.  The stream is released
   * only when the owning unix_socket reference count reaches zero.
   */
}


int
unp_socket(int domain, int type, int protocol, struct unix_socket *sock)
{
  int omask;
  int err;
  int fd;
  struct file *fp;
  struct unix_socket *us;
  usetup;

  if (type != SOCK_STREAM || protocol != 0)
    errno_return(EPROTONOSUPPORT, -1);

  if (sock != NULL)
    {
      us = sock;
      ix_lock_base();
      unix_socket_ref_locked(us);      /* new file endpoint */
      ix_unlock_base();
    }
  else
    {
      us = kmalloc(sizeof(struct unix_socket));
      if (us == NULL)
        errno_return(ENOMEM, -1);

      us->client_path[0] = '\0';
      us->server_path[0] = '\0';
      us->from_server = NULL;
      us->to_server = NULL;
      us->unix_name = NULL;
      us->connect_name = NULL;
      us->server = NULL;
      us->state = UNS_WAITING;
      us->refs = 1;                    /* client/listener file endpoint */
      init_socket_options(&us->client_options);
      init_socket_options(&us->server_options);
    }

  omask = syscall(SYS_sigsetmask, ~0);

  err = falloc(&fp, &fd);
  if (err)
    {
      errno = err;
      syscall(SYS_sigsetmask, omask);

      ix_lock_base();
      unix_socket_unref_locked(us);
      ix_unlock_base();
      return -1;
    }

  fp->f_sock = us;
  _set_socket_params(fp, domain, 0, 0);

  syscall(SYS_sigsetmask, omask);
  return fd;
}


int
unp_bind(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *fp;
  struct ix_unix_name *un;
  int tmp;
  int err;
  char path[sizeof(((struct unix_socket *)0)->client_path)];

  if (s < 0 || s >= NOFILE || (fp = u.u_ofile[s]) == NULL ||
      fp->f_sock == NULL)
    errno_return(EBADF, -1);

  if (fp->f_sock->client_path[0] != '\0')
    errno_return(EINVAL, -1);

  err = copy_unix_path(name, namelen, path, sizeof(path));
  if (err)
    errno_return(err, -1);

  ix_lock_base();
  un = find_unix_name(path);
  ix_unlock_base();
  if (un != NULL)
    errno_return(EADDRINUSE, -1);

  tmp = syscall(SYS_open, path, O_WRONLY | O_CREAT | O_EXCL, 0777);
  if (tmp < 0)
    {
      if (errno == EEXIST)
        errno = EADDRINUSE;
      return -1;
    }

  syscall(SYS_close, tmp);
  strcpy(fp->f_sock->client_path, path);
  return 0;
}


int
unp_listen(int s, int backlog)
{
  usetup;
  struct file *fp;
  struct unix_socket *us;

  if (s < 0 || s >= NOFILE || (fp = u.u_ofile[s]) == NULL ||
      fp->f_sock == NULL)
    errno_return(EBADF, -1);

  us = fp->f_sock;

  if (backlog < 0)
    errno_return(EINVAL, -1);

  if (us->client_path[0] == '\0' || us->unix_name != NULL ||
      us->to_server != NULL || us->from_server != NULL)
    errno_return(EOPNOTSUPP, -1);

  ix_lock_base();
  if (find_unix_name(us->client_path) != NULL)
    {
      ix_unlock_base();
      errno_return(EADDRINUSE, -1);
    }

  us->unix_name = add_unix_name(us->client_path, backlog);
  ix_unlock_base();

  return us->unix_name != NULL ? 0 : -1;
}


int
unp_accept(int s, struct sockaddr *name, int *namelen)
{
  usetup;
  struct file *listener;
  struct file *accepted;
  struct unix_socket *listener_us;
  struct unix_socket *client;
  struct ix_unix_name *un;
  struct unix_socket_options inherited_options;
  char server_path[sizeof(((struct unix_socket *)0)->server_path)];
  char client_path[sizeof(((struct unix_socket *)0)->client_path)];
  int omask;
  int err;
  int sleep_rc;
  int fd;
  int closing;

  if ((name == NULL) != (namelen == NULL))
    errno_return(EINVAL, -1);
  if (namelen != NULL && *namelen < 0)
    errno_return(EINVAL, -1);

  if (s < 0 || s >= NOFILE || (listener = u.u_ofile[s]) == NULL ||
      listener->f_sock == NULL)
    errno_return(EBADF, -1);

  listener_us = listener->f_sock;

  ix_lock_base();
  un = listener_us->unix_name;
  if (un == NULL || un->closing)
    {
      ix_unlock_base();
      errno_return(EOPNOTSUPP, -1);
    }
  unix_name_ref_locked(un);            /* accept() transient reference */
  ix_unlock_base();

  omask = syscall(SYS_sigsetmask, ~0);
  __get_file(listener);
  err = 0;
  client = NULL;

  for (;;)
    {
      ix_lock_base();
      closing = un->closing;

      if (un->queue_index != 0)
        {
          client = dequeue_client_locked(un);
          if (client != NULL)
            {
              inherited_options = listener_us->client_options;
              strncpy(server_path, un->path, sizeof(server_path));
              server_path[sizeof(server_path) - 1] = '\0';
              strncpy(client_path, client->client_path, sizeof(client_path));
              client_path[sizeof(client_path) - 1] = '\0';
            }
        }
      ix_unlock_base();

      if (client != NULL)
        break;

      if (closing)
        {
          err = ECONNREFUSED;
          break;
        }

      if (listener->f_flags & FNDELAY)
        {
          err = EWOULDBLOCK;
          break;
        }

      Forbid();
      __release_file(listener);
      syscall(SYS_sigsetmask, omask);
      sleep_rc = ix_sleep((caddr_t)un, "accept");
      Permit();

      if (sleep_rc < 0)
        setrun(FindTask(0));

      omask = syscall(SYS_sigsetmask, ~0);
      __get_file(listener);
    }

  __release_file(listener);
  syscall(SYS_sigsetmask, omask);

  if (err != 0)
    {
      ix_lock_base();
      unix_name_unref_locked(un);
      ix_unlock_base();
      errno_return(err, -1);
    }

  fd = unp_socket(PF_UNIX, SOCK_STREAM, 0, client);
  if (fd == -1)
    {
      ix_lock_base();
      client->state = UNS_ERROR;
      client->client_options.so_error = ECONNREFUSED;
      ix_wakeup((u_int)client);
      unix_socket_unref_locked(client);  /* release queue reference */
      unix_name_unref_locked(un);
      ix_unlock_base();
      return -1;
    }

  ix_lock_base();

  if (client->state == UNS_ERROR)
    {
      ix_unlock_base();
      syscall(SYS_close, fd);

      ix_lock_base();
      unix_socket_unref_locked(client);  /* release queue reference */
      unix_name_unref_locked(un);
      ix_unlock_base();

      errno_return(ECONNREFUSED, -1);
    }

  accepted = u.u_ofile[fd];
  client->server = accepted;
  client->server_options = inherited_options;
  strncpy(client->server_path, server_path, sizeof(client->server_path));
  client->server_path[sizeof(client->server_path) - 1] = '\0';
  client->state = UNS_ACCEPTED;

  ix_wakeup((u_int)client);
  unix_socket_unref_locked(client);      /* release queue reference */
  unix_name_unref_locked(un);
  ix_unlock_base();

  if (name != NULL)
    (void)copyout_unix_address(name, namelen, client_path);

  return fd;
}


int
unp_connect(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *f;
  struct unix_socket *us;
  struct ix_unix_name *un;
  struct sock_stream *to_server;
  struct sock_stream *from_server;
  struct Task *listener_task;
  int sleep_rc;
  int state;
  int err;
  char path[sizeof(((struct unix_socket *)0)->server_path)];

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  us = f->f_sock;

  if (us->unix_name != NULL)
    errno_return(EOPNOTSUPP, -1);
  if (us->state == UNS_ACCEPTED || us->to_server != NULL ||
      us->from_server != NULL)
    errno_return(EISCONN, -1);

  err = copy_unix_path(name, namelen, path, sizeof(path));
  if (err)
    errno_return(err, -1);

  to_server = init_stream();
  if (to_server == NULL)
    errno_return(ENOMEM, -1);

  from_server = init_stream();
  if (from_server == NULL)
    {
      stream_owner_release(to_server);
      errno_return(ENOMEM, -1);
    }

  ix_lock_base();

  un = find_unix_name(path);
  if (un == NULL || un->closing)
    {
      struct stat st;

      ix_unlock_base();
      stream_owner_release(to_server);
      stream_owner_release(from_server);
      /* as POSIX says: no file there is ENOENT, a file (a socket left
         behind, its server gone) with nobody listening ECONNREFUSED --
         tmux starts its server on either, and on nothing else (UP-Term) */
      err = syscall(SYS_stat, path, &st) == 0 ? ECONNREFUSED : ENOENT;
      us->client_options.so_error = err;
      errno_return(err, -1);
    }

  if (un->queue_index == un->queue_size)
    {
      ix_unlock_base();
      stream_owner_release(to_server);
      stream_owner_release(from_server);
      us->client_options.so_error = ECONNREFUSED;
      errno_return(ECONNREFUSED, -1);
    }

  us->to_server = to_server;
  us->from_server = from_server;
  us->state = UNS_WAITING;
  us->client_options.so_error = 0;
  strncpy(us->server_path, path, sizeof(us->server_path));
  us->server_path[sizeof(us->server_path) - 1] = '\0';

  unix_socket_ref_locked(us);           /* blocking connect() reference */
  unix_socket_ref_locked(us);           /* queue reference */
  unix_name_ref_locked(un);             /* queued client's name reference */
  unix_name_ref_locked(un);             /* local wakeup reference */

  us->connect_name = un;
  un->queue[un->queue_tail] = us;
  un->queue_tail++;
  if (un->queue_tail == un->queue_size)
    un->queue_tail = 0;
  un->queue_index++;

  listener_task = un->task;
  ix_unlock_base();

  signal_select_task(listener_task);
  ix_wakeup((u_int)un);

  ix_lock_base();
  unix_name_unref_locked(un);           /* local wakeup reference */
  ix_unlock_base();

  Forbid();
  state = us->state;
  while (state != UNS_ACCEPTED && state != UNS_ERROR)
    {
      sleep_rc = ix_sleep((caddr_t)us, "connect");
      state = us->state;

      /*
       * Preserve ixemul's historical connect semantics: a signal causes the
       * task to be rescheduled, but connect() continues waiting.
       */
      if (sleep_rc < 0)
        {
          Permit();
          setrun(FindTask(0));
          Forbid();
        }
    }
  Permit();

  if (state == UNS_ERROR)
    {
      ix_lock_base();

      if (us->connect_name != NULL)
        (void)remove_queued_client_locked(us->connect_name, us);

      if (us->server == NULL)
        {
          if (us->to_server != NULL)
            stream_owner_release(us->to_server);
          if (us->from_server != NULL)
            stream_owner_release(us->from_server);
          us->to_server = NULL;
          us->from_server = NULL;
        }

      us->server_path[0] = '\0';
      us->state = UNS_WAITING;
      err = us->client_options.so_error;
      if (err == 0)
        err = ECONNREFUSED;

      unix_socket_unref_locked(us);     /* blocking connect() reference */
      ix_unlock_base();

      errno_return(err, -1);
    }

  ix_lock_base();
  unix_socket_unref_locked(us);         /* blocking connect() reference */
  ix_unlock_base();
  return 0;
}


/*
 * socketpair(AF_UNIX, SOCK_STREAM): two connected sockets at once, as
 * accept() makes them -- one unix_socket with its two streams, the first
 * descriptor the connecting end, the second the accepted one (us->server).
 * No name and no listener: connect() sleeps until an accept, so the pair
 * cannot be made that way in one process. (UP-Term: libevent's signal
 * pipe, tmux's client and server)
 */
int
unp_socketpair(int domain, int type, int protocol, int sv[2])
{
  usetup;
  struct unix_socket *us;
  struct sock_stream *to_server;
  struct sock_stream *from_server;
  int fd0, fd1;

  if (sv == NULL)
    errno_return(EFAULT, -1);

  to_server = init_stream();
  if (to_server == NULL)
    errno_return(ENOMEM, -1);
  from_server = init_stream();
  if (from_server == NULL)
    {
      stream_owner_release(to_server);
      errno_return(ENOMEM, -1);
    }

  fd0 = unp_socket(domain, type, protocol, NULL);
  if (fd0 == -1)
    {
      stream_owner_release(to_server);
      stream_owner_release(from_server);
      return -1;
    }

  us = u.u_ofile[fd0]->f_sock;
  ix_lock_base();
  us->to_server = to_server;           /* the socket owns them now */
  us->from_server = from_server;
  us->state = UNS_ACCEPTED;
  ix_unlock_base();

  fd1 = unp_socket(domain, type, protocol, us);
  if (fd1 == -1)
    {
      syscall(SYS_close, fd0);
      return -1;
    }

  ix_lock_base();
  us->server = u.u_ofile[fd1];
  ix_unlock_base();

  sv[0] = fd0;
  sv[1] = fd1;
  return 0;
}


/*
 * sendmsg/recvmsg on a local socket: the data of the iovecs through the
 * stream, and SCM_RIGHTS -- descriptors passed to the other process (GNU
 * screen hands its backend the attaching terminal this way, tmux too).
 * An ixemul file is a shared struct file: passing one is a reference
 * queued on the stream, and a new descriptor for it in the receiver.
 * Network sockets (DTYPE_SOCKET) cannot be passed. (UP-Term)
 */
int
unp_sendmsg(int s, const struct msghdr *msg, int flags)
{
  usetup;
  struct file *f, *fr[UNIX_SOCKET_RIGHTS];
  int nr = 0, i, total = 0, n;
  struct cmsghdr *cm;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (msg == NULL || (msg->msg_iovlen > 0 && msg->msg_iov == NULL))
    errno_return(EFAULT, -1);
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


int
unp_recvmsg(int s, struct msghdr *msg, int flags)
{
  usetup;
  struct file *f;
  int i, n = 0, total = 0, limit = -1, deliver, fit, k = 0;
  struct sock_stream *ss;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (msg == NULL || (msg->msg_iovlen > 0 && msg->msg_iov == NULL))
    errno_return(EFAULT, -1);
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
           (ss->right_at[deliver] < ss->readn ||
            (n == 0 && ss->right_at[deliver] <= ss->readn)))
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


int
unp_send(int s, const void *buf, int len, int flags)
{
  usetup;
  struct file *f;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (flags != 0)
    errno_return(EOPNOTSUPP, -1);
  if (len < 0)
    errno_return(EINVAL, -1);
  if (len != 0 && buf == NULL)
    errno_return(EFAULT, -1);

  return unp_write(f, buf, len);
}


int
unp_recv(int s, void *buf, int len, int flags)
{
  usetup;
  struct file *f;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (flags != 0)
    errno_return(EOPNOTSUPP, -1);
  if (len < 0)
    errno_return(EINVAL, -1);
  if (len != 0 && buf == NULL)
    errno_return(EFAULT, -1);

  return unp_read(f, buf, len);
}


int
unp_shutdown(int s, int how)
{
  usetup;
  struct file *f;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  if (how < 0 || how > 2)
    errno_return(EINVAL, -1);

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);

  ix_lock_base();
  switch (how)
    {
    case 0:
      close_stream(f, TRUE);
      break;
    case 1:
      close_stream(f, FALSE);
      break;
    case 2:
      close_stream(f, TRUE);
      close_stream(f, FALSE);
      break;
    }
  ix_unlock_base();
  return 0;
}


int
unp_setsockopt(int s, int level, int name, const void *val, int valsize)
{
  usetup;
  struct file *f;
  struct unix_socket *us;
  struct unix_socket_options *opt;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  if (level != SOL_SOCKET)
    errno_return(ENOPROTOOPT, -1);

  us = f->f_sock;
  opt = socket_options(f, us);

  switch (name)
    {
    case SO_RCVBUF:
      if (val == NULL || valsize != (int)sizeof(int) ||
          *(const int *)val <= 0)
        errno_return(EINVAL, -1);
      opt->so_rcvbuf = *(const int *)val;
      return 0;

    case SO_SNDBUF:
      if (val == NULL || valsize != (int)sizeof(int) ||
          *(const int *)val <= 0)
        errno_return(EINVAL, -1);
      opt->so_sndbuf = *(const int *)val;
      return 0;

    case SO_RCVTIMEO:
      if (val == NULL || valsize != (int)sizeof(struct timeval))
        errno_return(EINVAL, -1);
      if (((const struct timeval *)val)->tv_sec < 0 ||
          ((const struct timeval *)val)->tv_usec < 0 ||
          ((const struct timeval *)val)->tv_usec >= 1000000)
        errno_return(EINVAL, -1);
      opt->so_rcvtimeo = *(const struct timeval *)val;
      return 0;

    case SO_SNDTIMEO:
      if (val == NULL || valsize != (int)sizeof(struct timeval))
        errno_return(EINVAL, -1);
      if (((const struct timeval *)val)->tv_sec < 0 ||
          ((const struct timeval *)val)->tv_usec < 0 ||
          ((const struct timeval *)val)->tv_usec >= 1000000)
        errno_return(EINVAL, -1);
      opt->so_sndtimeo = *(const struct timeval *)val;
      return 0;

    case SO_TYPE:
    case SO_ERROR:
      errno_return(EINVAL, -1);

    default:
      errno_return(ENOPROTOOPT, -1);
    }
}


int
unp_getsockopt(int s, int level, int name, void *val, int *valsize)
{
  usetup;
  struct file *f;
  struct unix_socket *us;
  struct unix_socket_options *opt;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  if (level != SOL_SOCKET)
    errno_return(ENOPROTOOPT, -1);

  if (val == NULL || valsize == NULL || *valsize < 0)
    errno_return(EINVAL, -1);

  us = f->f_sock;
  opt = socket_options(f, us);

  switch (name)
    {
    case SO_TYPE:
      if (*valsize < (int)sizeof(int))
        errno_return(EINVAL, -1);
      *(int *)val = opt->so_type;
      *valsize = sizeof(int);
      return 0;

    case SO_ERROR:
      if (*valsize < (int)sizeof(int))
        errno_return(EINVAL, -1);
      *(int *)val = opt->so_error;
      opt->so_error = 0;
      *valsize = sizeof(int);
      return 0;

    case SO_RCVBUF:
      if (*valsize < (int)sizeof(int))
        errno_return(EINVAL, -1);
      *(int *)val = opt->so_rcvbuf;
      *valsize = sizeof(int);
      return 0;

    case SO_SNDBUF:
      if (*valsize < (int)sizeof(int))
        errno_return(EINVAL, -1);
      *(int *)val = opt->so_sndbuf;
      *valsize = sizeof(int);
      return 0;

    case SO_RCVTIMEO:
      if (*valsize < (int)sizeof(struct timeval))
        errno_return(EINVAL, -1);
      *(struct timeval *)val = opt->so_rcvtimeo;
      *valsize = sizeof(struct timeval);
      return 0;

    case SO_SNDTIMEO:
      if (*valsize < (int)sizeof(struct timeval))
        errno_return(EINVAL, -1);
      *(struct timeval *)val = opt->so_sndtimeo;
      *valsize = sizeof(struct timeval);
      return 0;

    default:
      errno_return(ENOPROTOOPT, -1);
    }
}


int
unp_getsockname(int s, struct sockaddr *asa, int *alen)
{
  usetup;
  struct file *f;
  struct unix_socket *us;
  const char *path;
  int err;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  us = f->f_sock;
  if (us->server != NULL && us->server == f)
    path = us->server_path;
  else
    path = us->client_path;

  err = copyout_unix_address(asa, alen, path);
  if (err)
    errno_return(err, -1);
  return 0;
}


int
unp_getpeername(int s, struct sockaddr *asa, int *alen)
{
  usetup;
  struct file *f;
  struct unix_socket *us;
  const char *path;
  int err;

  if (s < 0 || s >= NOFILE || (f = u.u_ofile[s]) == NULL ||
      f->f_sock == NULL)
    errno_return(EBADF, -1);

  us = f->f_sock;
  if (us->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);

  if (us->server != NULL && us->server == f)
    path = us->client_path;
  else
    path = us->server_path;

  err = copyout_unix_address(asa, alen, path);
  if (err)
    errno_return(err, -1);
  return 0;
}


int
stream_read(struct file *f, char *buf, int len)
{
  usetup;
  int omask;
  int err;
  int really_read;
  int sleep_rc;
  struct sock_stream *ss;

  if (len < 0)
    errno_return(EINVAL, -1);
  if (len == 0)
    return 0;
  if (buf == NULL)
    errno_return(EFAULT, -1);

  omask = syscall(SYS_sigsetmask, ~0);
  err = errno;
  really_read = 0;
  ss = get_stream(f, TRUE);

  while (len != 0)
    {
      if (ss == NULL || !can_read(ss))
        {
          if (really_read != 0 || ss == NULL ||
              (ss->flags & UNF_NO_WRITER))
            {
              err = 0;
              break;
            }

          if (f->f_flags & FNDELAY)
            {
              really_read = -1;
              err = EAGAIN;
              break;
            }

          /*
           * Retain the temporary reference while sleeping on ss.  Only the
           * ring-buffer lock is released here.
           */
          Forbid();
          unlock_stream_locked(ss);
          syscall(SYS_sigsetmask, omask);
          sleep_rc = ix_sleep((caddr_t)ss, "sockread");
          Permit();

          if (sleep_rc < 0)
            setrun(FindTask(0));

          omask = syscall(SYS_sigsetmask, ~0);
          lock_held_stream(ss);
          continue;
        }

      {
        int avail;
        int do_read;

        if (ss->reader < ss->writer)
          avail = ss->writer - ss->reader;
        else
          avail = UNIX_SOCKET_SIZE - (ss->reader - ss->buffer);

        do_read = len < avail ? len : avail;
        really_read += do_read;
        ss->readn += do_read;
        bcopy(ss->reader, buf, do_read);
        ss->reader += do_read;
        len -= do_read;
        buf += do_read;

        if (ss->reader - ss->buffer == UNIX_SOCKET_SIZE)
          ss->reader = ss->buffer;
      }

      /* room now: the writer waiting in select */
      Forbid();
      signal_select_task(ss->wtask);
      Permit();
      ix_wakeup((u_int)ss);
    }

  release_stream(ss);
  syscall(SYS_sigsetmask, omask);
  errno = err;
  return really_read;
}



int
unp_read(struct file *f, char *buf, int len)
{
  usetup;

  if (f == NULL || f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_read(f, buf, len);
}


int
stream_write(struct file *f, const char *buf, int len)
{
  usetup;
  int omask;
  int err;
  int sleep_rc;
  int really_written;
  struct sock_stream *ss;

  if (len < 0)
    errno_return(EINVAL, -1);
  if (len == 0)
    return 0;
  if (buf == NULL)
    errno_return(EFAULT, -1);

  omask = syscall(SYS_sigsetmask, ~0);
  err = errno;
  really_written = 0;
  ss = get_stream(f, FALSE);

  while (len != 0)
    {
      if (ss == NULL || (ss->flags & UNF_NO_READER))
        {
          if (really_written == 0)
            {
              really_written = -1;
              err = EPIPE;
              _psignal(FindTask(0), SIGPIPE);
            }
          break;
        }

      if (!can_write(ss))
        {
          if (f->f_flags & FNDELAY)
            {
              if (really_written == 0)
                {
                  really_written = -1;
                  err = EAGAIN;
                }
              break;
            }

          /* Keep the temporary reference while ss is the sleep channel. */
          Forbid();
          unlock_stream_locked(ss);
          syscall(SYS_sigsetmask, omask);
          sleep_rc = ix_sleep((caddr_t)ss, "sockwrite");
          Permit();

          if (sleep_rc < 0)
            setrun(FindTask(0));

          omask = syscall(SYS_sigsetmask, ~0);
          lock_held_stream(ss);
          continue;
        }

      {
        int avail;
        int do_write;

        if (ss->writer < ss->reader)
          avail = ss->reader - ss->writer - 1;
        else
          {
            avail = UNIX_SOCKET_SIZE - 1 -
                    (ss->writer - ss->buffer);
            if (ss->reader > ss->buffer)
              avail++;
          }

        if (avail > 0)
          {
            do_write = len < avail ? len : avail;
            really_written += do_write;
            ss->written += do_write;
            bcopy(buf, ss->writer, do_write);
            len -= do_write;
            buf += do_write;
            ss->writer += do_write;

            if (ss->writer - ss->buffer == UNIX_SOCKET_SIZE)
              ss->writer = ss->buffer;
          }
      }

      Forbid();
      signal_select_task(ss->task);
      Permit();
      ix_wakeup((u_int)ss);
    }

  release_stream(ss);
  syscall(SYS_sigsetmask, omask);
  errno = err;
  return really_written;
}



int
unp_write(struct file *f, const char *buf, int len)
{
  usetup;

  if (f == NULL || f->f_sock == NULL)
    errno_return(EBADF, -1);
  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_write(f, buf, len);
}


int
unp_ioctl(struct file *f, int cmd, int inout, int arglen, caddr_t arg)
{
  usetup;
  int omask;
  int result;
  struct sock_stream *ss;

  (void)inout;
  (void)arglen;

  omask = syscall(SYS_sigsetmask, ~0);
  ss = get_stream(f, TRUE);
  result = 0;

  switch (cmd)
    {
    case FIONREAD:
      {
        unsigned int *pt;

        if (arg == NULL)
          {
            result = -1;
            errno = EINVAL;
            break;
          }

        pt = (unsigned int *)arg;
        if (ss == NULL)
          *pt = 0;
        else if (ss->reader < ss->writer)
          *pt = ss->writer - ss->reader;
        else if (ss->reader > ss->writer)
          *pt = UNIX_SOCKET_SIZE - (ss->reader - ss->writer);
        else
          *pt = 0;
        break;
      }

    case FIONBIO:
      if (arg == NULL)
        {
          result = -1;
          errno = EINVAL;
          break;
        }
      result = (f->f_flags & FNDELAY) ? 1 : 0;
      if (*(unsigned int *)arg)
        f->f_flags |= FNDELAY;
      else
        f->f_flags &= ~FNDELAY;
      break;

    case FIOASYNC:
      if (arg == NULL)
        {
          result = -1;
          errno = EINVAL;
          break;
        }
      result = (f->f_flags & FASYNC) ? 1 : 0;
      if (*(unsigned long *)arg)
        f->f_flags |= FASYNC;
      else
        f->f_flags &= ~FASYNC;
      break;

    case FIOCLEX:
    case FIONCLEX:
    case FIOSETOWN:
      result = 0;
      break;

    case FIOGETOWN:
      if (arg == NULL)
        {
          result = -1;
          errno = EINVAL;
        }
      else
        {
          *(int *)arg = 0;
          result = 0;
        }
      break;

    default:
      result = -1;
#ifdef ENOTTY
      errno = ENOTTY;
#else
      errno = EINVAL;
#endif
      break;
    }

  release_stream(ss);
  syscall(SYS_sigsetmask, omask);
  return result;
}


int
unp_select(struct file *f, int select_cmd, int io_mode,
           fd_set *ignored, u_long *also_ignored)
{
  struct sock_stream *ss;
  struct Task *task;
  struct ix_unix_name *un;
  int result;
  int ready_in;
  int ready_out;
  usetup;

  (void)ignored;
  (void)also_ignored;

  if (io_mode != SELMODE_IN && io_mode != SELMODE_OUT)
    return 0;

  if (f == NULL)
    return 0;

  ss = hold_stream(f, io_mode == SELMODE_IN);
  task = FindTask(0);

  if (f->f_type != DTYPE_PIPE && ss == NULL)
    {
      if (f->f_sock == NULL || io_mode != SELMODE_IN)
        return 0;

      un = f->f_sock->unix_name;
      if (un == NULL)
        return 0;

      ix_lock_base();

      if (select_cmd == SELCMD_CANCEL)
        {
          if (un->task == task)
            un->task = NULL;
          ix_unlock_base();
          return 0;
        }

      if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
        {
          result = !un->closing && un->queue_index != 0;

          if (select_cmd == SELCMD_CHECK && un->task == task)
            un->task = NULL;

          ix_unlock_base();
          return result;
        }

      if (!un->closing)
        un->task = task;
      result = un->closing ? 0 : (1UL << u.u_pipe_sig);
      ix_unlock_base();
      return result;
    }

  if (ss == NULL)
    return 0;

  Forbid();

  if (select_cmd == SELCMD_CANCEL)
    {
      if (io_mode == SELMODE_IN ? ss->task == task : ss->wtask == task)
        *(io_mode == SELMODE_IN ? &ss->task : &ss->wtask) = NULL;
      Permit();
      stream_drop_reference(ss);
      return 0;
    }

  ready_in = can_read(ss) || (ss->flags & UNF_NO_WRITER);
  ready_out = can_write(ss) || (ss->flags & UNF_NO_READER);

  if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
    {
      if (select_cmd == SELCMD_CHECK &&
          (io_mode == SELMODE_IN ? ss->task == task : ss->wtask == task))
        *(io_mode == SELMODE_IN ? &ss->task : &ss->wtask) = NULL;

      result = io_mode == SELMODE_IN ? ready_in : ready_out;
      Permit();
      stream_drop_reference(ss);
      return result;
    }

  /* reading and writing wait in separate slots: a reader and a writer in
     two processes on one stream each get their own wake-up (UP-Term) */
  if (io_mode == SELMODE_IN)
    ss->task = task;
  else
    ss->wtask = task;
  if ((io_mode == SELMODE_IN && ready_in) ||
      (io_mode == SELMODE_OUT && ready_out))
    signal_select_task(task);

  Permit();
  stream_drop_reference(ss);
  return 1UL << u.u_pipe_sig;
}



int
unp_close(struct file *f)
{
  struct unix_socket *us;

  ix_lock_base();

  f->f_count--;
  if (f->f_count != 0)
    {
      ix_unlock_base();
      return 0;
    }

  us = f->f_sock;
  if (us == NULL)
    {
      ix_unlock_base();
      return 0;
    }

  if (us->unix_name != NULL)
    close_listener_locked(us);

  if (us->connect_name != NULL)
    {
      (void)remove_queued_client_locked(us->connect_name, us);
      us->state = UNS_ERROR;
      us->client_options.so_error = ECONNREFUSED;
      ix_wakeup((u_int)us);
    }
  else if (us->state == UNS_PROCESSING && us->server == NULL)
    {
      us->state = UNS_ERROR;
      us->client_options.so_error = ECONNREFUSED;
      ix_wakeup((u_int)us);
    }

  close_stream(f, TRUE);
  close_stream(f, FALSE);

  if (us->server == f)
    us->server = NULL;

  f->f_sock = NULL;
  unix_socket_unref_locked(us);         /* file endpoint reference */

  ix_unlock_base();
  return 0;
}

