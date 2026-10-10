/* binshprobe: an ixemul program starting the shell by its Unix names, on the
 * rig with vsh installed as the kit installs it (GG:bin/sh is vsh). configure
 * scripts and make start /bin/sh: system() and popen() by _PATH_BSHELL
 * (include/paths.h, "/gg/bin/sh"), execl() and a #! line by the name the
 * program or the script gives. Each check prints [OK] or [FAIL] (as
 * compat/test/spawnprobe.c) and is appended to the log file named by the
 * first argument, closed after each line; the exit status is the number of
 * failures. A child that cannot exec exits 100 + errno. Scratch files in T:.
 *   system     system("echo ok >T:binsh.out")
 *   popen      popen("echo ok", "r") reads "ok"
 *   execl sh   execl("/bin/sh", "sh", "-c", ...)
 *   execl bash execl("/bin/bash", "bash", "-c", ...)
 *   #!/bin/sh  a script whose first line is #!/bin/sh, by execl
 *   #!/bin/bash the same with #!/bin/bash
 * Build: m68k-amigaos-gcc -mcrt=ixemul -O2 -Wall -o binshprobe tests/amiga/binshprobe.c
 * Run, in a visible window on the rig (vsh copied to GG:bin/sh, this tree's
 * ixemul.library first in LIBS:): binshprobe RAM:binshprobe.log
 * Rig 2, 2026-10-10: before the __load_seg.c fix system and popen [OK], the
 * other four [FAIL] errno 2 (ENOENT); after it all six [OK]. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OUT "T:binsh.out"
#define SCRIPT "T:binsh.script"

static const char *logname;
static int fails;

static void
say(const char *line)
{
	FILE *f;

	printf("%s\n", line);
	fflush(stdout);
	if (logname && (f = fopen(logname, "a")) != NULL) {
		fprintf(f, "%s\n", line);
		fclose(f);
	}
}

static void
check(int ok, const char *what, const char *seen)
{
	char line[256];

	snprintf(line, sizeof line, "%s %s%s%s", ok ? "[OK]  " : "[FAIL]", what,
	    ok ? "" : ": ", ok ? "" : seen);
	say(line);
	if (!ok)
		fails++;
}

/* OUT holds "ok\n" */
static int
out_ok(char *seen, size_t max)
{
	FILE *f = fopen(OUT, "r");
	size_t n = 0;

	seen[0] = 0;
	if (f) {
		n = fread(seen, 1, max - 1, f);
		fclose(f);
	}
	seen[n] = 0;
	return (strcmp(seen, "ok\n") == 0);
}

static void
status_text(char *s, size_t max, int st, const char *out)
{
	if (WIFEXITED(st) && WEXITSTATUS(st) >= 100)
		snprintf(s, max, "exec failed, errno %d (%s)", WEXITSTATUS(st) - 100,
		    strerror(WEXITSTATUS(st) - 100));
	else if (WIFEXITED(st))
		snprintf(s, max, "exit %d, output [%s]", WEXITSTATUS(st), out);
	else
		snprintf(s, max, "status 0x%x", st);
}

/* vfork + execv(path, argv), then OUT */
static void
exec_check(const char *what, const char *path, char *const argv[])
{
	char seen[64], text[160];
	pid_t pid;
	int st = -1;

	unlink(OUT);
	pid = vfork();
	if (pid == 0) {
		execv(path, argv);
		_exit(100 + errno);
	}
	if (pid < 0) {
		check(0, what, "vfork failed");
		return;
	}
	waitpid(pid, &st, 0);
	out_ok(seen, sizeof seen);
	status_text(text, sizeof text, st, seen);
	check(WIFEXITED(st) && WEXITSTATUS(st) == 0 && strcmp(seen, "ok\n") == 0, what, text);
}

static void
script_check(const char *what, const char *first)
{
	FILE *f = fopen(SCRIPT, "w");
	char *argv[2];

	if (!f) {
		check(0, what, "cannot write " SCRIPT);
		return;
	}
	fprintf(f, "%s\necho ok >" OUT "\n", first);
	fclose(f);
	chmod(SCRIPT, 0755);
	argv[0] = SCRIPT;
	argv[1] = NULL;
	exec_check(what, SCRIPT, argv);
}

int
main(int argc, char **argv)
{
	char seen[64], text[160];
	char *sh_argv[] = { "sh", "-c", "echo ok >" OUT, NULL };
	char *bash_argv[] = { "bash", "-c", "echo ok >" OUT, NULL };
	FILE *p;
	int st;
	size_t n;

	logname = argc > 1 ? argv[1] : NULL;
	if (logname)
		unlink(logname);

	unlink(OUT);
	st = system("echo ok >" OUT);
	out_ok(seen, sizeof seen);
	status_text(text, sizeof text, st, seen);
	check(st == 0 && strcmp(seen, "ok\n") == 0, "system", text);

	p = popen("echo ok", "r");
	n = p ? fread(seen, 1, sizeof seen - 1, p) : 0;
	seen[n] = 0;
	st = p ? pclose(p) : -1;
	status_text(text, sizeof text, st, seen);
	check(p && st == 0 && strcmp(seen, "ok\n") == 0, "popen", text);

	exec_check("execl /bin/sh", "/bin/sh", sh_argv);
	exec_check("execl /bin/bash", "/bin/bash", bash_argv);
	script_check("#!/bin/sh script", "#!/bin/sh");
	script_check("#!/bin/bash script", "#!/bin/bash");

	unlink(OUT);
	unlink(SCRIPT);
	snprintf(text, sizeof text, "%d failures", fails);
	say(text);
	return (fails);
}
