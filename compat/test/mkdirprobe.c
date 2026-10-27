/* mkdirprobe: mkdir() of an existing path is EEXIST (POSIX), including volume
 * roots, which the handler answers with an error that mapped to EIO. Checks:
 *   root      mkdir("RAM:") is -1/EEXIST
 *   unixroot  mkdir("/RAM") is -1/EEXIST
 *   slash     mkdir("/") is -1/EEXIST
 *   create    mkdir("T:mkdirprobe.d") succeeds (success path unchanged)
 *   drawer    mkdir of that existing sub-drawer is -1/EEXIST
 *   subunix   same drawer via "/T/mkdirprobe.d" is -1/EEXIST
 *   noparent  mkdir("T:mkdirprobe.nope/x") still fails with ENOENT, not EEXIST
 * Each check prints [OK] or [FAIL] and is appended to the log file named by
 * the first argument; the exit status is the number of failures.
 * Build: m68k-amigaos-gcc -mcrt=ixemul -O2 -Wall -o mkdirprobe compat/test/mkdirprobe.c
 * Old lib: root, unixroot and slash FAIL (errno is EIO). */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define DIR_NAME "T:mkdirprobe.d"
static const char *logname;
static int fails;

static void check(const char *name, int ok)
{
  FILE *l = logname ? fopen(logname, "a") : NULL;
  if (!ok) fails++;
  printf("%s %s\n", ok ? "[OK]" : "[FAIL]", name);
  if (l) { fprintf(l, "%s %s\n", ok ? "[OK]" : "[FAIL]", name); fclose(l); }
}

static int exists(const char *path)
{
  errno = 0;
  return mkdir(path, 0755) == -1 && errno == EEXIST;
}

int main(int argc, char **argv)
{
  logname = argc > 1 ? argv[1] : NULL;
  rmdir(DIR_NAME);
  check("root", exists("RAM:"));
  check("unixroot", exists("/RAM"));
  check("slash", exists("/"));
  check("create", mkdir(DIR_NAME, 0755) == 0);
  check("drawer", exists(DIR_NAME));
  check("subunix", exists("/T/mkdirprobe.d"));
  errno = 0;
  check("noparent", mkdir("T:mkdirprobe.nope/x", 0755) == -1 && errno == ENOENT);
  rmdir(DIR_NAME);
  return fails;
}
