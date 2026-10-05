#!/usr/bin/env python3
"""Count an Amiga library's function vectors: load the hunk file, apply
HUNK_RELOC32 with each hunk based at a distinct fake address, find the
RomTag (0x4AFC + self pointer), follow RT_INIT (RTF_AUTOINIT) -> funcTable,
and count longwords until -1. Prints count and the version string."""
import struct, sys

def load(path):
    d = open(path, 'rb').read(); p = 0
    def L():
        nonlocal p; v = struct.unpack('>I', d[p:p+4])[0]; p += 4; return v
    assert L() == 0x3F3
    while L(): pass
    n = L(); first = L(); last = L()
    sizes = [(L() & 0x3FFFFFFF) * 4 for _ in range(last - first + 1)]
    hunks = []; cur = -1; relocs = []
    while p < len(d):
        t = L() & 0x3FFFFFFF
        if t in (0x3E9, 0x3EA):
            nl = L(); cur += 1; hunks.append(bytearray(d[p:p+nl*4]) + bytearray(sizes[cur]-nl*4)); p += nl*4
        elif t == 0x3EB:
            L(); cur += 1; hunks.append(bytearray(sizes[cur]))
        elif t == 0x3EC:
            while True:
                c = L()
                if not c: break
                tgt = L()
                for _ in range(c): relocs.append((cur, tgt, L()))
        elif t == 0x3F0:  # symbols
            while True:
                c = L()
                if not c: break
                p += (c & 0xFFFFFF) * 4 + 4
        elif t == 0x3F1:
            nl = L(); p += nl*4
        elif t == 0x3F2:
            continue
        else:
            raise SystemExit('hunk %x' % t)
    base = []; a = 0x100000
    for h in hunks: base.append(a); a += (len(h) + 0xFFFF) & ~0xFFFF
    for (h, tgt, off) in relocs:
        v = struct.unpack('>I', hunks[h][off:off+4])[0] + base[tgt]
        hunks[h][off:off+4] = struct.pack('>I', v & 0xFFFFFFFF)
    mem = {}
    def rd(addr):
        for i, b in enumerate(base):
            if b <= addr < b + len(hunks[i]):
                return struct.unpack('>I', hunks[i][addr-b:addr-b+4])[0]
        raise KeyError(hex(addr))
    def rds(addr):
        for i, b in enumerate(base):
            if b <= addr < b + len(hunks[i]):
                s = hunks[i][addr-b:]; return s[:s.index(0)].decode('latin1')
    return hunks, base, rd, rds

hunks, base, rd, rds = load(sys.argv[1])
h = hunks[0]
for off in range(0, len(h) - 26, 2):
    if h[off:off+2] == b'\x4a\xfc' and struct.unpack('>I', h[off+2:off+6])[0] == base[0] + off:
        init = struct.unpack('>I', h[off+22:off+26])[0]
        idstr = struct.unpack('>I', h[off+18:off+22])[0]
        ver = h[off+11]
        functab = rd(init + 4)
        n = 0
        while rd(functab + 4*n) != 0xFFFFFFFF: n += 1
        print('%s: RT_VERSION %d, %d vectors (4 std + %d), last LVO -%d, id %r' % (sys.argv[1].split('/')[-1], ver, n, n-4, 6*n, rds(idstr).strip()))
        break
