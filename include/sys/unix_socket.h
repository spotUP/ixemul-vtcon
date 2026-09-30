#ifndef _UNIX_SOCKET_H_
#define _UNIX_SOCKET_H_

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
  struct file *rights[UNIX_SOCKET_RIGHTS];
  short nrights;
  /* bytes that went through, and where in them each passed descriptor's
     message starts: a recvmsg reads up to the next one and delivers the
     descriptors whose message it began (as BSD keeps records; UP-Term) */
  u_long written, readn;
  u_long right_at[UNIX_SOCKET_RIGHTS];
};

struct ix_unix_name {
  struct ix_unix_name *next;
  char          path[104];       /* size of struct sockaddr_un.sun_path */
  int           queue_size;
  int           queue_index;
  int           *queue;
  struct Task   *task;
};

struct unix_socket {
  char          path[104];       /* size of struct sockaddr_un.sun_path */
  short         state;
  struct sock_stream *from_server;
  struct sock_stream *to_server;
  struct ix_unix_name *unix_name;
  struct file   *server;
};

#define UNF_NO_READER	(1<<0)
#define UNF_NO_WRITER	(1<<1)
#define UNF_LOCKED	(1<<2)
#define UNF_WANT_LOCK	(1<<3)

/* waiting for accept() */
#define UNS_WAITING     (0)
/* accept() failed */
#define UNS_ERROR       (-1)
/* accept() is processing the request, continue sleeping */
#define UNS_PROCESSING  (1)
/* accept() accepted the request */
#define UNS_ACCEPTED    (2)

#endif
