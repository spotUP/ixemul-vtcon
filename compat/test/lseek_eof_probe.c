/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* lseek() past EOF changes the offset, never the size (POSIX); a later
 * write() fills the gap with zeros. ixemul used to Write() the zeros inside
 * lseek(), so sqlite3's lseek(24) + read() on a new database left a 24-byte
 * zero file and every later open said "file is not a database".
 * Build: m68k-amigaos-gcc -O1 -m68020 -o lseek_eof_probe lseek_eof_probe.c
 * Run on the Amiga: lseek_eof_probe <path of a scratch file>, on RAM: and on
 * a disk volume. Prints one line per check; exit status = failures. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static int fails;

static void
check (const char *what, long got, long want)
{
  printf ("%s %s: %ld (want %ld)\n", got == want ? "[OK]" : "[FAIL]", what,
          got, want);
  if (got != want)
    fails++;
}

static long
size_of (int fd)
{
  struct stat st;
  return fstat (fd, &st) == 0 ? (long) st.st_size : -1L;
}

int
main (int argc, char **argv)
{
  const char *p = argc > 1 ? argv[1] : "lseek_eof_probe.tmp";
  char buf[64], big[4100];
  int fd, i, zeros;

  unlink (p);

  /* sqlite3's first open of an empty database */
  fd = open (p, O_RDWR | O_CREAT, 0644);
  check ("open", fd >= 0, 1);
  if (fd < 0)
    return 1;
  check ("lseek 24", (long) lseek (fd, 24, SEEK_SET), 24);
  check ("SEEK_CUR after pending lseek", (long) lseek (fd, 0, SEEK_CUR), 24);
  check ("SEEK_CUR +6 from pending", (long) lseek (fd, 6, SEEK_CUR), 30);
  check ("SEEK_CUR -6 back", (long) lseek (fd, -6, SEEK_CUR), 24);
  check ("read at 24 is EOF", (long) read (fd, buf, sizeof buf), 0);
  check ("fstat size after lseek+read", size_of (fd), 0);
  check ("SEEK_END", (long) lseek (fd, 0, SEEK_END), 0);
  close (fd);

  fd = open (p, O_RDWR);
  check ("reopen", fd >= 0, 1);
  if (fd < 0)
    return 1;
  check ("size after reopen", size_of (fd), 0);

  /* the gap is filled by the write that lands past EOF */
  check ("lseek 4096", (long) lseek (fd, 4096, SEEK_SET), 4096);
  check ("size before write", size_of (fd), 0);
  check ("write 4 at 4096", (long) write (fd, "ABCD", 4), 4);
  check ("SEEK_CUR after write", (long) lseek (fd, 0, SEEK_CUR), 4100);
  check ("size after write", size_of (fd), 4100);
  close (fd);

  fd = open (p, O_RDONLY);
  memset (big, 0x55, sizeof big);
  check ("read back 4100", (long) read (fd, big, sizeof big), 4100);
  for (zeros = 1, i = 0; i < 4096; i++)
    if (big[i])
      zeros = 0;
  check ("bytes 0..4095 are zero", zeros, 1);
  check ("bytes 4096..4099", memcmp (big + 4096, "ABCD", 4) == 0, 1);
  close (fd);

  /* a pending position the file grows past is a plain position again */
  fd = open (p, O_RDWR);
  check ("lseek 5000", (long) lseek (fd, 5000, SEEK_SET), 5000);
  check ("lseek back to 10", (long) lseek (fd, 10, SEEK_SET), 10);
  check ("read inside file", (long) read (fd, buf, 4), 4);
  check ("SEEK_CUR inside file", (long) lseek (fd, 0, SEEK_CUR), 14);
  check ("size unchanged", size_of (fd), 4100);
  close (fd);

  unlink (p);
  printf ("%s: %d failure(s)\n", fails ? "FAIL" : "PASS", fails);
  return fails;
}
