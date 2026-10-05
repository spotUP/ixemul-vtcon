/*
 * Revision 1.3.1  2026/04/29  JJ, Copilot
 *  - Added MAP_FAILED fallback definition for ixemul builds.
 *  - Replaced multi-character constants 'StCk'/'sTcK' with 32-bit literals
 *    to ensure C89 portability and eliminate compiler warnings.
 *
 * Revision 1.3  2026/04/06  JJ, Copilot
 *  - Fixed uninitialized fd usage in the stat() failure path.
 *  - Corrected mmap() error handling (test addr == MAP_FAILED).
 *  - Enabled writable mmap() (PROT_READ | PROT_WRITE) to match write()
 *    operations on the underlying file.
 *  - Ensured munmap() is called in all exit paths to avoid leaks.
 *  - Replaced long* magic reads with uint32_t for clarity; semantics unchanged.
 *  - Added <ctype.h> and corrected isdigit() usage with unsigned char cast.
 *  - Made main() explicitly return int.
 *  These changes remove undefined behaviour and resource leaks while preserving
 *  the original ixstack functionality and file format semantics.
 */

#define UTILITY_TAGITEM_H
#define _SIZE_T
#define __AMIGA_TYPES__

#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ixemul.h>
#include <ix.h>
#include <proto/exec.h>
#include <ctype.h>

#include <stdint.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

char VERSION[] = "\000$VER: ixstack 1.1 (14.06.97)";

void setstack(long size, char *filename)
{
  int fd;
  int len, i;
  caddr_t addr;
  struct stat s;
  static int printed_header = 0;
  
  if (stat(filename, &s) == -1 || !S_ISREG(s.st_mode))
    return;

  fd = open(filename, O_RDWR);
  if (fd == -1)
  {
    perror(filename);
    return;
  }

  len = lseek(fd, 0, SEEK_END);
  if (len <= 0)
  {
    close(fd);
    return;
  }

  addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
  {
    perror(filename);
    close(fd);
    return;
  }
  for (i = 0; i < len - 12; i += 2)
    if (*(uint32_t *)(addr + i) == 0x5374436B &&
        *(uint32_t *)(addr + i + 8) == 0x7354634B)
    {
      if (size >= 0)
      {
        lseek(fd, i + 4, SEEK_SET);
        write(fd, &size, 4);
      }
      else
      {
        if (!printed_header)
        {
          printed_header = 1;
          printf("stacksize  filename\n---------  --------\n");
        }
        printf("%9ld  %s\n", *(long *)(addr + i + 4), filename);
      }
      munmap(addr, len);
      close(fd);
      return;
    }
  munmap(addr, len);
  close(fd);
  if (size)
    printf("cannot set stack: %s\n", filename);
}

static int ctrlc = 0;

static void sigint()
{
  ctrlc = 1;
}

static void show(void)
{
  struct MsgPort *port;
  struct SUMessage *msg;
  u_long portsig;
  int printed_header = 0;
  
  signal(SIGINT, sigint);
  if ((port = CreatePort("ixstack port", 0)))
  {
    while (!ctrlc)
    {
      portsig = 1 << port->mp_SigBit | SIGBREAKF_CTRL_C;
      ix_wait(&portsig);
      if (portsig & (1 << port->mp_SigBit))
      {
        while ((msg = (struct SUMessage *)GetMsg(port)))
        {
          if (!printed_header)
          {
            printed_header = 1;
            printf("  Usage   Total Program\n\n");
          }
          printf("%7d %7d %s\n", msg->stack_usage, msg->stack_size, msg->name);
          ReplyMsg((struct Message *)msg);
        }
      }
      
      if (portsig & SIGBREAKF_CTRL_C)
      {
        while ((msg = (struct SUMessage *)GetMsg(port)))
          ReplyMsg((struct Message *)msg);
        break;
      }
    }
    DeletePort(port);
  }
  exit(0);
}


int main(int argc, char **argv)
{
  long size;

  if (argc == 2 && !strcmp(argv[1], "-s"))
    show();
  if (argc < 3)
  {
    fprintf(stderr, "set stacksize:   ixstack <stacksize> <files ...>\n"
                    "show stacksize:  ixstack -l <files ...>\n"
                    "show stackusage: ixstack -s\n");
    exit(1);
  }

  if (!strcmp(argv[1], "-l"))
    size = -1;
  else
  {
    int i;
    
    for (i = 0; argv[1][i]; i++)
      if (!isdigit((unsigned char)argv[1][i]))
      {
        fprintf(stderr, "stacksize %s is not a number\n", argv[1]);
        exit(1);
      }
    size = atol(argv[1]);
    if (size < 4000 && size)
    {
      fprintf(stderr, "stacksize must be at least 4000 bytes\n");
      exit(1);
    }
    if (size && (size & 3))
    {
      fprintf(stderr, "stacksize must be a multiple of 4\n");
      exit(1);
    }
  }
  argv += 2;
  while (*argv)
    setstack(size, *argv++);
}
