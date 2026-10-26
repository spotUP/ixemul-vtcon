/* lseekprobe: lseek() past EOF must not extend the file (POSIX); the next
 * write does. New file in T:, lseek 24, then:
 *   lseek     lseek(fd, 24, SEEK_SET) returns 24
 *   read      read() at 24 returns 0
 *   size0     fstat size is still 0
 *   tell      lseek(fd, 0, SEEK_CUR) is still 24
 *   write     write of 1 byte returns 1
 *   size25    fstat size is 25 and bytes 0..23 are zero, byte 24 is 'X'
 *   trunc     ftruncate(fd, 40) after lseek 30 past EOF: offset stays 30,
 *             the next write lands at 30, size stays 40
 *   append    O_APPEND file, lseek 24 past EOF, write 1 byte: size is 1
 *             (append writes at EOF and leaves no gap)
 * Each check prints [OK] or [FAIL] and is appended to the log file named by
 * the first argument; the exit status is the number of failures.
 * Build: m68k-amigaos-gcc -mcrt=ixemul -O2 -Wall -o lseekprobe compat/test/lseekprobe.c
 * Old lib: size0 FAILS (size is 24 after the lseek); read passes there too,
 * because the old lib had already moved EOF to the offset. */
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
  fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) { check("open2", 0); return fails + 1; }
  lseek(fd, 30, SEEK_SET);
  check("trunc", ftruncate(fd, 40) == 0 && lseek(fd, 0, SEEK_CUR) == 30
        && write(fd, "Y", 1) == 1 && fstat(fd, &st) == 0 && st.st_size == 40
        && lseek(fd, 30, SEEK_SET) == 30 && read(fd, buf, 1) == 1 && buf[0] == 'Y');
  close(fd);

  unlink(FILE_NAME);
  fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0644);
  if (fd < 0) { check("open3", 0); return fails + 1; }
  lseek(fd, 24, SEEK_SET);
  check("append", write(fd, "Z", 1) == 1 && fstat(fd, &st) == 0 && st.st_size == 1);
  close(fd);
  unlink(FILE_NAME);
  return fails;
}
