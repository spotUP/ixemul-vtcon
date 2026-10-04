/* spawnprobe: posix_spawn/posix_spawnp from libixcompat on the rig (vtcon
 * plan item 0.2). Run it by a path (VTC:spawnprobe or /VTC/spawnprobe): it
 * spawns itself as the child. Each check prints [OK] or [FAIL]; the exit
 * status is the number of failures. Scratch files go to T:.
 *   spawn      posix_spawn runs the child; waitpid sees its exit status
 *   dup2/open  file actions: the child's stdout into a pipe, then a file
 *   chdir      the child's directory changes, the parent's does not
 *   sigmask    the child starts with the mask from the attributes
 *   envp       the child sees envp; the parent's environ is untouched
 *   spawnp     posix_spawnp searches envp's PATH
 *   enoent     a failing exec returns ENOENT, reaps the child, and leaves
 *              the parent's errno, environ and heap as they were */
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;
static int fails;

static void
check(int ok, const char *what)
{
	printf("%s %s\n", ok ? "[OK]  " : "[FAIL]", what);
	if (!ok)
		fails++;
}

static int
child(int argc, char **argv)
{
	sigset_t m;
	const char *v;

	if (strcmp(argv[2], "exit") == 0)
		return (atoi(argv[3]));
	if (strcmp(argv[2], "say") == 0) {
		printf("%s\n", argv[3]);
		return (0);
	}
	if (strcmp(argv[2], "exists") == 0)
		return (access(argv[3], F_OK) == 0 ? 0 : 1);
	if (strcmp(argv[2], "env") == 0)
		return ((v = getenv("SPAWNPROBE")) != NULL && strcmp(v, "yes") == 0 ? 0 : 1);
	if (strcmp(argv[2], "mask") == 0) {
		sigprocmask(SIG_BLOCK, NULL, &m);
		return (sigismember(&m, SIGUSR1) ? 0 : 1);
	}
	(void)argc;
	return (99);
}

static int
run(const char *self, const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t *at, char *const av[], char *const env[], int *code)
{
	pid_t pid;
	int st, r;

	if ((r = posix_spawn(&pid, self, fa, at, av, env)) != 0)
		return (r);
	if (waitpid(pid, &st, 0) != pid || !WIFEXITED(st))
		return (-1);
	*code = WEXITSTATUS(st);
	return (0);
}

int
main(int argc, char **argv)
{
	char *self = argv[0], buf[64], dir[256], cwd[256], *base, *heap;
	char **saved;
	posix_spawn_file_actions_t fa;
	posix_spawnattr_t at;
	sigset_t m;
	pid_t pid;
	int p[2], code = -1, r, n, fd, st;

	if (argc > 3 && strcmp(argv[1], "child") == 0)
		return (child(argc, argv));
	setvbuf(stdout, NULL, _IONBF, 0);

	{ char *av[] = { self, "child", "exit", "7", NULL };
	  r = run(self, NULL, NULL, av, environ, &code);
	  check(r == 0 && code == 7, "spawn: the child ran and exited 7"); }

	{ char *av[] = { self, "child", "say", "piped", NULL };
	  pipe(p);
	  posix_spawn_file_actions_init(&fa);
	  posix_spawn_file_actions_adddup2(&fa, p[1], 1);
	  posix_spawn_file_actions_addclose(&fa, p[0]);
	  posix_spawn_file_actions_addclose(&fa, p[1]);
	  r = posix_spawn(&pid, self, &fa, NULL, av, environ);
	  close(p[1]);
	  n = r == 0 ? read(p[0], buf, sizeof buf - 1) : -1;
	  close(p[0]);
	  if (r == 0)
		waitpid(pid, &st, 0);
	  buf[n > 0 ? n : 0] = '\0';
	  posix_spawn_file_actions_destroy(&fa);
	  check(r == 0 && strcmp(buf, "piped\n") == 0, "dup2: the child's stdout reached the pipe"); }

	{ char *av[] = { self, "child", "say", "filed", NULL };
	  unlink("T:spawnprobe.out");
	  posix_spawn_file_actions_init(&fa);
	  posix_spawn_file_actions_addopen(&fa, 1, "T:spawnprobe.out",
	      O_WRONLY | O_CREAT | O_TRUNC, 0644);
	  r = run(self, &fa, NULL, av, environ, &code);
	  posix_spawn_file_actions_destroy(&fa);
	  n = -1;
	  if ((fd = open("T:spawnprobe.out", O_RDONLY)) >= 0) {
		n = read(fd, buf, sizeof buf - 1);
		close(fd);
	  }
	  buf[n > 0 ? n : 0] = '\0';
	  check(r == 0 && strcmp(buf, "filed\n") == 0, "open: the child's stdout went to T:spawnprobe.out"); }

	{ char *av[] = { self, "child", "exists", "spawnprobe.mark", NULL };
	  fd = open("T:spawnprobe.mark", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	  close(fd);
	  getcwd(cwd, sizeof cwd);
	  posix_spawn_file_actions_init(&fa);
	  posix_spawn_file_actions_addchdir(&fa, "T:");
	  r = run(self, &fa, NULL, av, environ, &code);
	  posix_spawn_file_actions_destroy(&fa);
	  getcwd(dir, sizeof dir);
	  check(r == 0 && code == 0 && strcmp(cwd, dir) == 0,
	      "chdir: the child found T:spawnprobe.mark, the parent stayed");
	  unlink("T:spawnprobe.mark"); }

	{ char *av[] = { self, "child", "mask", "x", NULL };
	  posix_spawnattr_init(&at);
	  sigemptyset(&m);
	  sigaddset(&m, SIGUSR1);
	  posix_spawnattr_setsigmask(&at, &m);
	  posix_spawnattr_setflags(&at, POSIX_SPAWN_SETSIGMASK);
	  r = run(self, NULL, &at, av, environ, &code);
	  check(r == 0 && code == 0, "sigmask: the child started with SIGUSR1 blocked");
	  sigprocmask(SIG_BLOCK, NULL, &m);
	  check(!sigismember(&m, SIGUSR1), "sigmask: the parent's mask is unchanged"); }

	{ char *av[] = { self, "child", "env", "x", NULL };
	  char *env[] = { "SPAWNPROBE=yes", NULL };
	  saved = environ;
	  r = run(self, NULL, NULL, av, env, &code);
	  check(r == 0 && code == 0, "envp: the child saw SPAWNPROBE=yes");
	  check(environ == saved && getenv("SPAWNPROBE") == NULL,
	      "envp: the parent's environ is untouched"); }

	/* posix_spawnp: the name alone, found through envp's PATH */
	strncpy(dir, self, sizeof dir - 1);
	dir[sizeof dir - 1] = '\0';
	base = strrchr(dir, '/');
	if (base == NULL)
		base = strrchr(dir, ':');
	if (base != NULL) {
		char path[300];
		char *env[] = { path, "SPAWNPROBE=yes", NULL };
		char *av[] = { NULL, "child", "env", "x", NULL };
		av[0] = base + 1;
		snprintf(path, sizeof path, "PATH=%.*s", (int)(base - dir + (*base == ':')), dir);
		r = posix_spawnp(&pid, base + 1, NULL, NULL, av, env);
		code = -1;
		if (r == 0 && waitpid(pid, &st, 0) == pid && WIFEXITED(st))
			code = WEXITSTATUS(st);
		printf("       spawnp: %s with %s\n", base + 1, path);
		check(r == 0 && code == 0, "spawnp: found by envp's PATH, saw envp");
	} else
		check(0, "spawnp: run spawnprobe by a path (VTC:spawnprobe)");

	{ char *av[] = { "x", NULL };
	  heap = malloc(32);
	  strcpy(heap, "canary");
	  saved = environ;
	  errno = 1234;
	  r = posix_spawn(&pid, "T:no/such/program", NULL, NULL, av, environ);
	  check(r == ENOENT, "enoent: a missing program gives ENOENT");
	  printf("       enoent: returned %d (%s)\n", r, strerror(r));
	  check(environ == saved && strcmp(heap, "canary") == 0,
	      "enoent: the parent's environ and heap are as they were");
	  check(waitpid(-1, &st, WNOHANG) <= 0, "enoent: no child left to reap");
	  free(heap); }

	printf("%d failures\n", fails);
	return (fails);
}
