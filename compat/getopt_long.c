/* getopt_long and getopt_long_only (UP-Term libixcompat). ixemul 48.2 has
 * getopt() but no long options (fzy 1.0 failed to link on getopt_long).
 * GNU behaviour, as programs expect it: operands are permuted behind the
 * options ("fzy dir -q x" finds -q), unless POSIXLY_CORRECT is set or the
 * option string starts with '+'; a leading '-' returns every operand as
 * option 1; a leading ':' makes a missing argument ':' and silences the
 * messages; "--" ends the options; a unique prefix of a long name is
 * accepted. The scan state is private. optind (0 starts again), optarg,
 * opterr and optopt are ixemul getopt's variables, so a program that mixes
 * getopt() with this finishes one scan first. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

static char *nextchar;		/* rest of a short-option word, if inside one */
static int first_nonopt;	/* the operands skipped so far are [first_nonopt, optstart) */
static int optstart;		/* where the option word being parsed began, or -1 */
static int last_optind;		/* optind we returned last (a lower optind restarts) */

static void
reverse(char **v, int a, int b)	/* [a,b) */
{
	for (b--; a < b; a++, b--) {
		char *t = v[a];
		v[a] = v[b];
		v[b] = t;
	}
}

/* [a,b) operands, [b,c) options -> options first */
static void
exchange(char **v, int a, int b, int c)
{
	reverse(v, a, b);
	reverse(v, b, c);
	reverse(v, a, c);
}

static int
finish(int ret)
{
	last_optind = optind;
	nextchar = NULL;
	return ret;
}

static int
parse(int argc, char **argv, const char *shortopts, const struct option *lo, int *li, int only)
{
	int permute = 1, retnon = 0, silent, colon_first;
	const char *prog = argv[0];
	const char *p;
	char c;

	if (getenv("POSIXLY_CORRECT"))
		permute = 0;
	if (*shortopts == '+') {
		permute = 0;
		shortopts++;
	} else if (*shortopts == '-') {
		retnon = 1;
		shortopts++;
	}
	colon_first = (*shortopts == ':');
	if (colon_first)
		shortopts++;
	silent = colon_first || opterr == 0;

	if (optind == 0 || optind < last_optind) {
		optind = 1;
		nextchar = NULL;
		first_nonopt = 1;
		optstart = -1;
	}
	optarg = NULL;

	if (!nextchar || !*nextchar) {
		nextchar = NULL;
		if (optstart >= 0 && first_nonopt < optstart) {
			exchange(argv, first_nonopt, optstart, optind);
			first_nonopt += optind - optstart;
		} else if (optstart >= 0) {
			first_nonopt = optind;
		}
		optstart = -1;
		while (optind < argc && !(argv[optind][0] == '-' && argv[optind][1] != '\0')) {
			if (retnon) {
				optarg = argv[optind++];
				first_nonopt = optind;
				return finish(1);
			}
			if (!permute)
				break;
			optind++;
		}
		if (optind < argc && strcmp(argv[optind], "--") == 0) {
			optind++;
			if (first_nonopt < optind - 1)
				exchange(argv, first_nonopt, optind - 1, optind);
			optind = first_nonopt = first_nonopt + 1;	/* "--" now sits before the operands */
			return finish(-1);
		}
		if (optind >= argc || !(argv[optind][0] == '-' && argv[optind][1] != '\0')) {
			if (permute && first_nonopt < optind)
				optind = first_nonopt;
			first_nonopt = optind;
			return finish(-1);
		}
		optstart = optind;

		if (argv[optind][1] == '-' ||
		    (only && !(argv[optind][2] == '\0' && argv[optind][1] != ':' && strchr(shortopts, argv[optind][1])))) {
			char *name = argv[optind] + (argv[optind][1] == '-' ? 2 : 1);
			size_t len = strcspn(name, "=");
			const struct option *o, *hit = NULL;
			int ambig = 0;

			for (o = lo; o && o->name; o++) {
				if (strncmp(o->name, name, len) != 0)
					continue;
				if (strlen(o->name) == len) {
					hit = o;
					ambig = 0;
					break;
				}
				if (hit && (hit->has_arg != o->has_arg || hit->flag != o->flag || hit->val != o->val))
					ambig = 1;
				if (!hit)
					hit = o;
			}
			if (!hit && only && argv[optind][1] != '-' && name[0] != ':' && strchr(shortopts, name[0]))
				goto shortword;
			if (ambig || !hit) {
				if (!silent)
					fprintf(stderr, ambig ? "%s: option '%s%.*s' is ambiguous\n" :
					    "%s: unrecognized option '%s%.*s'\n", prog,
					    argv[optind][1] == '-' ? "--" : "-", (int)len, name);
				optopt = 0;
				optind++;
				return finish('?');
			}
			optind++;
			if (name[len] == '=') {
				if (hit->has_arg == no_argument) {
					if (!silent)
						fprintf(stderr, "%s: option '--%s' doesn't allow an argument\n", prog, hit->name);
					optopt = hit->val;
					return finish('?');
				}
				optarg = name + len + 1;
			} else if (hit->has_arg == required_argument) {
				if (optind < argc)
					optarg = argv[optind++];
				else {
					if (!silent)
						fprintf(stderr, "%s: option '--%s' requires an argument\n", prog, hit->name);
					optopt = hit->val;
					return finish(colon_first ? ':' : '?');
				}
			}
			if (li)
				*li = (int)(hit - lo);
			last_optind = optind;
			if (hit->flag) {
				*hit->flag = hit->val;
				return 0;
			}
			return hit->val;
		}
shortword:
		nextchar = argv[optind] + 1;
	}

	c = *nextchar++;
	p = (c == ':') ? NULL : strchr(shortopts, c);
	if (*nextchar == '\0')
		optind++;
	if (!p) {
		if (!silent)
			fprintf(stderr, "%s: invalid option -- '%c'\n", prog, c);
		optopt = c;
		return (last_optind = optind, '?');
	}
	if (p[1] == ':') {
		if (*nextchar != '\0') {
			optarg = nextchar;
			nextchar = NULL;
			optind++;
		} else if (p[2] == ':') {
			optarg = NULL;		/* optional: only in the same word */
		} else if (optind < argc) {
			optarg = argv[optind++];
		} else {
			if (!silent)
				fprintf(stderr, "%s: option requires an argument -- '%c'\n", prog, c);
			optopt = c;
			return finish(colon_first ? ':' : '?');
		}
	}
	last_optind = optind;
	return c;
}

int
getopt_long(int argc, char *const *argv, const char *shortopts, const struct option *lo, int *li)
{
	return parse(argc, (char **)argv, shortopts, lo, li, 0);
}

int
getopt_long_only(int argc, char *const *argv, const char *shortopts, const struct option *lo, int *li)
{
	return parse(argc, (char **)argv, shortopts, lo, li, 1);
}
