/* spawn.h for the ixemul SDK (UP-Term): posix_spawn and posix_spawnp from
 * libixcompat.a. ixemul has no fork(), only vfork(); these start the child
 * with vfork and exec it, the child changing only its own process state
 * (compat/spawn.c), so a port replaces a fork+exec with them instead of
 * carrying its own vfork code. Not supported: the scheduling attributes
 * (ixemul has no <sched.h>). POSIX_SPAWN_SETSID is POSIX.1-2024's. */
#ifndef _SPAWN_H_
#define _SPAWN_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <signal.h>

#define	POSIX_SPAWN_RESETIDS	0x01
#define	POSIX_SPAWN_SETPGROUP	0x02
#define	POSIX_SPAWN_SETSIGDEF	0x04
#define	POSIX_SPAWN_SETSIGMASK	0x08
#define	POSIX_SPAWN_SETSID	0x80

struct __spawn_action;

typedef struct {
	int			__used;
	int			__size;
	struct __spawn_action	*__actions;
} posix_spawn_file_actions_t;

typedef struct {
	short		__flags;
	pid_t		__pgroup;
	sigset_t	__sigdefault;
	sigset_t	__sigmask;
} posix_spawnattr_t;

__BEGIN_DECLS
int	posix_spawn __P((pid_t *, const char *, const posix_spawn_file_actions_t *,
	    const posix_spawnattr_t *, char *const [], char *const []));
int	posix_spawnp __P((pid_t *, const char *, const posix_spawn_file_actions_t *,
	    const posix_spawnattr_t *, char *const [], char *const []));

int	posix_spawn_file_actions_init __P((posix_spawn_file_actions_t *));
int	posix_spawn_file_actions_destroy __P((posix_spawn_file_actions_t *));
int	posix_spawn_file_actions_addopen __P((posix_spawn_file_actions_t *, int,
	    const char *, int, mode_t));
int	posix_spawn_file_actions_addclose __P((posix_spawn_file_actions_t *, int));
int	posix_spawn_file_actions_adddup2 __P((posix_spawn_file_actions_t *, int, int));
int	posix_spawn_file_actions_addchdir __P((posix_spawn_file_actions_t *, const char *));
int	posix_spawn_file_actions_addfchdir __P((posix_spawn_file_actions_t *, int));
int	posix_spawn_file_actions_addchdir_np __P((posix_spawn_file_actions_t *, const char *));
int	posix_spawn_file_actions_addfchdir_np __P((posix_spawn_file_actions_t *, int));

int	posix_spawnattr_init __P((posix_spawnattr_t *));
int	posix_spawnattr_destroy __P((posix_spawnattr_t *));
int	posix_spawnattr_getflags __P((const posix_spawnattr_t *, short *));
int	posix_spawnattr_setflags __P((posix_spawnattr_t *, short));
int	posix_spawnattr_getpgroup __P((const posix_spawnattr_t *, pid_t *));
int	posix_spawnattr_setpgroup __P((posix_spawnattr_t *, pid_t));
int	posix_spawnattr_getsigdefault __P((const posix_spawnattr_t *, sigset_t *));
int	posix_spawnattr_setsigdefault __P((posix_spawnattr_t *, const sigset_t *));
int	posix_spawnattr_getsigmask __P((const posix_spawnattr_t *, sigset_t *));
int	posix_spawnattr_setsigmask __P((posix_spawnattr_t *, const sigset_t *));
__END_DECLS

#endif /* _SPAWN_H_ */
