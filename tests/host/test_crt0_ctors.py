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

    def test_variants_present(self):
        self.assertTrue(any(os.path.exists(os.path.join(CRT0_DIR, n)) for n in VARIANTS), CRT0_DIR)


if __name__ == "__main__":
    unittest.main()
