# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# tools/selfcall_check.py must flag a function that targets its own entry in
# both forms gcc 16 produced for memmove-calls-bcopy (W39): `bras <_memmove>`
# at -O2, `jsr %pc@(.. <_memmove>)` at -O0. Run: make -C tests/host test
# With the toolchain present (AGCC, OBJDUMP) it also compiles the real
# memmove-calls-bcopy source and checks the object file end to end.
import os, subprocess, sys, tempfile, unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools"))
import selfcall_check as sc

O2 = "00000d6e <_memmove>:\n   d70:\t6000 fffc \tbras d6e <_memmove>\n"
O0 = "00000590 <_memmove>:\n 5a0:\t4eba 0000 \tjsr %pc@(590 <_memmove>)\n"
OK = "000000a0 <_bcopy>:\n a2:\t4e75 \trts\n00000590 <_memmove>:\n 5a0:\t4eba 0000 \tjsr %pc@(a0 <_bcopy>)\n"

AGCC = os.environ.get("AGCC", os.path.expanduser("~/opt/amiga16/bin/m68k-amigaos-gcc"))
SRC = "void bcopy(const void *, void *, unsigned long);\nvoid *memmove(void *d, const void *s, unsigned long n) { bcopy(s, d, n); return d; }\n"


class SelfCall(unittest.TestCase):
    def test_flags_bras_to_own_entry(self):
        self.assertEqual([c for c, _ in sc.selfcalls(O2)], ["_memmove"])

    def test_flags_jsr_pc_relative_to_own_entry(self):
        self.assertEqual([c for c, _ in sc.selfcalls(O0)], ["_memmove"])

    def test_accepts_call_to_another_function(self):
        self.assertEqual(sc.selfcalls(OK), [])

    @unittest.skipUnless(os.path.exists(AGCC), "no m68k gcc")
    def test_real_object_with_builtin_folding_fails_and_fno_builtin_passes(self):
        objdump = os.environ.get("OBJDUMP", os.path.join(os.path.dirname(AGCC), "m68k-amigaos-objdump"))
        sc.OBJDUMP = objdump
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "mm.c")
            open(src, "w").write(SRC)
            for opt in ("-O2", "-O0"):
                bad, good = os.path.join(d, "bad.o"), os.path.join(d, "good.o")
                subprocess.run([AGCC, "-m68020", opt, "-c", src, "-o", bad], check=True)
                subprocess.run([AGCC, "-m68020", opt, "-fno-builtin", "-c", src, "-o", good], check=True)
                dis = lambda o: subprocess.run([objdump, "-d", o], capture_output=True, text=True, check=True).stdout
                self.assertTrue(sc.selfcalls(dis(bad)), opt + " without -fno-builtin")
                self.assertEqual(sc.selfcalls(dis(good)), [], opt + " with -fno-builtin")


if __name__ == "__main__":
    unittest.main()
