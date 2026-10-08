/* cli_args.h -- the AmigaDOS argument line, one argument at a time
 * (cli_args.c; used by _cli_parse.c, tested on the host by
 * tests/host/test_cli_args.c). No ixemul headers: it builds anywhere. */
#ifndef IX_CLI_ARGS_H
#define IX_CLI_ARGS_H

/* Where the argument line of len bytes at line ends: at a NUL, and before
 * the newline (or CR LF) that ends every line RunCommand passes. */
char *__ix_cli_line_end(char *line, long len);

/* The next argument of the line between *linep and lineend, made a
 * NUL-terminated string in place (the line is rewritten: an argument is
 * never longer than its text). Returns 0 when the line holds no further
 * argument, else 1 with *argp set to the argument, *quotedp to 1 when it
 * was written in double quotes, and *linep moved past it. *lineend must
 * be writable (the line's NUL): an argument at the end is ended there. */
int __ix_cli_next_arg(char **linep, char *lineend, char **argp, int *quotedp);

#endif
