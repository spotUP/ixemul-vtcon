#!/usr/bin/env python3
# selfcall_check.py OBJ...: fail when a function in OBJ jumps, branches or
# calls back to its own entry. string/ implements the functions gcc knows as
# builtins; a gcc that folds the implementation into a call to itself
# (memset's loop became `jra _memset`, memmove's bcopy() call `bras
# _memmove`) leaves a library where the first call never returns. None of
# string/'s functions recurse, so any such branch is that bug.
# Usage: python3 tools/selfcall_check.py buildgcc16/string/68020/68881/amigaos/all.o
import os, re, subprocess, sys

OBJDUMP = os.environ.get("OBJDUMP", "m68k-amigaos-objdump")
bad = 0
for obj in sys.argv[1:]:
    dis = subprocess.run([OBJDUMP, "-d", obj], capture_output=True, text=True, check=True).stdout
    cur = None
    for line in dis.splitlines():
        m = re.match(r"^[0-9a-f]+ <(_[A-Za-z]\w*)>:", line)
        if m:
            cur = m.group(1)
            continue
        m = re.search(r"\t(j\w+|b\w+)\s.*<(_\w+)>\s*$", line)
        if cur and m and m.group(2) == cur:
            print(f"[ERROR] {obj}: {cur} branches to its own entry: {line.strip()}")
            bad += 1
print("[OK] no function branches to its own entry" if not bad else f"[ERROR] {bad} self-branch(es)")
sys.exit(1 if bad else 0)
