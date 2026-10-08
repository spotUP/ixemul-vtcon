/*
 *  This file is part of ixemul.library for the Amiga.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 */

/* An AmigaDOS argument line, split the way dos.library's ReadItem (and so
 * ReadArgs and every native command) splits it, measured on AmigaOS 3.1,
 * dos.library 40 (vtcon tests/amiga/readitem.c on the rig, 2026-10-08):
 *
 *  - Arguments are separated by spaces and tabs. An unquoted newline (or
 *    carriage return) ends the line; nothing after it is an argument.
 *  - An argument that starts with " runs to the next unescaped ". Inside
 *    it * is the escape character: *" is ", ** is *, *N and *n a newline,
 *    *E and *e escape (0x1b), and * before any other character is that
 *    character (ixemul keeps the star there, below). A quoted argument ends at its closing quote: "q"r is two
 *    arguments, q and r. "" is an empty argument.
 *  - In an unquoted argument " and * are ordinary characters (a"b, a*b).
 *
 * Where ReadItem fails a line, or reads it for ReadArgs's keywords, an ixemul
 * program wants a Unix argv instead, so four things differ on purpose:
 *  - = belongs to the argument (ReadItem ends an item at =, for KEY=value):
 *    CC=gcc, if=file and --opt=x are one argument each.
 *  - A newline (or carriage return) inside quotes is part of the argument
 *    (ReadItem fails the line): vsh before 4413259 wrote an argument
 *    holding a newline that way (it writes *N now), and the shells before
 *    it did too.
 *  - An unterminated quote, or a * at the end of the line, is kept to the
 *    end of the line (ReadItem fails it).
 *  - Inside quotes only *" ** *N *n *E *e are escapes; * before any other
 *    character is a literal star (ReadItem drops it). Unix users type quoted
 *    globs and expressions in the AmigaShell: python3 -c "print(6*7)" read
 *    as print(67), find . -name "*.c" as .c. The six escapes are all vsh
 *    writes (" * newline as *" ** *N) and all AmigaDOS documents, so a line
 *    meant for a native command still reads the same; "*exe" stays ESC xe
 *    and "*n" a newline, as ReadItem, and "*" is an unterminated quote.
 * The old ixemul rules, \" and '" for a quote and the line cut at its first
 * newline, are gone: inside quotes they read "a\" (the argument a\) as an
 * open quote, and they are not what a native command reads from the same
 * line, so a shell could not write one line for both. */

#include "cli_args.h"

static int
cli_blank (char c)
{
  return c == ' ' || c == '\t';
}

char *
__ix_cli_line_end (char *line, long len)
{
  char *end = line;

  while (end < line + len && *end)
    end++;
  while (end > line && (end[-1] == '\n' || end[-1] == '\r'))
    end--;
  return end;
}

int
__ix_cli_next_arg (char **linep, char *lineend, char **argp, int *quotedp)
{
  char *rd = *linep, *wr;

  while (rd < lineend && cli_blank (*rd))
    rd++;
  if (rd >= lineend || *rd == '\n' || *rd == '\r')
    {
      *linep = lineend;
      return 0;
    }

  if (*rd == '"')
    {
      *argp = wr = ++rd;
      *quotedp = 1;
      while (rd < lineend && *rd != '"')
        {
          char c = *rd++;

          /* only the escapes vsh writes and AmigaDOS documents; any
             other * is a literal star (see the top of the file) */
          if (c == '*' && rd < lineend)
            switch (*rd)
              {
              case '"': case '*':
                c = *rd++;
                break;
              case 'N': case 'n':
                rd++;
                c = '\n';
                break;
              case 'E': case 'e':
                rd++;
                c = 0x1b;
                break;
              }
          *wr++ = c;
        }
      /* past the closing quote; a missing one ends the line */
      *linep = rd < lineend ? rd + 1 : lineend;
      *wr = 0;
      return 1;
    }

  *argp = rd;
  *quotedp = 0;
  while (rd < lineend && !cli_blank (*rd) && *rd != '\n' && *rd != '\r')
    rd++;
  /* a newline ends the line: no argument after it */
  *linep = (rd < lineend && cli_blank (*rd)) ? rd + 1 : lineend;
  *rd = 0;
  return 1;
}
