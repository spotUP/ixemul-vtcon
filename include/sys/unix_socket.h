/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996 Hans Verkuil
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
 * Revision 1.2  2026-08-01  ChatGPT modification  (JJ)
 *
 * Add a reference count to sock_stream.  One reference belongs to the
 * owning pipe or unix_socket; active stream operations hold temporary
 * references so close cannot free a stream used as an I/O or sleep object.
 *
 * Revision 1.1  2026-08-01  ChatGPT modification  (JJ)
 *
 * Add reference counts for AF_UNIX socket/name lifetime, separate client
 * and server pathname state, a queued-listener link, and per-endpoint
 * socket-option storage.
 */

#ifndef _UNIX_SOCKET_H_
#define _UNIX_SOCKET_H_

#include <sys/types.h>
#include <sys/time.h>

struct unix_socket;
struct file;
struct Task;

#define UNIX_SOCKET_SIZE 5120

/* descriptors in flight (sendmsg SCM_RIGHTS), taken by the next recvmsg */
#define UNIX_SOCKET_RIGHTS 8

struct sock_stream {
  char  buffer[UNIX_SOCKET_SIZE];
  char  *reader, *writer;
  short flags;
  struct Task *task;             /* waiting to read (in select) */
  struct Task *wtask;            /* waiting to write: a reader and a writer in
                                    two processes wait on one stream, and one
                                    slot for both let each take the other's
                                    wake-up -- tmux's client slept for ever
                                    (UP-Term) */

  /* descriptors passed with sendmsg (SCM_RIGHTS), each holding a reference
     to its struct file until a recvmsg takes it or the stream is freed;
     the bytes that went through, and where in them each descriptor's
     message starts: a recvmsg reads up to the next one and delivers the
     descriptors whose message it began (as BSD keeps records; UP-Term) */
  struct file *rights[UNIX_SOCKET_RIGHTS];
  short nrights;
  u_long written, readn;
  u_long right_at[UNIX_SOCKET_RIGHTS];

  /* One owner reference plus one reference per active operation. */
  int refs;
};

struct unix_socket_options {
  int so_error;
  int so_rcvbuf;
  int so_sndbuf;
  struct timeval so_rcvtimeo;
  struct timeval so_sndtimeo;
  int so_type;
};

struct ix_unix_name {
  struct ix_unix_name *next;
  char          path[104];
  int           queue_size;
  int           queue_index;
  int           queue_head;
  int           queue_tail;
  struct unix_socket **queue;
  struct Task   *task;

  /* Protected by ix_lock_base(). */
  int           refs;
  int           closing;
};

struct unix_socket {
  /*
   * A connected pair shares this object.  client_path belongs to the
   * connecting endpoint; server_path belongs to the accepted endpoint.
   * A listening socket uses client_path as its local pathname.
   */
  char          client_path[104];
  char          server_path[104];

  short         state;
  struct sock_stream *from_server;
  struct sock_stream *to_server;

  struct ix_unix_name *unix_name;     /* listener registration */
  struct ix_unix_name *connect_name;  /* queue membership while connecting */
  struct file   *server;              /* accepted server endpoint */

  /* Protected by ix_lock_base(). */
  int           refs;

  struct unix_socket_options client_options;
  struct unix_socket_options server_options;
};

#define UNF_NO_READER  (1<<0)
#define UNF_NO_WRITER  (1<<1)
#define UNF_LOCKED     (1<<2)
#define UNF_WANT_LOCK  (1<<3)

#define UNS_WAITING     (0)
#define UNS_ERROR       (-1)
#define UNS_PROCESSING  (1)
#define UNS_ACCEPTED    (2)

#endif /* _UNIX_SOCKET_H_ */
