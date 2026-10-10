/* getopt.h for the ixemul SDK (UP-Term): getopt_long and getopt_long_only,
 * which libixcompat (compat/getopt_long.c) provides over ixemul's getopt
 * variables (optarg, optind, opterr, optopt from <unistd.h>). */
#ifndef _GETOPT_H_
#define _GETOPT_H_

#include <unistd.h>

#define no_argument		0
#define required_argument	1
#define optional_argument	2

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

__BEGIN_DECLS
int getopt_long __P((int, char * const *, const char *, const struct option *, int *));
int getopt_long_only __P((int, char * const *, const char *, const struct option *, int *));
__END_DECLS

#endif /* _GETOPT_H_ */
