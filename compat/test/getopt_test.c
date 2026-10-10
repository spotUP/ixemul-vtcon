/* Host test of compat/getopt_long.c (built with -Dgetopt_long=ixc_getopt_long
 * and the long_only twin): each case is an argv, an option string, and the
 * results in order ("c" or "c=arg", ending with the operands left after
 * optind and the argv order). Exit status = failures. */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ixc_getopt_long(int, char *const *, const char *, const struct option *, int *);
int ixc_getopt_long_only(int, char *const *, const char *, const struct option *, int *);

static int fails;
static int verbose_flag;
static const struct option longs[] = {
	{"verbose", no_argument, &verbose_flag, 1},
	{"query", required_argument, NULL, 'q'},
	{"workers", required_argument, NULL, 'j'},
	{"show-matches", required_argument, NULL, 'e'},
	{"show-info", no_argument, NULL, 'i'},
	{"opt", optional_argument, NULL, 'O'},
	{NULL, 0, NULL, 0}
};

static void
run(const char *name, int only, const char *shortopts, const char *want, const char *args)
{
	char *argv[16], buf[256], line[512], *tok;
	int argc = 0, c, idx = -1;

	strncpy(buf, args, sizeof buf);
	for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " "))
		argv[argc++] = strdup(strcmp(tok, "_") == 0 ? "" : tok);
	argv[argc] = NULL;
	line[0] = 0;
	optind = 0;
	opterr = 0;
	verbose_flag = 0;
	while ((c = (only ? ixc_getopt_long_only : ixc_getopt_long)(argc, argv, shortopts, longs, &idx)) != -1) {
		char one[64];

		if (c == 1)
			snprintf(one, sizeof one, "R=%s ", optarg);
		else if (c == 0)
			snprintf(one, sizeof one, "flag%d ", verbose_flag);
		else if (optarg)
			snprintf(one, sizeof one, "%c=%s ", c, optarg);
		else
			snprintf(one, sizeof one, "%c ", c);
		strcat(line, one);
	}
	strcat(line, "|");
	for (c = optind; c < argc; c++) {
		strcat(line, " ");
		strcat(line, argv[c]);
	}
	if (strcmp(line, want) != 0) {
		printf("[FAIL] %s: got \"%s\", want \"%s\"\n", name, line, want);
		fails++;
	}
}

int
main(void)
{
	unsetenv("POSIXLY_CORRECT");
	run("short cluster", 0, "abo:", "a b o=x |", "p -ab -o x");
	run("short arg glued", 0, "abo:", "o=xyz a |", "p -oxyz -a");
	run("long with =", 0, "q:", "q=hi |", "p --query=hi");
	run("long separate arg", 0, "q:", "q=hi |", "p --query hi");
	run("unique prefix", 0, "", "j=4 |", "p --wor 4");
	run("ambiguous prefix", 0, "", "? |", "p --show");
	run("exact beats prefix", 0, "", "i |", "p --show-info");
	run("flag option", 0, "", "flag1 |", "p --verbose");
	run("operands after options are found", 0, "q:v", "q=x v | dir file", "p dir -q x file -v");
	run("-- ends options", 0, "v", "v | -v file", "p -v -- -v file");
	run("-- after operands", 0, "v", "| dir -v", "p dir -- -v");
	run("lone dash is an operand", 0, "v", "v | -", "p - -v");
	run("POSIX plus stops at first operand", 0, "+v", "| dir -v", "p dir -v");
	run("minus returns operands", 0, "-v", "R=dir v |", "p dir -v");
	run("unknown short", 0, "v", "? v |", "p -z -v");
	run("missing argument", 0, "q:", "? |", "p -q");
	run("missing argument, colon", 0, ":q:", ": |", "p -q");
	run("missing long argument, colon", 0, ":", ": |", "p --query");
	run("no argument allowed", 0, "", "? |", "p --verbose=1");
	run("optional argument glued", 0, "", "O=3 |", "p --opt=3");
	run("optional argument alone", 0, "", "O | 3", "p --opt 3");
	run("long_only single dash", 1, "v", "q=a v |", "p -query a -v");
	run("long_only falls back to short", 1, "vx", "v x |", "p -v -x");
	run("no options", 0, "v", "| a b", "p a b");
	run("empty", 0, "v", "|", "p");
	setenv("POSIXLY_CORRECT", "1", 1);
	run("POSIXLY_CORRECT stops at operand", 0, "v", "| dir -v", "p dir -v");
	printf("%s\n", fails ? "getopt_test: FAILED" : "getopt_test: ok");
	return fails != 0;
}
