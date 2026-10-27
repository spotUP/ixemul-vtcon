# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# The product has no AmiTCP: volume. Any "AmiTCP:" string in ixnet/ code makes a
# library load (run by ramlib, with ramlib's own pr_WindowPtr) raise "Please
# insert volume AmiTCP: in any drive", which blocks nano. No host harness can
# run ixnet, so this checks the source. Run: make -C tests/host test
import glob, os, re, unittest

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")


def code(src):
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return re.sub(r"//[^\n]*", "", src)


class NoRequester(unittest.TestCase):
    def test_no_amitcp_volume_string_in_ixnet(self):
        for path in glob.glob(os.path.join(ROOT, "ixnet", "*.c")):
            src = code(open(path, encoding="latin-1").read())
            self.assertNotIn("AmiTCP:", src, "%s names the AmiTCP: volume" % os.path.basename(path))

    def test_usergroup_opened_by_plain_name(self):
        src = open(os.path.join(ROOT, "ixnet", "ixnet_open.c"), encoding="latin-1").read()
        self.assertIn('OpenLibrary("usergroup.library"', src)


if __name__ == "__main__":
    unittest.main()
