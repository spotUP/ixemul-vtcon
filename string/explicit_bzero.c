/*
/*
 *  This file is part of ixemul.library for the Amiga.
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
 * Adapted for ixemul.library by JJ, with assistance from Copilot.
 * 
 * C89-compatible explicit memory clearing wrapper for ixemul.library.
 *
 * The actual clearing is performed by ixemul's existing bzero()
 * implementation, including any CPU-specific assembler optimization.
 *
 * explicit_bzero() is deliberately implemented as an out-of-line
 * library function so that callers cannot optimize away the memory
 * clearing as a dead store.
 *
 * Revision 1.2  2026/09/23  ChatGPT modifications (JJ)
 *
 *	Clarified the documentation describing why explicit_bzero()
 *	must remain an out-of-line function.
 *
 * 1.1  2026/07/13  Initial ixemul version.
 */

#include <string.h>

void
explicit_bzero(void *p, size_t n)
{
	if (n != 0)
		bzero(p, n);
}
