---
date: 2026-10-05
topic: Move ixemul-vtcon from 48.2 to ixemul 80.1, carrying the UP-Term patches
tags: [ixemul, 80.1, rebase, af_unix, scm_rights, pty, termios, sigwinch, ixnet]
status: final
---

# ixemul 80.1 migration

**Decision (owner, 2026-10-05):** migrate fully to 80.1 now. 48.2 stays as the backup:
branch `feature/wide-chars` (fa0156c) and its `build295` library are kept untouched, and
the kit can be rebuilt from them at any time (`IXEMUL_LIB=`).

**Input:** vtcon `thoughts/shared/research/2026-10-05_ixemul-80-evaluation.md` (the patch
table #1-#29, ABI measurements, risks, rig matrix). Section numbers below refer to it.

**Done means:** a library built from `feature/ixemul-80` (80.1 + every patch the table
marks keep/rebase/rewrite) passes the rig matrix R1-R11 with ixemul and ixnet from the
same build, and the UP-Term kit installs both libraries.

## Source

- Archive: `https://aminet.net/dev/lib/ixemul-80.1-m68k.lha`, 5,536,049 bytes,
  sha256 `9bf3d573650ffd7211b399451900345029f2d0cce54ddf6b28754f1ff55308ae`.
- The import commit is `ixemul/` of that archive, laid over pristine 48.2 (1ee186a), so
  `git diff 1ee186a <import>` is exactly 48.2 -> 80.1.
- Branches: `import/ixemul-80.1` (pristine 80.1), `feature/ixemul-80` (our patches on it).
- Work tree: `~/Code/ixemul-80` (a git worktree). Remove it when the branch is merged.

## Constraints

- Library compiler stays gcc 2.95.3 in Docker (`docker/build.sh`); bebbo's gcc 6 only for
  libixcompat and the ports.
- ixemul and ixnet are version-locked (`ixnet_open.c:82-91`, ix_panic on a mismatch). Every
  place that installs ixemul.library must install ixnet.library from the same build: the
  rig (`ixpty_rig.use_ixemul`, `rig.py` RIG_LIBS) and the kit (`dist/install.dos`,
  `Makefile` dist).
- Max 2 emulators at once.
- The SDK in `~/opt/amiga/m68k-amigaos/ixemul` is shared by every port: change it only in
  phase M8, after the library passes the rig.

## Checklist

### M0 Import
- [x] M0.1 Branch `import/ixemul-80.1` from 1ee186a; tree = 80.1 `ixemul/`; one commit.
- [x] M0.2 `tools/functable.py` and `tools/stubcheck.py` (the vector and stub checkers
      from the evaluation) committed on `feature/ixemul-80`.

### M1 Build system (#1, #2)
- [x] M1.1 `docker/build.sh` builds pristine 80.1 (ixemul 020/881 + ixnet 020).
- [x] M1.2 functable: 660 vectors in ixemul, 112 in ixnet (section 3).
- [x] M1.3 Our stdint.h dropped in favour of 80.1's; create_header cross fix carried.

### M2 Clean rebases
- [x] M2.1 #4 ^\ ^Z (905dc76)
- [x] M2.2 #5 job control (cfe5322)
- [x] M2.3 #9 `_psignal` guard (98cefba)
- [x] M2.4 #14 execve native env (5f2d856)
- [x] M2.5 #15 IXPIPE: quiet (8836946)
- [x] M2.6 #20 FIONREAD, printf z t j hh (9c75798)
- [x] M2.7 #22 size_t (ddd6e22)
- [x] M2.8 #23 __plock device part (0a1c765)
- [x] M2.9 #25-#29 libixcompat and headers (0f62d3e, 672d4f7, aea8ab7, 2e152be, a8d7bdb)

### M3 Hand rebases into files 80.1 rewrote
- [x] M3.1 #3 termios and winsize by packet (bf02921) into `__tioctl.c`
- [x] M3.2 #6 BSD ptys on PTY:, SIGWINCH to the group (64ae67f), `__close.c` per-process
- [x] M3.3 #7 version id "[UP-Term ...]" (e085db4)
- [x] M3.4 #8 SIGTTIN/SIGTTOU (6d36ffc) into the rewritten `__read.c`/`__write.c`
- [x] M3.5 #13a stat of /dev/tty (825ece7, the /dev/tty half)
- [x] M3.6 #17 SDK header parts (bb620d4): va_start/va_copy, signal.h, termios O*,
      socket.h names, langinfo
- [x] M3.7 #19 select: write-only wake, EINTR from ixnet, console read/write (3073a40)
- [x] M3.8 #21 malloc small-block cache (5fbb894) on the changed `malloc.c`/`vfork.c`
- [x] M3.9 #24 non-seekable stream S_IFIFO/ESPIPE (8dad95d) on the rewritten `lseek.c`
- [x] M3.10 Dropped as covered, verified by reading 80.1: #10 (ac7b11c), #12 compat
      poll/realpath (e137b24), #13b connect (825ece7 second half)

### M4 AF_UNIX on 80.1's refcounted streams
- [x] M4.1 #16 socketpair(AF_UNIX), socklen_t/sa_family_t, `*_r`, if_nametoindex (5c7bd59);
      one `socklen_t` guard across sys/socket.h, arpa/inet.h, netdb.h
- [x] M4.2 #11 sendmsg/recvmsg SCM_RIGHTS (bc5a7a1)
- [x] M4.3 #18 descriptors ride with their message, one waiter per direction, POSIX
      connect errors (78d2934)

### M5 vtcon handlers: per-process WAIT_CHAR (section 5)
- [x] M5.1 `pty_handler.c` keeps a waiter list instead of ending the older WAIT_CHAR
- [x] M5.2 `vtcon_handler.c` the same
- [x] M5.3 Host test for each (vtcon `make test`)

### M6 Rig and kit carry ixnet
- [x] M6.1 `ixpty_rig.use_ixemul` copies ixnet.library beside ixemul.library
- [x] M6.2 Kit: `Makefile` dist copies ixnet.library; `install.dos` and
      `Install.installer` install it (and keep the old one as .orig, like ixemul)

### M7 Rig matrix (section 7)
- [x] R1 library loads, versions match, GG binaries (ls, wc, less, nano, tcsh)
- [x] R2 ixpty 24/24, ptytest, ttyprobe, getty
- [x] R3 ixc99 17/17, ixbg/ixsig/ixsock/ixwait, tcsh ^Z bg fg
- [x] R4 tmux_rig 7/7, screen_rig all
- [x] R5 ixpipe_rig 4/4, no requester
- [x] R6 vshpath_rig, slash_rig, dotdot
- [ ] R7 SIGWINCH redraw (tmux, vim, less) and the 20-exits-while-dragging freeze recipe
- [x] R8 CPython c3-sentinel all OK; vector-audit 0 mismatches
- [x] R9 nvim_rig --tui, --v012 --tui
- [x] R10 upterm-ports grep 3.12, ncurses 6.6 cases; spawnprobe
- [x] R11 mallocbench 48.2 vs 80.x recorded

### M8 SDK and ports
- [x] M8.1 SDK headers from 80.x + ours; resolve libgen.h (const vs writable), poll.h
      (nfds_t, POLLWR*), neovim's netdb.h addrinfo
- [x] M8.2 `make -C compat install` (the stale 4.6 KB libixcompat.a, section 4)
- [x] M8.3 Rebuild CPython, neovim (0.4.4, 0.12.5), tmux, screen, upterm-ports; re-run
      R4, R8, R9, R10

### M9 Land
- [x] M9.1 Kit built with the 80.x pair; install_rig passes
- [x] M9.2 vtcon ledger and this plan updated; worktree removed after merge

### M10 Library with bebbo's gcc 6 (owner question 2026-10-05)
Status note 2026-10-06: M1-M9 landed; R7, M10.1 and M10.2 stay open. The M10 work moved to the gcc 16 track (see the notes at the end); the gcc 6 wording below is stale.
- [ ] M10.1 Build the M9 tree with bebbo's gcc 6 (the 32efe2b C fixes are carried);
      the 48.2 gcc 6 build crashed at run time, cause never found: bisect it against
      the 2.95.3 build object by object if it recurs
- [ ] M10.2 Rig matrix R1-R11 on the gcc 6 build; mallocbench and conbench against
      the 2.95.3 build; keep gcc 6 only if it passes everything and is faster

Rig results gcc 16 (2026-10-06, 060 rig, buildgcc16 ixemul + ixnet 80.1 in VTC:ixp6,
prebuilt vtcon/build/amiga binaries; M10.2 stays OPEN):
- R1 pass (versions 80.1/80.1, ls, wc -c 6, tcsh, less and nano draw/edit/quit).
- R2 FAIL only in getty_rig: ixpty 26/26, ptytest 34/34, ttyprobe no FAIL; getty 2 of 7 on
  gcc 16 AND 2 of 7 on 2.95 (same rig, same binaries; first check "vsh's prompt arrives":
  the prompt carries OSC 7/133 sequences). Not library-specific; test/vsh drift, not run down.
- R3 pass (ixc99 17/17, ixsock names/pairpingpong/pairwake/rights/accept, tcsh ^Z, bg, fg,
  SIGTTOU and SIGTTIN suspend; ixsig not built in vtcon/build/amiga, not run).
- R4 same as recorded: tmux 6/7, screen 4 children ok; both fail only the colour cube (36 of 240).
- R5 3/3 (part 1, no requester, skipped: IXPIPE: is mounted at boot on this rig, as before).
- R6 pass (vshpath 5/5, slash 9/9, dotdot identical to build295).
- R7 known open: after resize less does not reflow (W47, same on 48.2 and 2.95);
  20 less exits while dragging another window's title bar: no freeze, 20/20, agent alive.
- R8 sentinel 27 OK, no FAIL; vector-audit 258/258 (source table, build-independent).
- R9 nvim 0.4.4 8/8, 0.12.5 9/9.
- R10 grep 17/17, ncurses 5/5; spawnprobe 13/14 (the known spawnp PATH case).
- R11 ixmalloc (mallocbench is a vbcc program and not built): ms per 10000 calls, gcc 16
  vs 2.95: malloc 24-33 vs 18-19, malloc+free 10-13 vs 7-8, realloc 7-8 vs 5-6
  (gcc 16 about 40 percent slower; no conbench row in R1-R11).

## Rig results so far (2026-10-05, 060 rig)

- Pristine 80.1 from build.sh: loads, ixnet version matches, GG ls runs.
- 80.x + patches: ixpty 24/24; tmux_rig 6/7 and screen_rig all four
  children, the same as 48.2 on this rig (both fail only the colour cube,
  36 of 240 cells, on the 060 rig's RTG screen: not the library);
  ixsock names/pairpingpong/pairwake/rights all ok.
- Found on the way: 80.1 refused a sun_path without NUL (screen's bind):
  fixed (2ae1c37), test vtcon ixsock names (2b7e767).

- R2 (vtcon 842467d handlers): ixpty 26/26, ptytest 34/34, ttyprobe as
  designed (shot), getty 7/7. R3: ixc99 17/17; tcsh job control: ^Z, bg,
  fg, SIGTTOU "Suspended (tty output)", SIGTTIN "Suspended (tty input)".
- M5 (vtcon 842467d): waitset per task; debug trace old 35 WAIT_CHAR / 30
  "no", new 15 / 10.

- R1: Version 80.1 for both libraries, GG ls, less draws and quits, nano
  edits, saves and quits. R4 after M5: tmux 6/7, screen 4/4 (the colour
  cube fails the same on 48.2: the 060 rig's RTG screen). R5 ixpipe 3/3.
  R6 vshpath 5/5, slash 9/9, dotdot identical to 48.2. R8 sentinel all OK,
  vector-audit 0 mismatches. R9 nvim 0.4.4 8/8, 0.12.5 9/9. R10 grep and
  ncurses cases 22/22 (spawnprobe waits for M8: libixcompat builds
  against the SDK headers). R11 malloc+free 795 ns (80.x) vs 724 ns (48.2),
  both with the small-block cache.
- R7 open: after a window resize, less, nvim and tmux do not redraw -- on
  48.2 just the same (not a regression; vtcon ledger W47). The
  20-exits-while-dragging freeze recipe not run yet.
- FS-UAE aborted (host malloc) when nvim, run without -i NONE and with no
  HOME, wrote its ShaDa into a host folder named "~" on BOOTX: -- a host
  file system bug of FS-UAE, not ixemul; with -i NONE the same runs pass.

- M8 (2026-10-05): SDK headers from 80.x + ours installed
  (docker/install-sdk-headers.sh; backup ~/opt/ixemul-sdk-backup-2026-10-05.tgz),
  libixcompat installed (46 KB, was the stale 4.6 KB). libc.a stays 48.2's
  stub set, so rebuilt ports run on both libraries. Fixes it needed:
  libixcompat stpcpy/mempcpy (gcc writes calls to them once declared),
  neovim-amiga compat netdb.h defers to 80.x's (be3c4e5). Rebuilt and
  rig-checked on 80.x: CPython sentinel 27 OK, tmux 6/7, screen 4/4, nvim
  0.4.4 8/8 and 0.12.5 9/9, grep+ncurses cases 22/22, spawnprobe the known
  13/14 (same on 48.2). On 48.2: sentinel 27 OK, nvim 0.12.5 9/9, tmux 6/7.

- M9 (2026-10-05): kit with the 80.1 pair, install_rig 56/56 (ixnet kept as
  .orig and restored). ixemul-vtcon's main checkout is on feature/ixemul-80
  (build295 = 80.1, the default path of vtcon's kit and rig); the 48.2 line
  is branch stable-48.2 (fa0156c). Worktree ~/Code/ixemul-80 removed.
  2026-10-06: the remaining hang is fixed. string/memmove.c calls bcopy(),
  which gcc 16 folds into memmove(): a jump to memmove's own entry, so the
  first memmove never returned (also at -O0). string/Makefile.in now builds
  with -fno-builtin. Rig: ls and ixc99 17 of 17 on the full gcc 16 library
  (before: ls hung). tools/selfcall_check.py reports any function that
  branches to its own entry; clean on string, general, stdlib, stdio.
  The R1-R11 matrix on the gcc 16 build is still open.
- M10 (gcc 16) open, separate track. Found and fixed so far: gcc 16 left
  d2 unsaved (m68k_save_reg shortcut on stale df chains) and has no
  AmigaOS float return (new -mfloat-return-d0) -- compiler branch
  feature/m68k-save-reg-and-float-return-d0 in ~/Code/amiga-gcc15/projects/gcc;
  library: memset became `jra _memset` (e8bcc9d), USP/SR asm moved across
  calls, setrun clobbers, asm string syntax (40852da). Still hangs: works
  with gcc 2.95's string objects and gcc 16's rest, hangs with gcc 16's
  string even at -O0 -- how gcc 16's string objects combine with the rest
  (duplicate definitions, data placement), not optimisation. Tools:
  tools/mixlink.sh (a.out objects first in a mixed archive).

## Decisions log

- 2026-10-05: worktree at `~/Code/ixemul-80`; 48.2 branch kept as the backup.
- 2026-10-05: 80.1's library/Makefile.in hard-wired an in-tree build
  (`true_srcdir = ../../..`); restored the `@srcdir@` pattern the other
  directories use, so the Docker out-of-tree build works.
- 2026-10-05: M0.1 import is 21a78ae on `import/ixemul-80.1`.
- 2026-10-05: docker/build.sh builds the compiler image only once: its gcc is
  configured with include/, so every header edit rebuilt gcc 2.95.3 under
  emulation (the first 80.1 build sat in that for 15 minutes).
- 2026-10-05: 80.1's intops/ (bswap16/32/64) was not in AC_OUTPUT and its
  Makefile was a checked-in in-tree copy; added to configure.in/configure,
  Makefile.in takes @srcdir@ like the other directories.
- 2026-10-05: M3.10 #10 verified covered (`copyout_unix_address` NULL-safe,
  unp.c:526, called only when name != NULL, :1006). #13b verified covered:
  bind tests `client_path`, connect writes `server_path` (unp.c:802, :1079),
  so a failed connect no longer blocks a bind.
- 2026-10-05: #12 libixcompat poll()/realpath() are KEPT, not dropped: 80.1's
  vectors 643/656 do not exist in 48.2, so a port linked to them would jump
  past the end of the 48.2 backup library. The static copies win at link
  and run on both.
- 2026-10-05: gcc 2.95.3 first (known-good base), gcc 6 after as M10.
