#!/usr/bin/env python3
# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# fpret.py: libc.a glue for programs compiled for the FPU (-m68881).
#
# ixemul.library is built soft-float: a function returning a double leaves it
# in d0/d1. Code compiled with -m68881 reads a double result from fp0, so
# every such call through libc.a's glue (strtod, atof, pow, ldexp, ...)
# returned whatever fp0 last held. The plain glue jumps into the library
# (`movel _ixemulbase,a0; jmp a0@(-(vec+4)*6)`); the FPU glue calls it
# instead, with a copy of the arguments, and moves d0/d1 into fp0 on the way
# back. Soft-float programs keep the plain glue: the FPU archives are
# installed only in the libm020/libm881 directories, which the drivers search
# for -m68881 alone.
#
# Which functions: every ixemul vector (sys/syscall.def) whose prototype in
# the tree's headers returns double or float, found with gcc -aux-info, so a
# new vector is covered when its prototype is. The argument bytes come from
# the same prototype. libc.a also holds C code that returns doubles
# (static/rand48.c: drand48, erand48); "check" fails when that set changes,
# because those members are rebuilt from source, not given glue.
#
#   fpret.py gen OUTDIR FLAVOUR   one .s per function (FLAVOUR: abs, base16, base32)
#   fpret.py list                 the functions and their argument bytes
#   fpret.py check LIBC.A         the C members of LIBC.A returning a double are the known ones
#   fpret.py verify DIS NAME...   in a linked program's objdump -d, each NAME leaves its result in fp0
# AGCC (the m68k-amigaos-gcc to run) and INCLUDE (the tree's include dir)
# come from the environment.
import os, re, subprocess, sys, tempfile

AGCC = os.environ.get("AGCC", "m68k-amigaos-gcc")
INCLUDE = os.environ.get("INCLUDE", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "include"))
HEADERS = ["math.h", "stdlib.h", "stdio.h", "string.h", "time.h", "unistd.h", "wchar.h",
           "inttypes.h", "locale.h", "signal.h", "search.h"]
# C code in libc.a that returns a double; static/rand48.c is rebuilt for the FPU
C_MEMBERS = {"drand48": "rand48.o", "erand48": "rand48.o"}
# argument types and their size on the stack
ARG_BYTES = {"double": 8, "int": 4, "long int": 4, "time_t": 4}
# the base register load for each library flavour (libc.a, libb, libb32)
BASE_LOAD = {
    "abs": "movel _ixemulbase,a0",
    "base16": "movel a4@(_ixemulbase:W),a0",
    "base32": "movel a4@(_ixemulbase:L),a0",
}


def run(args, **kw):
    return subprocess.run(args, check=True, capture_output=True, text=True, **kw).stdout


def vectors():
    """name -> vector number, from sys/syscall.def"""
    with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as f:
        f.write('#define SYSTEM_CALL(f, v) SC f v\n#include "sys/syscall.def"\n')
    try:
        out = run([AGCC, "-E", "-P", "-I" + INCLUDE, f.name])
    finally:
        os.unlink(f.name)
    vec = {}
    for line in out.splitlines():
        w = line.split()
        if len(w) == 3 and w[0] == "SC":
            vec[w[1]] = int(w[2])
    return vec


def float_prototypes():
    """name -> list of argument types, for every declaration returning double or float"""
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "all.c")
        aux = os.path.join(d, "aux.txt")
        with open(src, "w") as f:
            f.writelines("#include <%s>\n" % h for h in HEADERS)
        run([AGCC, "-mcrt=ixemul", "-I" + INCLUDE, "-fsyntax-only", "-w", "-aux-info", aux, src])
        text = open(aux).read()
    protos = {}
    for m in re.finditer(r"^/\*[^*]*\*/ extern (?:long )?(?:double|float) (\w+) \((.*)\);$", text, re.M):
        args = [a.strip() for a in m.group(2).split(",")]
        protos[m.group(1)] = [] if args == ["void"] else args
    return protos


def arg_bytes(name, args):
    n = 0
    for a in args:
        if a.endswith("*"):
            n += 4
        elif a in ARG_BYTES:
            n += ARG_BYTES[a]
        else:
            sys.exit("fpret.py: %s: no stack size for argument type '%s'" % (name, a))
    return n


def table():
    """[(name, vector, argument bytes)] of the glue that returns a double"""
    vec = vectors()
    protos = float_prototypes()
    return sorted((n, vec[n], arg_bytes(n, a)) for n, a in protos.items() if n in vec)


def stub(name, vec, nbytes, flavour):
    lines = ["\t.text", "\t.even", "\t.globl\t_%s" % name, "_%s:" % name]
    # the library function sees its arguments above a return address, as
    # after the plain glue's jmp: copy them below ours, last word first
    lines += ["\tmovel\tsp@(%d),sp@-" % nbytes] * (nbytes // 4)
    lines += ["\t%s" % BASE_LOAD[flavour],
              "\tjsr\ta0@(%d:W)" % (-(vec + 4) * 6),
              "\tlea\tsp@(%d),sp" % nbytes,
              # d0 holds the high word: it goes to the lower address
              "\tmovel\td1,sp@-",
              "\tmovel\td0,sp@-",
              "\tfmoved\tsp@+,fp0",
              "\trts", ""]
    return "\n".join(lines)


def check(libc):
    """the C members of libc that define a function returning double are the known ones"""
    protos = float_prototypes()
    vec = vectors()
    nm = run([os.path.join(os.path.dirname(AGCC), "m68k-amigaos-nm"), libc])
    member, found = None, {}
    for line in nm.splitlines():
        if line.endswith(".o:"):
            member = line[:-1]
            continue
        w = line.split()
        if len(w) == 3 and w[1] == "T" and w[2].startswith("_"):
            name = w[2][1:]
            if name in protos and name not in vec:
                found[name] = member
    if found != C_MEMBERS:
        sys.exit("fpret.py: %s: C members returning a double %s, expected %s" % (libc, found, C_MEMBERS))
    print("fpret: %s: %d glue functions, C members %s" % (libc, len(table()), " ".join(sorted(set(found.values())))))


FP0_WRITE = re.compile(r"\tf\w+\s+\S*,%fp0$")
TAIL_JUMP = re.compile(r"\tj(?:mp|ra)\s+\w+ <(\w+)>$")


def verify(dis, names):
    """each of names, in the objdump -d listing dis of a linked program,
    writes fp0 itself or tail-jumps to a function that does"""
    bodies, cur = {}, None
    for line in open(dis):
        m = re.match(r"[0-9a-f]+ <(\w+)>:$", line)
        if m:
            cur = bodies.setdefault(m.group(1), [])
        elif cur is not None:
            cur.append(line.rstrip("\n"))

    def returns_fp0(sym, seen):
        body = bodies.get(sym, [])
        if any(FP0_WRITE.search(l) for l in body):
            return True
        return any(m.group(1) not in seen and returns_fp0(m.group(1), seen | {m.group(1)})
                   for m in (TAIL_JUMP.search(l) for l in body) if m)

    bad = [n for n in names if not returns_fp0("_" + n, {"_" + n})]
    if bad:
        sys.exit("fpret.py: %s: no fp0 result from %s" % (dis, " ".join(bad)))
    print("fpret: %s: %d functions return in fp0" % (dis, len(names)))


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "gen":
        out, flavour = sys.argv[2], sys.argv[3]
        if flavour not in BASE_LOAD:
            sys.exit("fpret.py: flavour must be one of %s" % " ".join(BASE_LOAD))
        os.makedirs(out, exist_ok=True)
        for name, vec, nbytes in table():
            with open(os.path.join(out, name + ".s"), "w") as f:
                f.write(stub(name, vec, nbytes, flavour))
    elif len(sys.argv) == 2 and sys.argv[1] == "list":
        for name, vec, nbytes in table():
            print(name, vec, nbytes)
    elif len(sys.argv) == 3 and sys.argv[1] == "check":
        check(sys.argv[2])
    elif len(sys.argv) >= 4 and sys.argv[1] == "verify":
        verify(sys.argv[2], sys.argv[3:])
    else:
        sys.exit("usage: fpret.py gen OUTDIR abs|base16|base32 | list | check LIBC.A | verify DIS NAME...")


if __name__ == "__main__":
    main()
