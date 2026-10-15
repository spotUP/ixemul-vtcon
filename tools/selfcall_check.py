#!/usr/bin/env python3
# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# selfcall_check.py OBJ...: fail when a function in OBJ jumps, branches or
# calls back to its own entry. string/ implements the functions gcc knows as
# builtins; a gcc that folds the implementation into a call to itself
# (memset's loop became `jra _memset`, memmove's bcopy() call `bras
# _memmove`) leaves a library where the first call never returns. None of
# string/'s functions recurse, so any such branch is that bug.
# tests/host/test_selfcall.py runs it from `make -C tests/host test`.
# Usage: python3 tools/selfcall_check.py buildgcc16/string/68020/68881/amigaos/all.o
import os, re, subprocess, sys

OBJDUMP = os.environ.get("OBJDUMP", "m68k-amigaos-objdump")
# a branch, jump or call whose target is <_name>: `bras d6e <_memmove>` (-O2)
# or `jsr %pc@(5a0 <_memmove>)` (-O0, the target inside an addressing mode)
BRANCH = re.compile(r"\t(j\w+|b\w+)\s.*<(_\w+)>\)?\s*$")


def selfcalls(dis):
    """The (function, line) pairs of a disassembly where a function targets its own entry."""
    found = []
    cur = None
    for line in dis.splitlines():
        m = re.match(r"^[0-9a-f]+ <(_[A-Za-z]\w*)>:", line)
        if m:
            cur = m.group(1)
            continue
        m = BRANCH.search(line)
        if cur and m and m.group(2) == cur:
            found.append((cur, line.strip()))
    return found


def main(objs):
    bad = 0
    for obj in objs:
        dis = subprocess.run([OBJDUMP, "-d", obj], capture_output=True, text=True, check=True).stdout
        for cur, line in selfcalls(dis):
            print(f"[ERROR] {obj}: {cur} branches to its own entry: {line}")
            bad += 1
    print("[OK] no function branches to its own entry" if not bad else f"[ERROR] {bad} self-branch(es)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
