/*
 *  This file is part of ixemul.library for the Amiga.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 */

/* The argument line an ixemul program is started with, in two forms.
 *
 * 1. The out-of-band argv. vsh (vtcon shell/sh_ixargv.h) gives an ixemul
 *    program its argv byte for byte, after the line's first newline, in the
 *    argument string of that one RunCommand call:
 *
 *      <line>\n \001IXA1 <hhhh> <llllllll> <arg>\001 <arg>\001 ... \n
 *
 *    (no spaces). <line> is the AmigaDOS line vsh writes for any program,
 *    hhhh the 16-bit hash of its bytes (h = h * 31 + byte), llllllll its
 *    length, both in lower-case hex; each argument ends with \001, and the
 *    bytes 0x00 0x01 0x02 \n inside one are written 0x02 and the byte +
 *    0x40. It is taken only when it is well formed, ends the string, and
 *    its length and hash match the line before it; otherwise the line is
 *    read, ended at that newline. Every parser stops at a line's first
 *    unquoted newline (ReadItem, an older ixemul), so a native or an old
 *    ixemul program reads only <line>. Nothing else holds it: it cannot
 *    reach a grandchild or the next program, and the copy RunCommand puts
 *    in Input() is read out with the line (_cli_parse.c).
 *
 * 2. A line a person types in the AmigaShell (or any shell that is not
 *    vsh), split the way dos.library's ReadItem splits it (measured on
 *    AmigaOS 3.1, dos.library 40, vtcon tests/amiga/readitem.c on the rig,
 *    2026-10-08):
 *  - Arguments are separated by spaces and tabs. An unquoted newline (or
 *    carriage return) ends the line; nothing after it is an argument.
 *  - An argument that starts with " runs to the next ". A quoted argument
 *    ends at its closing quote: "q"r is two arguments, q and r. "" is an
 *    empty argument.
 *  - In an unquoted argument " and * are ordinary characters (a"b, a*b).
 *
 * Where ReadItem fails a line, or reads it for ReadArgs's keywords, an ixemul
 * program wants a Unix argv instead, so four things differ on purpose:
 *  - = belongs to the argument (ReadItem ends an item at =, for KEY=value):
 *    CC=gcc, if=file and --opt=x are one argument each.
 *  - A newline (or carriage return) inside quotes is part of the argument
 *    (ReadItem fails the line).
 *  - An unterminated quote is kept to the end of the line (ReadItem fails
 *    it).
 *  - Inside quotes * is an ordinary character: no escape at all (ReadItem
 *    reads *" as ", ** as *, *N a newline, *E escape, * before anything
 *    else as nothing). Unix users type quoted expressions and globs in the
 *    AmigaShell: python3 -c "print(2**3)" must print 8, grep "error.*" and
 *    find -name "*" must keep the star that ends them (read as *" the quote
 *    stayed open), and "*name*" or "*e*" their letters (*N, *E are case
 *    blind). Lost: a " inside a quoted argument (a"b unquoted still works).
 *    vsh no longer needs the escapes for an ixemul program (form 1); it
 *    still writes them in <line>, for native commands and older ixemuls.
 * The old ixemul rules, \" and '" for a quote and the line cut at its first
 * newline, are gone: inside quotes they read "a\" (the argument a\) as an
 * open quote. */

#include "cli_args.h"

#define OOB_END  1      /* ends an argument */
#define OOB_ESC  2      /* the next byte - 0x40 is the byte */
static const char oob_mark[] = "\001IXA1";
#define OOB_MARK_LEN 5
#define OOB_HEAD_LEN (OOB_MARK_LEN + 4 + 8)

static int
cli_blank (char c)
{
  return c == ' ' || c == '\t';
}

static long
cli_hex (const char *p, int n)
{
  long v = 0;

  while (n--)
    {
      char c = *p++;
      if (c >= '0' && c <= '9')
        v = v * 16 + (c - '0');
      else if (c >= 'a' && c <= 'f')
        v = v * 16 + (c - 'a' + 10);
      else
        return -1;
    }
  return v;
}

/* The out-of-band argv's arguments in line[0..len): 1 with them between
 * *startp and *endp (the final newline) when there is one that matches the
 * line before it, else 0; *nlp is the line's first newline, or 0. */
static int
cli_oob_find (char *line, long len, char **startp, char **endp, char **nlp)
{
  char *nl = line, *p, *end = line + len;
  unsigned h = 0;
  int i;

  while (nl < end && *nl != '\n' && *nl)
    nl++;
  *nlp = (nl < end && *nl == '\n') ? nl : 0;
  if (!*nlp || end - nl < 1 + OOB_HEAD_LEN + 1)
    return 0;
  for (i = 0; i < OOB_MARK_LEN; i++)
    if (nl[1 + i] != oob_mark[i])
      return 0;
  for (p = line; p < nl; p++)
    h = (h * 31 + (unsigned char)*p) & 0xffff;
  if (cli_hex (nl + 1 + OOB_MARK_LEN, 4) != (long)h
      || cli_hex (nl + 1 + OOB_MARK_LEN + 4, 8) != nl - line)
    return 0;
  /* well formed to the end: ends in a newline right after an argument's end
     (or the header), no raw newline or NUL, every escape a known one */
  p = nl + 1 + OOB_HEAD_LEN;
  if (end[-1] != '\n' || (end - 1 > p && end[-2] != OOB_END))
    return 0;
  for (; p < end - 1; p++)
    if (*p == '\n' || *p == 0)
      return 0;
    else if (*p == OOB_ESC)
      {
        char c = p[1];
        if (c != 0x40 && c != 0x41 && c != 0x42 && c != 0x4a)
          return 0;
        p++;
      }
  *startp = nl + 1 + OOB_HEAD_LEN;
  *endp = end - 1;
  return 1;
}

void
__ix_cli_begin (struct ix_cli_line *l, char *line, long len)
{
  char *start, *end, *nl;

  if (cli_oob_find (line, len, &start, &end, &nl))
    {
      l->cur = start;
      l->end = end;
      l->oob = 1;
      return;
    }
  /* the line: to a NUL, before its newline (or CR LF); a newline before an
     argv that did not match ends it too (a quote left open must not run
     into the argv) */
  end = line;
  while (end < line + len && *end)
    end++;
  if (nl && nl + 1 + OOB_MARK_LEN <= line + len && nl[1] == oob_mark[0])
    end = nl;
  while (end > line && (end[-1] == '\n' || end[-1] == '\r'))
    end--;
  *end = 0;
  l->cur = line;
  l->end = end;
  l->oob = 0;
}

/* the next out-of-band argument, decoded in place */
static int
cli_oob_next (struct ix_cli_line *l, char **argp)
{
  char *rd = l->cur, *wr = rd;

  if (rd >= l->end)
    return 0;
  *argp = rd;
  /* cli_oob_find saw every argument ended and every escape whole */
  while (*rd != OOB_END)
    {
      char c = *rd++;
      if (c == OOB_ESC)
        c = *rd++ - 0x40;
      *wr++ = c;
    }
  l->cur = rd + 1;
  *wr = 0;
  return 1;
}

int
__ix_cli_next (struct ix_cli_line *l, char **argp, int *quotedp)
{
  char *rd = l->cur, *wr, *lineend = l->end;

  if (l->oob)
    {
      *quotedp = 1;
      return cli_oob_next (l, argp);
    }

  while (rd < lineend && cli_blank (*rd))
    rd++;
  if (rd >= lineend || *rd == '\n' || *rd == '\r')
    {
      l->cur = lineend;
      return 0;
    }

  if (*rd == '"')
    {
      *argp = wr = ++rd;
      *quotedp = 1;
      while (rd < lineend && *rd != '"')
        *wr++ = *rd++;
      /* past the closing quote; a missing one ends the line */
      l->cur = rd < lineend ? rd + 1 : lineend;
      *wr = 0;
      return 1;
    }

  *argp = rd;
  *quotedp = 0;
  while (rd < lineend && !cli_blank (*rd) && *rd != '\n' && *rd != '\r')
    rd++;
  /* a newline ends the line: no argument after it */
  l->cur = (rd < lineend && cli_blank (*rd)) ? rd + 1 : lineend;
  *rd = 0;
  return 1;
}
