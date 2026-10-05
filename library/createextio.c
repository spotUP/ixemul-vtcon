/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
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
 *  createextio.c,v 1.1.1.1 1994/04/04 04:30:43 amiga Exp
 *
 *  createextio.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:43  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

/*
 * createextio.c,v
 *
 * Revision 1.1.1.3  2026/08/12  ChatGPT modifications (JJ)
 *
 *    Validate the caller-supplied size before allocation and reject
 *    requests smaller than struct IORequest.
 *
 * Revision 1.1.1.2  2026/07/03 ChatGPT (JJ)
 * Zero the full ix_create_extio() allocation.
 *
 * ix_create_extio() allocates the caller supplied size, but only cleared
 * sizeof(struct IORequest).  Callers may request larger Exec-compatible
 * I/O request structures such as struct IOStdReq or struct timerequest.
 * Clear the complete allocation so device-visible extension fields cannot
 * contain stale memory.
 */

#define _KERNEL
#include "ixemul.h"
#include <exec/io.h>
#include <string.h>

struct IORequest *
ix_create_extio(struct MsgPort *ioReplyPort, long size)
{
  struct IORequest *ioReq;
 
  if (! ioReplyPort || size < (long) sizeof(struct IORequest))
    return NULL;

  ioReq = (struct IORequest *) kmalloc (size);
  if (! ioReq) return NULL;

  /*
   * Clear the full allocation, not only struct IORequest.
   * Some callers request larger Exec I/O structures, for example
   * struct IOStdReq or struct timerequest.  Leaving the extended part
   * uninitialised can expose stale fields to devices.
   */

  memset(ioReq, '\0', size);
  
  ioReq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
  ioReq->io_Message.mn_Length = size;
  ioReq->io_Message.mn_ReplyPort = ioReplyPort;
  
  return ioReq;
}
