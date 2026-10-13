# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
import re, subprocess, sys, os
AM = os.path.expanduser("~/opt/amiga/bin/m68k-amigaos-objdump")
def defs(path):
    return {m.group(1): int(m.group(2)) for m in re.finditer(r"SYSTEM_CALL\s*\((\w+)\s*,\s*(\d+)\)", open(path).read())}
def stubs(libc):
    dis = subprocess.run([AM, "-d", libc], capture_output=True, text=True).stdout
    cur = None; out = {}
    for line in dis.splitlines():
        m = re.match(r"^[0-9a-f]+ <(_\w+)>:", line)
        if m: cur = m.group(1)[1:]; continue
        m = re.search(r"jmp a0@\((-\d+)\)", line)
        if m and cur and cur not in out: out[cur] = -int(m.group(1)) // 6 - 4
    return out
d48 = defs(sys.argv[1]); s = stubs(sys.argv[2])
same = [n for n in s if n in d48 and d48[n] == s[n]]
moved = [(n, d48[n], s[n]) for n in s if n in d48 and d48[n] != s[n]]
new = sorted((s[n], n) for n in s if n not in d48)
print("stubs:", len(s), "same vector as 48.2:", len(same), "moved:", moved, "beyond 48.2 table:", len(new))
print(" ", new[:60])
