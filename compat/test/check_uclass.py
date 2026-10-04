#!/usr/bin/env python3
"""Compare `wide_test dump` (stdin) with Python's unicodedata: every code
point's class (the ixc_wide.h mapping of the General_Category) and its
simple upper/lower case mapping. Python's Unicode version must be the
table's. Exit status 1 on any difference, listing the first ten."""
import sys
import unicodedata

CLASSES = ["CN", "LU", "LL", "LO", "M", "ND", "NO", "P", "S", "ZS", "ZL", "CC", "CF", "CS"]
GC = {"Lu": "LU", "Ll": "LL", "Lt": "LO", "Lm": "LO", "Lo": "LO", "Mn": "M", "Mc": "M",
      "Me": "M", "Nd": "ND", "Nl": "NO", "No": "NO", "Zs": "ZS", "Zl": "ZL", "Zp": "ZL",
      "Cc": "CC", "Cf": "CF", "Co": "CF", "Cs": "CS", "Cn": "CN"}
GC.update({g: "P" for g in ("Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po")})
GC.update({g: "S" for g in ("Sm", "Sc", "Sk", "So")})
want_ver = sys.argv[1]
if unicodedata.unidata_version != want_ver:
    sys.exit("check_uclass.py: Python has Unicode %s, the table %s" % (unicodedata.unidata_version, want_ver))


def simple(c, f):
    """the simple (one code point) mapping, as UnicodeData field 12/13"""
    s = f(chr(c))
    return ord(s) if len(s) == 1 else c


bad = []
n = 0
for line in sys.stdin:
    c, k, up, lo = (int(x, 16) for x in line.split())
    n += 1
    ch = chr(c)
    want = (CLASSES.index(GC[unicodedata.category(ch)]),)
    got = (k,)
    # str.upper/lower are the full mappings; where they are one code point
    # they equal the simple ones except for the few with special casing
    # (SpecialCasing.txt), which UnicodeData's simple fields still list
    if want != got:
        bad.append("U+%04X class %s, want %s" % (c, CLASSES[k], CLASSES[want[0]]))
    for name, mine, f in (("upper", up, str.upper), ("lower", lo, str.lower)):
        s = f(ch)
        if len(s) == 1 and ord(s) != mine:
            bad.append("U+%04X %s U+%04X, want U+%04X" % (c, name, mine, ord(s)))
if n != 0x110000:
    bad.append("%d code points dumped, want %d" % (n, 0x110000))
for b in bad[:10]:
    print("[FAIL]", b)
print("%s uclass: %d code points, %d differences from unicodedata %s" % (
    "[OK]  " if not bad else "[FAIL]", n, len(bad), unicodedata.unidata_version))
sys.exit(1 if bad else 0)
