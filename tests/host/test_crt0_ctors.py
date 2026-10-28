# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# Every crt0 variant must walk the a.out constructor and destructor sets.
# Before the fix start_stdio called main directly, so ___CTOR_LIST__ entries
# (libamisslauto's ___init_amissl) never ran: AmiSSLBase stayed NULL and the
# first AmiSSL call froze the machine. A fixed crt0 references both lists,
# contributes one absolute-zero element to each (so programs without
# constructors still link), and registers the destructor walker with atexit.
# Run: CRT0_DIR=<dir with the built crt0 objects> python3 tests/host/test_crt0_ctors.py
import os, subprocess, unittest

CRT0_DIR = os.environ.get("CRT0_DIR", os.path.expanduser("~/opt/amiga/m68k-amigaos/ixemul/lib"))
NM = os.environ.get("NM", os.path.expanduser("~/opt/amiga/bin/m68k-amigaos-nm"))
OBJDUMP = os.environ.get("OBJDUMP", os.path.join(os.path.dirname(NM), "m68k-amigaos-objdump"))
VARIANTS = ("crt0.o", "bcrt0.o", "rcrt0.o", "lcrt0.o", "scrt0.o", "mcrt0.o", "gcrt0.o")


def symbols(obj):
    out = subprocess.run([NM, obj], capture_output=True, text=True, check=True).stdout
    return [line.split() for line in out.splitlines() if line.strip()]


@unittest.skipUnless(os.path.exists(NM), "no m68k nm")
class Crt0Ctors(unittest.TestCase):
    def test_every_variant_walks_both_sets(self):
        for name in VARIANTS:
            obj = os.path.join(CRT0_DIR, name)
            if not os.path.exists(obj):
                continue
            with self.subTest(variant=name):
                syms = symbols(obj)
                undefined = {s[-1] for s in syms if len(s) == 2 and s[0] == "U"}
                seta = {s[-1] for s in syms if "SETA" in s}
                self.assertIn("___CTOR_LIST__", undefined)
                self.assertIn("___DTOR_LIST__", undefined)
                self.assertEqual(seta, {"___CTOR_LIST__", "___DTOR_LIST__"})
                self.assertIn("_atexit", undefined)

    # AmigaOS starts a program at the first byte of its first code hunk, and
    # execve() pattern-matches the same bytes. When crt0.c is compiled with the
    # fd2inline headers in build-inlines, gcc 2.95 emits their static _va
    # wrappers (__EasyRequest_va, ...) ahead of the entry asm, so a program
    # started inside __EasyRequest_va with a garbage a6 and reset the machine.
    def test_every_variant_starts_with_jump_to_entry(self):
        for name in VARIANTS:
            obj = os.path.join(CRT0_DIR, name)
            if not os.path.exists(obj):
                continue
            with self.subTest(variant=name):
                entry = [int(s[0], 16) for s in symbols(obj) if s[-1] == "_ENTRY"]
                self.assertEqual(len(entry), 1)
                dump = subprocess.run([OBJDUMP, "-s", "-j", ".text", obj],
                                      capture_output=True, text=True, check=True).stdout
                row = next(l for l in dump.splitlines() if l.startswith(" 0000 "))
                word0, word1 = int(row[6:10], 16), int(row[10:14], 16)
                self.assertEqual(word0, 0x6000, "offset 0 is not a bra.w")
                self.assertEqual(2 + word1, entry[0], "bra.w at offset 0 does not reach _ENTRY")

    def test_variants_present(self):
        self.assertTrue(any(os.path.exists(os.path.join(CRT0_DIR, n)) for n in VARIANTS), CRT0_DIR)


if __name__ == "__main__":
    unittest.main()
