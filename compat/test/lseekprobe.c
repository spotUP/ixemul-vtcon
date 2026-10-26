/* lseekprobe: lseek() past EOF must not extend the file (POSIX); the next
 * write does. New file in T:, lseek 24, then:
 *   lseek     lseek(fd, 24, SEEK_SET) returns 24
 *   read      read() at 24 returns 0
 *   size0     fstat size is still 0
 *   tell      lseek(fd, 0, SEEK_CUR) is still 24
 *   write     write of 1 byte returns 1
 *   size25    fstat size is 25 and bytes 0..23 are zero, byte 24 is 'X'
 * Each check prints [OK] or [FAIL] and is appended to the log file named by
 * the first argument; the exit status is the number of failures.
 * Build: m68k-amigaos-gcc -mcrt=ixemul -O2 -Wall -o lseekprobe compat/test/lseekprobe.c
 * Old lib: size0 and read FAIL (size is 24 after the lseek). */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "T:lseekprobe.dat"
static const char *logname;
static int fails;

static void check(const char *name, int ok)
{
  FILE *l = logname ? fopen(logname, "a") : NULL;
  if (!ok) fails++;
  printf("%s %s\n", ok ? "[OK]" : "[FAIL]", name);
  if (l) { fprintf(l, "%s %s\n", ok ? "[OK]" : "[FAIL]", name); fclose(l); }
}

int main(int argc, char **argv)
{
  struct stat st;
  char buf[32];
  int fd, i, zeros = 1;

  logname = argc > 1 ? argv[1] : NULL;
  unlink(FILE_NAME);
  fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) { check("open", 0); return 1; }
  check("lseek", lseek(fd, 24, SEEK_SET) == 24);
  check("read", read(fd, buf, sizeof buf) == 0);
  check("size0", fstat(fd, &st) == 0 && st.st_size == 0);
  check("tell", lseek(fd, 0, SEEK_CUR) == 24);
  check("write", write(fd, "X", 1) == 1);
  check("size25", fstat(fd, &st) == 0 && st.st_size == 25);
  lseek(fd, 0, SEEK_SET);
  memset(buf, 1, sizeof buf);
  if (read(fd, buf, 25) != 25) zeros = 0;
  for (i = 0; i < 24; i++) if (buf[i] != 0) zeros = 0;
  check("content", zeros && buf[24] == 'X');
  close(fd);
  unlink(FILE_NAME);
  return fails;
}
