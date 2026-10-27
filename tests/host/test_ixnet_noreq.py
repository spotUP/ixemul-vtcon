# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# ixnet_open() must never put up an AmigaDOS "Please insert volume AmiTCP:"
# requester: the product has no AmiTCP: assign, and a full-path OpenLibrary on
# a missing volume blocks the program (nano) on the requester. No host harness
# can run ixnet, so this checks the source: every OpenLibrary of a path that
# names a volume (a colon) in ixnet/ and library/ must sit between a
# pr_WindowPtr = (APTR)-1 and its restore. Run: make -C tests/host test
import glob, os, re, unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
FULLPATH = re.compile(r'OpenLibrary\s*\(\s*"[^"]*:')


class NoRequester(unittest.TestCase):
    def test_volume_path_open_library_suppresses_requesters(self):
        files = glob.glob(os.path.join(ROOT, "ixnet", "*.c")) + glob.glob(os.path.join(ROOT, "library", "*.c"))
        seen = 0
        for path in files:
            src = open(path, encoding="latin-1").read()
            for m in FULLPATH.finditer(src):
                seen += 1
                before = src[:m.start()]
                off = before.rfind("pr_WindowPtr = (APTR)-1")
                restore = before.rfind("pr_WindowPtr = oldwin")
                self.assertTrue(off >= 0 and off > restore and m.start() - off < 600,
                                "%s: OpenLibrary on a volume path without pr_WindowPtr = -1" % os.path.basename(path))
        self.assertGreaterEqual(seen, 1, "the optional AmiTCP: fallback open is gone; update this test")

    def test_usergroup_tries_plain_name_first(self):
        src = open(os.path.join(ROOT, "ixnet", "ixnet_open.c"), encoding="latin-1").read()
        self.assertLess(src.index('OpenLibrary("usergroup.library"'), src.index('"AmiTCP:libs/usergroup.library"'))


if __name__ == "__main__":
    unittest.main()
