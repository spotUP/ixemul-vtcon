---
date: 2026-10-05
topic: Did bebbo's faulty gcc 6.5 btst miscompile anything ixemul-vtcon builds?
tags: [gcc, m68k, btst, libixcompat, toolchain]
status: final
---

# The btst miscompile in bebbo's gcc 6.5 cc1 and ixemul-vtcon

**Answer: nothing this repository ships was affected.** Measured below.

## The bug
Until 2026-10-05 the installed cc1 (`~/opt/amiga/libexec/gcc/m68k-amigaos/6.5.0b/cc1`)
had m68k.md's `*tst_bftst_mem{,1,2}` emit `btst #(7-P),<mem>` on the first byte: a one-bit
test at position P >= 8 of an int/short in memory tested bit (7-P) & 7 of the first byte
(`flags & 1` on an int argument became `btst #-24,(16,a5)`, bit 24). Fixed in gcc commit
34f82c7cf (`~/Code/cpython-amiga/build/gcc/src`); the faulty binary is kept as
`cc1.orig-2026-10-05`. The patterns need `TARGET_68020 && TARGET_BITFIELD` (gcc/config/m68k/m68k.md:6146 in that tree),
so only `-m68020` (and up) code can contain it.

## What this repository builds, and with what
- **ixemul.library / ixnet.library** (`docker/build.sh`, output `build295/`): gcc 2.95.3 in
  the `ixemul-gcc295` Docker image (`docker/Dockerfile`), not bebbo's cc1. Not affected.
- **compat/libixcompat.a** (`compat/Makefile`): bebbo's gcc 6.5, `-mcrt=ixemul -O2 -Wall`, no
  CPU flag, so 68000 code (the driver predefines only `__mc68000__`). Measured anyway:
  the 16 objects compiled to assembly with the faulty cc1 (`-B` to a directory holding it)
  and the fixed one, compared per function: **16 files, 101 functions, no difference**. The
  same 16 with `-m68020` added (as a port might build it): also 101 functions, no difference.
  Headers: the overlay `docker/install-sdk-headers.sh` makes (SDK=<scratch dir>), as
  ~/Code/upterm-ports does.
- **build68020/** (32efe2b, ixemul.library with gcc 6, 68020/68881): built 2026-09-29 by the
  faulty cc1; f354e6f records that this build crashed on the rig and was replaced by the gcc
  2.95 one. Not measured (not shipped, and its make.log ends in errors); the btst bug is one
  candidate for that crash, unconfirmed.

## Rebuild
`make -C compat libixcompat.a CPPFLAGS=-I<overlay> VTCON=~/Code/vtcon` after deleting the 16
objects: libixcompat.a 46,180 bytes before and after. 14 objects byte-identical; c99.o and
wchar.o differ in bytes but their disassembly (`objdump -d -r`) equals the faulty compiler's
output (the assembler is not byte-deterministic). `make -C compat test`: 0 failures.
Not installed into the SDK (`~/opt/amiga/m68k-amigaos/ixemul/lib/libixcompat.a` is still the
4,656-byte 2026-09-30 copy; upterm-ports links its own sysroot copy by path).
