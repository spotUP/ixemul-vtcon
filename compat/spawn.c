/* posix_spawn and posix_spawnp over ixemul's vfork (UP-Term; vtcon plan
 * item 0.2). ixemul has no fork(). Its vfork child shares the parent's data
 * and heap (not the stack, of which it gets a copy) and holds the parent
 * until it calls execve() or _exit(). So, as the ports that already did
 * this by hand:
 *   - the child changes only its own process state: signal dispositions,
 *     process group, session, signal mask, descriptors, directory, in the
 *     order of tmux-amiga's amiga/vspawn.c, then execs. It never mallocs
 *     and writes no global but the two below;
 *   - an exec (or file action) error comes back through a static variable,
 *     as neovim-amiga's libuv does (vendor/libuv/src/unix/process.c,
 *     uv__vfork_child_err): when vfork returns in the parent, the child has
 *     either exec'd (0) or failed and exited (the errno), and posix_spawn
 *     returns that error after reaping the child, as POSIX allows;
 *   - environ is swapped to envp in the child for execvp's PATH search and
 *     the exec, in memory it shares with us: the parent puts its own back
 *     as soon as vfork returns (vspawn.c's rule).
 * The file actions are built by the parent (malloc is the parent's). */
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

enum { A_OPEN, A_CLOSE, A_DUP2, A_CHDIR, A_FCHDIR };

struct __spawn_action {
	int	type;
	int	fd;
	int	newfd;		/* dup2's target */
	int	oflag;
	mode_t	mode;
	char	*path;		/* open, chdir: a copy */
};

static volatile int child_err;

static void
child(const char *file, const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *at, char *const argv[], char *const envp[],
    int use_path)
{
	const struct __spawn_action *a;
	int i, fd, sig;
	short fl = at != NULL ? at->__flags : 0;

	if (fl & POSIX_SPAWN_SETSIGDEF)
		for (sig = 1; sig < NSIG; sig++)
			if (sigismember(&at->__sigdefault, sig))
				signal(sig, SIG_DFL);
	if ((fl & POSIX_SPAWN_SETSID) && setsid() == -1)
		goto fail;
	if ((fl & POSIX_SPAWN_SETPGROUP) && setpgid(0, at->__pgroup) == -1)
		goto fail;
	if ((fl & POSIX_SPAWN_RESETIDS) &&
	    (setgid(getgid()) == -1 || setuid(getuid()) == -1))
		goto fail;
	for (i = 0; fa != NULL && i < fa->__used; i++) {
		a = &fa->__actions[i];
		switch (a->type) {
		case A_OPEN:
			if ((fd = open(a->path, a->oflag, a->mode)) == -1)
				goto fail;
			if (fd != a->fd) {
				if (dup2(fd, a->fd) == -1)
					goto fail;
				close(fd);
			}
			break;
		case A_CLOSE:
			close(a->fd);	/* POSIX: closing a closed fd is no error */
			break;
		case A_DUP2:
			if (a->fd == a->newfd) {	/* keep it across exec */
				if ((fd = fcntl(a->fd, F_GETFD)) == -1 ||
				    fcntl(a->fd, F_SETFD, fd & ~FD_CLOEXEC) == -1)
					goto fail;
			} else if (dup2(a->fd, a->newfd) == -1)
				goto fail;
			break;
		case A_CHDIR:
			if (chdir(a->path) == -1)
				goto fail;
			break;
		case A_FCHDIR:
			if (fchdir(a->fd) == -1)
				goto fail;
			break;
		}
	}
	if (fl & POSIX_SPAWN_SETSIGMASK)
		sigprocmask(SIG_SETMASK, &at->__sigmask, NULL);
	environ = (char **)envp;
	if (use_path)
		execvp(file, argv);
	else
		execv(file, argv);
fail:
	child_err = errno != 0 ? errno : ENOEXEC;
	_exit(127);
}

static int
spawn(pid_t *pidp, const char *file, const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *at, char *const argv[], char *const envp[],
    int use_path)
{
	char **saved = environ;
	int status, err;
	pid_t pid;

	child_err = 0;
	pid = vfork();
	if (pid == 0)
		child(file, fa, at, argv, envp != NULL ? envp : saved, use_path);
	environ = saved;
	if (pid == -1)
		return (errno);
	if ((err = child_err) != 0) {
		while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
			;
		child_err = 0;
		return (err);
	}
	if (pidp != NULL)
		*pidp = pid;
	return (0);
}

int
posix_spawn(pid_t *pid, const char *path, const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *at, char *const argv[], char *const envp[])
{
	return (spawn(pid, path, fa, at, argv, envp, 0));
}

int
posix_spawnp(pid_t *pid, const char *file, const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *at, char *const argv[], char *const envp[])
{
	return (spawn(pid, file, fa, at, argv, envp, 1));
}

/* ---- file actions (the parent's side: these may allocate) ---------------- */

int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *fa)
{
	fa->__used = fa->__size = 0;
	fa->__actions = NULL;
	return (0);
}

int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fa)
{
	int i;

	for (i = 0; i < fa->__used; i++)
		free(fa->__actions[i].path);
	free(fa->__actions);
	return (posix_spawn_file_actions_init(fa));
}

static struct __spawn_action *
add(posix_spawn_file_actions_t *fa, int type, int fd)
{
	struct __spawn_action *a;

	if (fd < 0)
		return (NULL);
	if (fa->__used == fa->__size) {
		int n = fa->__size ? 2 * fa->__size : 8;
		a = realloc(fa->__actions, n * sizeof *a);
		if (a == NULL)
			return (NULL);
		fa->__actions = a;
		fa->__size = n;
	}
	a = &fa->__actions[fa->__used];
	memset(a, 0, sizeof *a);
	a->type = type;
	a->fd = fd;
	return (a);
}

static int
add_path(posix_spawn_file_actions_t *fa, int type, int fd, const char *path,
    int oflag, mode_t mode)
{
	struct __spawn_action *a;
	char *copy;

	if (fd < 0)
		return (EBADF);
	if ((copy = strdup(path)) == NULL)
		return (ENOMEM);
	if ((a = add(fa, type, fd)) == NULL) {
		free(copy);
		return (ENOMEM);
	}
	a->path = copy;
	a->oflag = oflag;
	a->mode = mode;
	fa->__used++;
	return (0);
}

static int
add_fd(posix_spawn_file_actions_t *fa, int type, int fd, int newfd)
{
	struct __spawn_action *a;

	if (fd < 0 || newfd < 0)
		return (EBADF);
	if ((a = add(fa, type, fd)) == NULL)
		return (ENOMEM);
	a->newfd = newfd;
	fa->__used++;
	return (0);
}

int
posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fa, int fd,
    const char *path, int oflag, mode_t mode)
{
	return (add_path(fa, A_OPEN, fd, path, oflag, mode));
}

int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fa, int fd)
{
	return (add_fd(fa, A_CLOSE, fd, 0));
}

int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fa, int fd, int newfd)
{
	return (add_fd(fa, A_DUP2, fd, newfd));
}

int
posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *fa, const char *path)
{
	return (add_path(fa, A_CHDIR, 0, path, 0, 0));
}

int
posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *fa, int fd)
{
	return (add_fd(fa, A_FCHDIR, fd, 0));
}

int
posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *fa, const char *path)
{
	return (posix_spawn_file_actions_addchdir(fa, path));
}

int
posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t *fa, int fd)
{
	return (posix_spawn_file_actions_addfchdir(fa, fd));
}

/* ---- attributes ------------------------------------------------------------ */

int
posix_spawnattr_init(posix_spawnattr_t *at)
{
	memset(at, 0, sizeof *at);
	return (0);
}

int
posix_spawnattr_destroy(posix_spawnattr_t *at)
{
	(void)at;
	return (0);
}

int
posix_spawnattr_getflags(const posix_spawnattr_t *at, short *fl)
{
	*fl = at->__flags;
	return (0);
}

int
posix_spawnattr_setflags(posix_spawnattr_t *at, short fl)
{
	if (fl & ~(POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP |
	    POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSID))
		return (EINVAL);	/* the scheduling flags: not supported */
	at->__flags = fl;
	return (0);
}

int
posix_spawnattr_getpgroup(const posix_spawnattr_t *at, pid_t *pg)
{
	*pg = at->__pgroup;
	return (0);
}

int
posix_spawnattr_setpgroup(posix_spawnattr_t *at, pid_t pg)
{
	at->__pgroup = pg;
	return (0);
}

int
posix_spawnattr_getsigdefault(const posix_spawnattr_t *at, sigset_t *s)
{
	*s = at->__sigdefault;
	return (0);
}

int
posix_spawnattr_setsigdefault(posix_spawnattr_t *at, const sigset_t *s)
{
	at->__sigdefault = *s;
	return (0);
}

int
posix_spawnattr_getsigmask(const posix_spawnattr_t *at, sigset_t *s)
{
	*s = at->__sigmask;
	return (0);
}

int
posix_spawnattr_setsigmask(posix_spawnattr_t *at, const sigset_t *s)
{
	at->__sigmask = *s;
	return (0);
}
