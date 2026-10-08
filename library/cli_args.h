/* cli_args.h -- the argument line an ixemul program is started with, one
 * argument at a time (cli_args.c; used by _cli_parse.c, tested on the host
 * by tests/host/test_cli_args.c). No ixemul headers: it builds anywhere. */
#ifndef IX_CLI_ARGS_H
#define IX_CLI_ARGS_H

/* A line being split. */
struct ix_cli_line {
  char *cur, *end;  /* what is left of it */
  int oob;          /* 1: the arguments are the out-of-band argv */
};

/* Starts splitting the argument line of len bytes at line (line[len] must
 * be writable: the line is rewritten in place, an argument is never longer
 * than its text). When the line carries a well-formed out-of-band argv that
 * matches it (vsh's, see cli_args.c), that argv is what is split; else the
 * one in var (varlen bytes, the variable __ixargv; 0 for none, rewritten
 * in place too) when it matches the line. */
void __ix_cli_begin(struct ix_cli_line *l, char *line, long len, char *var, long varlen);

/* The next argument as a NUL-terminated string: 0 when there is none, else
 * 1 with *argp set and *quotedp 1 when it is never to be globbed (written in
 * double quotes, or an out-of-band argument). */
int __ix_cli_next(struct ix_cli_line *l, char **argp, int *quotedp);

#endif
