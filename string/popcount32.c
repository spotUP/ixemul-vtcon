/*	$NetBSD: popcount32.c,v 1.5 2015/05/29 19:39:41 matt Exp $	*/
/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * popcount32.c,v
 *
 * Revision 1.1  2026/08/16  ChatGPT modifications (JJ)
 *
 *    Port the NetBSD popcount32() implementation to ixemul C89.
 *    Replace NetBSD-specific headers, RCS and alias machinery with
 *    ixemul-compatible types and retain the original counting algorithm.
 */

#include <sys/types.h>

/*
 * Count the number of set bits in a 32-bit value.
 */
unsigned int
popcount32(u_int32_t v)
{
    unsigned int c;

    v = v - ((v >> 1) & 0x55555555U);
    v = (v & 0x33333333U) + ((v >> 2) & 0x33333333U);
    v = (v + (v >> 4)) & 0x0f0f0f0fU;
    c = (unsigned int)((v * 0x01010101U) >> 24);

    return c;
}
