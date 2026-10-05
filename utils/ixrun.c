/*
 * Revision 1.2  2026/04/06  JJ, Copilot
 *  - Fix argv[] path mangling: avoid corrupting first argument and
 *    do not modify argv[] pointers in place.
 *  - Correct format specifier for allocation size and make main()
 *    explicitly return int.
 *  - Keep Execute() exit status mapping, but make the code clearer.
 *  Behaviour is unchanged except for fixing the /vol/path translation
 *  bug and avoiding undefined behaviour on argv[].
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/dos.h>
#include <utility/tagitem.h>
#include <ix.h>
#include <ctype.h>
#include <signal.h>

char VERSION[] = "\000$VER: ixrun 1.1 (14.06.97)";

static void usage(void)
{
  fprintf(stderr, "Usage: ixrun [-n | -q] filename [arguments...]\n"
"-n\tdon't add quotes (\") around the arguments\n"
"-q\tadd quotes (\") around the arguments (default)\n"
"-nv\tas -n, but print the command line to standard error, don't execute it\n"
"-qv\tas -q, but print the command line to standard error, don't execute it\n");
  exit(1);
}

int main(int argc, char **argv)
{
  char *p;
  char *first_arg;
  char *first_conv = NULL;
  long size, i, first_opt = 1, add_quotes = 2, debug = 0;

  if (argc == 1)
    usage();
  if (!strcmp(argv[1], "-q") || !strcmp(argv[1], "-qv"))
  {
    if (argc == 2)
      usage();
    first_opt++;
    debug = !strcmp(argv[1], "-qv");
  }
  if (!strcmp(argv[1], "-n") || !strcmp(argv[1], "-nv"))
  {
    if (argc == 2)
      usage();
    add_quotes = 0;
    first_opt++;
    debug = !strcmp(argv[1], "-nv");
  }

  first_arg = argv[first_opt];

  /*
   * Translate Unix-style /vol/path into Amiga-style vol:path,
   * without modifying argv[] in place.
   */
  if (first_arg[0] == '/' && (p = strchr(first_arg + 1, '/')))
  {
    size_t flen = strlen(first_arg);
    size_t vol_len = (size_t)(p - (first_arg + 1)); /* between first and second '/' */

    first_conv = malloc(flen); /* one char less ('/' removed) */
    if (!first_conv)
    {
      fprintf(stderr, "couldn't allocate %lu bytes\n", (unsigned long)flen);
      exit(1);
    }

    /* copy volume name (without leading '/') */
    memcpy(first_conv, first_arg + 1, vol_len);
    first_conv[vol_len] = ':';
    /* copy rest of path after second '/' */
    strcpy(first_conv + vol_len + 1, p + 1);

    first_arg = first_conv;
  }

  for (size = strlen(first_arg) + 1, i = first_opt + 1; argv[i]; i++)
    size += strlen(argv[i]) + 1 + add_quotes;
  p = malloc(size);
  if (p == NULL)
  {
    fprintf(stderr, "couldn't allocate %ld bytes\n", size);
    free(first_conv);
    exit(1);
  }

  strcpy(p, first_arg);

  for (i = first_opt + 1; argv[i]; i++)
  {
    if (add_quotes)
      strcat(p, " \"");
    else
      strcat(p, " ");
    strcat(p, argv[i]);
    if (add_quotes)
      strcat(p, "\"");
  }

  free(first_conv);

  if (debug)
    fprintf(stderr, "command line = '%s'\n", p);
  else
  {
    int result, omask;

    omask = sigsetmask(~0);
    result = !Execute(p, NULL, NULL);
    sigsetmask(omask);
    exit(result);
  }
}
