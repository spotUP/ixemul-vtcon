/*
 *  Written by Hans Verkuil (hans@wyst.hobby.nl)
 *
 *  battclock.resource patch idea was shamelessly stolen from the unixclock
 *  utility written by Geert Uytterhoeven. unixclock is available on Aminet.
 */

/*
 * Revision 1.1.3  2026/09/19  ChatGPT modifications (JJ)
 *
 *    Treat only positive tm_isdst values as active daylight saving time.
 *
 *    Check creation, writing and closing of IXGMTOFFSET files and return
 *    a failure status when ENV: or ENVARC: cannot be updated completely.
 */

 /*

 * Revision 1.1.2  2026/04/29  JJ
 *  - Corrected BattClockBase cast: OpenResource() result is now cast to
 *    struct Node* instead of struct Library* to match battclock.resource
 *    base type and eliminate incompatible pointer warnings.
 * 
 * Revision 1.1.1  2026/02/18  JJ, Copilot
 *  - Modernized and secured IXGMTOFFSET handling: explicit endian stable
 *    encoding/decoding, removed struct-padding assumptions.
 *  - Replaced hardcoded epoch arithmetic with AMIGA_UNIX_EPOCH constant.
 *  - Added NULL safety for gmtime()/localtime() and clarified DST flag logic.
 *  - Replaced Disable()/Enable() with Forbid()/Permit() for safe patching of
 *    battclock.resource without disabling interrupts.
 *  - Corrected pointer arithmetic and size calculations for patch block.
 *  - Added sanity checks for AllocMem() and improved FreeMem() correctness.
 *  - Cleaned up includes, types, and return paths; main() now returns int.
 *  These changes improve robustness and eliminate undefined behaviour while
 *  preserving the original ixtimezone semantics and on system behaviour.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <ixemul.h>
#include <exec/memory.h>
#include <dos/var.h>

#include <sys/time.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

/*
 * Amiga battclock epoch (1978-01-01) to Unix epoch (1970-01-01)
 * in seconds: 8 years including 2 leap years.
 */
#define AMIGA_UNIX_EPOCH 252460800L

#define LVOReadBattClock	(-12)
#define LVOWriteBattClock	(-18)

/* Flags. Currently only bit 0 is used to tell whether Daylight Saving Time
   is in effect or not. However, ixtimezone only sets this flag, but it doesn't
   test it. And neither does ixemul.library. In fact, the library completely
   ignores the flag field. */

#define DST_ON	0x01

typedef struct {
  long          offset; /* seconds east of GMT (negative for west) */
  unsigned char flags;  /* DST_ON etc. */
} ixtime;

struct Node *BattClockBase = NULL;

char VERSION[] = "\000$VER: ixtimezone 1.0 (10.11.95)";

/* battclock.resource patch code */
asm("
	.text
	.globl _NewReadBattClock
	.globl _NewWriteBattClock
	.globl _OldReadBattClock
	.globl _OldWriteBattClock
	.globl _GMTOffset
	.globl _EndOfPatch

_NewReadBattClock:
	.int	0x207a0014, 0x4e9090ba, 0x00164e75

/*	Since the GNU assembler is currently unable to properly compile
	PC-relative code, I'm using the hex-code directly. As soon as the
	assembler can handle PC-relative code, the line above should be
	replaced by:

	move.l		_OldReadBattClock(pc),a0
	jsr		(a0)
	sub.l		_GMTOffset(pc),d0
	rts
*/

_NewWriteBattClock:
	.int	0xd0ba0010, 0x207a0008
	.short	0x4ed0

/*	Since the GNU assembler is currently unable to properly compile
	PC-relative code, I'm using the hex-code directly. As soon as the
	assembler can handle PC-relative code, the line above should be
	replaced by:

	add.l		_GMTOffset(pc),d0
	move.l		_OldWriteBattClock(pc),a0
	jmp		(a0)
*/

_OldReadBattClock:
	.int		0
_OldWriteBattClock:
	.int		0
_GMTOffset:
	.int		0
_EndOfPatch:
");

extern char NewReadBattClock;
extern char NewWriteBattClock;
extern char OldReadBattClock;
extern char OldWriteBattClock;
extern long GMTOffset; 
extern long EndOfPatch;

static ixtime *read_ixtime(void)
{
  static ixtime t;
  unsigned char buf[5];
  
  /*
   * Read the GMT offset. This environment variable is 5 bytes long. The
   * first 4 form a long that contains the offset in seconds and the fifth
   * byte contains flags.
   */
  if (GetVar("IXGMTOFFSET", (char *)buf, sizeof(buf), GVF_BINARY_VAR) == 5)
  {
    /* explicit, padding-safe, endian-stable decode */
    t.offset  = (long)(((long)buf[0] << 24) |
                       ((long)buf[1] << 16) |
                       ((long)buf[2] <<  8) |
                       ((long)buf[3]      ));
    t.flags = buf[4];
    return &t;
  }
  return NULL;
}

static int create_ixtime(ixtime *t, char *pathname)
{
  FILE *f = fopen(pathname, "w");
  
  if (f)
  {
    unsigned char buf[5];

    /* explicit, endian-stable encoding of offset + flags */
    buf[0] = (unsigned char)((t->offset >> 24) & 0xff);
    buf[1] = (unsigned char)((t->offset >> 16) & 0xff);
    buf[2] = (unsigned char)((t->offset >>  8) & 0xff);
    buf[3] = (unsigned char)((t->offset      ) & 0xff);
    buf[4] = t->flags;

    if (fwrite(buf, sizeof(buf), 1, f) != 1)
    {
      perror(pathname);
      fclose(f);
      return -1;
    }
    if (fclose(f) != 0)
    {
      perror(pathname);
      return -1;
    }
    return 0;
  }

  perror(pathname);
  return -1;
}

static int write_ixtime(ixtime *t, int write_also_to_envarc)
{
  int status = 0;

  ix_set_gmt_offset(t->offset);
  if (create_ixtime(t, "/ENV/IXGMTOFFSET") != 0)
    status = -1;
  if (write_also_to_envarc)
    if (create_ixtime(t, "/ENVARC/IXGMTOFFSET") != 0)
      status = -1;

  return status;
}

static void set_clock(long offset)
{
  struct timeval tv;
  
  gettimeofday(&tv, NULL);
  tv.tv_sec += offset;
  settimeofday(&tv, NULL);
}

static void reset_clock(void)
{
  struct timeval tv;
  
  tv.tv_usec = 0;
  tv.tv_sec = ReadBattClock() + AMIGA_UNIX_EPOCH + ix_get_gmt_offset();
  settimeofday(&tv, NULL);
}

static long *get_function_addr(void)
{
  return *((long **)((unsigned char *)BattClockBase + LVOReadBattClock + 2));
}

static void patch_batt_resource(long offset)
{
  char *mem;
  long size = (long)((char *)&EndOfPatch - (char *)&NewReadBattClock);
  long mem_offset;
  long *oldread, *oldwrite, *gmt;

  BattClockBase = (struct Node *)OpenResource("battclock.resource");
  if (!BattClockBase)
    return;
  if (*(gmt = get_function_addr()) == 0x207a0014)
  {
    printf("battclock.resource was already patched.\n");
    gmt = (long *)(((char *)&GMTOffset) + (long)((char *)gmt - &NewReadBattClock));
    if (*gmt != offset)
    {
      *gmt = offset;
      reset_clock();
    }
    return;  /* already patched */
  }
  GMTOffset = offset;
  mem = AllocMem(size, MEMF_PUBLIC);
  if (!mem)
    return;
  mem_offset = (long)((char *)mem - (char *)&NewReadBattClock);
  memcpy(mem, &NewReadBattClock, size);
  CacheClearE(mem, size, CACRF_ClearI);  /* clear instruction cache */
  oldread = (long *)(&OldReadBattClock + mem_offset);
  oldwrite = (long *)(&OldWriteBattClock + mem_offset);
  Forbid();
  *oldread = (long)SetFunction((struct Library *)BattClockBase, LVOReadBattClock, (void *)(&NewReadBattClock + mem_offset));
  *oldwrite = (long)SetFunction((struct Library *)BattClockBase, LVOWriteBattClock, (void *)(&NewWriteBattClock + mem_offset));
  Permit();
  reset_clock();
  printf("patched battclock.resource.\n");
}

static void remove_patch(void)
{
  long *p, mem_offset, *oldread, *oldwrite;
  long size = (long)((char *)&EndOfPatch - (char *)&NewReadBattClock);

  BattClockBase = (struct Node *)OpenResource("battclock.resource");
  if (!BattClockBase)
    exit(0);
  if (*(p = get_function_addr()) != 0x207a0014)
  {
    printf("battclock.resource wasn't patched.\n");
    exit(0);  /* not patched */
  }
  mem_offset = (long)((char *)p - (char *)&NewReadBattClock);
  oldread = (long *)(&OldReadBattClock + mem_offset);
  oldwrite = (long *)(&OldWriteBattClock + mem_offset);
  Forbid();
  SetFunction((struct Library *)BattClockBase, LVOReadBattClock, (void *)*oldread);
  SetFunction((struct Library *)BattClockBase, LVOWriteBattClock, (void *)*oldwrite);
  Permit();
  FreeMem(p, (ULONG)size);
  reset_clock();
  printf("removed battclock.resource patch.\n");
  exit(0);
}

static void test(void)
{
  time_t t;
  
  time(&t);
  printf("GMT:   %s", asctime(gmtime(&t)));
  printf("Local: %s", asctime(localtime(&t)));
  exit(0);
}

static void usage(void)
{
  fprintf(stderr, "Usage: ixtimezone <option>
Where <option> is one of:

-test		Show GMT and localtime
-get-offset	Get GMT offset and patch ixemul.library
-check-dst	As -get-offset, but also automatically adjust the Amiga
		time if Daylight Saving Time has gone in effect (or vice
		versa)
-patch-resource	As -get-offset, but also patch the battclock.resource
-remove-patch	Remove the battclock.resource patch\n");
  exit(1);
}

int main(int argc, char **argv)
{
  struct tm *local_tm, *gmt_tm;
  int local_hms, gmt_hms;
  time_t t, local_t, gmt_t;
  ixtime *old, new;
  int set_the_clock = 0, patch_resource = 0;
  int write_to_envarc = 0;
  int status;

  if (argc != 2)
    usage();

  if (!strcmp(argv[1], "-test"))
    test();
  else if (!strcmp(argv[1], "-check-dst"))
    set_the_clock = 1;
  else if (!strcmp(argv[1], "-patch-resource"))
    patch_resource = 1;
  else if (!strcmp(argv[1], "-remove-patch"))
    remove_patch();
  else if (strcmp(argv[1], "-get-offset"))
    usage();

  /*
   * Get current time, both GMT and local.  
   * We don't care if these values are correct or not, we are only interested
   * in the difference between the two.
   */

  time(&t);
  gmt_tm = gmtime(&t);
  if (!gmt_tm)
    usage();
  gmt_hms = gmt_tm->tm_hour * 3600 + gmt_tm->tm_min * 60 + gmt_tm->tm_sec;
  local_tm = localtime(&t);
  if (!local_tm)
    usage();
  local_hms = local_tm->tm_hour * 3600 + local_tm->tm_min * 60 + local_tm->tm_sec;
  new.flags = (local_tm->tm_isdst > 0 ? DST_ON : 0);
  new.offset = 0;
  if (gmt_hms != local_hms)
  {
    /* They are not the same. So compute the difference between them */

    local_tm->tm_isdst = 0;     /* don't let these values influence the result! */
    local_tm->tm_zone = NULL;
    local_tm->tm_gmtoff = 0;
    local_t = mktime(local_tm);
    gmt_tm = gmtime(&t);
    gmt_tm->tm_isdst = 0;
    gmt_tm->tm_zone = NULL;
    gmt_tm->tm_gmtoff = 0;
    gmt_t = mktime(gmt_tm);
    new.offset = gmt_t - local_t;     /* obtain the difference */
  }

  old = read_ixtime();
  if (old == NULL || old->offset != new.offset)
  {
    write_to_envarc = 1;
    if (set_the_clock && old)
      set_clock(old->offset - new.offset);
  }
  status = write_ixtime(&new, write_to_envarc);
  if (patch_resource)
    patch_batt_resource(new.offset);

  return status == 0 ? 0 : 1;
}
