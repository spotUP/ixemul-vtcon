#!/bin/sh
# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# Build ixemul.library with gcc 2.95.3 in the ixemul-gcc295 image
# (docker/Dockerfile), twice: 68020/68881 for machines with an FPU, and
# 68020/soft-float for those without one (a 68LC060, a 68EC020: the 68881
# build stops at startup there, "math emulation code not found"). Output:
# build295/library/68020/{68881,soft-float}/amigaos/.
# Needs bebbo's amiga-gcc in ~/opt/amiga for the NDK 3.2 headers and
# libamiga.a (the GG libamiga-bin archive is no longer downloadable).
set -e
cd "$(dirname "$0")/.."
PATH=/Applications/Docker.app/Contents/Resources/bin:$PATH
# The image's gcc is configured with this tree's include/ (--with-headers),
# so any header edit would rebuild gcc 2.95.3 under amd64 emulation. The
# library's own Makefiles put include/ first anyway: build the image once,
# or again with IMAGE_REBUILD=1 after a Dockerfile change.
if [ -n "$IMAGE_REBUILD" ] || ! docker image inspect ixemul-gcc295 >/dev/null 2>&1; then
  docker build -f docker/Dockerfile -t ixemul-gcc295 . >/dev/null
fi
# KEEP=1: build into the existing build295 (make redoes what changed;
# only for .c edits: all.c does not depend on headers, and a configure.in,
# Makefile.in or header change needs the full build)
[ -n "$KEEP" ] && [ -f build295/config.status ] || { rm -rf build295 && mkdir build295; }
# /inlines/include, first in the include path of configure and of every compile: GG's fd2inline 1.11
# headers used to be downloaded into the image (the file is gone, 2026-10-13). The NDK's own
# inline/*.h stand in, without the out-of-line stdarg wrappers that name the library base as a global
# (gcc 2.95 emits an unused static function, and it fails to link: ixnet's ix_panic.c has
# IntuitionBase as a local). The wrappers that take the base as a parameter stay.
INL="$PWD/build-inlines"
rm -rf "$INL" && mkdir -p "$INL/include/inline"
for f in "$HOME"/opt/amiga/m68k-amigaos/ndk-include/inline/*.h; do
  LC_ALL=C awk '/^AMIGA_VA_WRAPPER_ATTR$/ { held = $0; next } held != "" { if ($0 ~ /const __base/) print held; else skip = 1; held = "" } skip { if ($0 == "}") skip = 0; next } { print }' "$f" \
    > "$INL/include/inline/$(basename "$f")"
done
docker run --rm --platform linux/amd64 \
  -v "$PWD":/ix \
  -v "$INL/include":/inlines/include:ro \
  -v "$HOME/opt/amiga/m68k-amigaos/ndk-include":/ndk:ro \
  -v "$HOME/opt/amiga/m68k-amigaos/ixemul":/ixsdk:ro \
  -v "$HOME/opt/amiga/m68k-amigaos/lib":/amigalib:ro \
  ixemul-gcc295 sh -c '
    cc="m68k-amigaos-gcc -idirafter /ix/compat-include -idirafter /ndk -B/ixsdk/lib/ -L/ixsdk/lib -L/amigalib"
    cd /ix/build295 &&
    { [ -f config.status ] || CC="$cc" CPP="m68k-amigaos-gcc -I/inlines/include -idirafter /ix/compat-include -idirafter /ndk -E" \
      sh /ix/configure --host=m68k-amigaos --target=m68k-amigaos --build=i686-pc-linux-gnu >configure.log; } &&
    make CPU-FPU-TYPES="68020.68881 68020.soft-float" OTHER_CFLAGS="-static -fomit-frame-pointer -Wall" \
      RANLIB=m68k-amigaos-ranlib AR=m68k-amigaos-ar CC="$cc" \
      $(for d in glue stack static net db; do for b in 68000.soft-float.baserel 68020.soft-float.baserel32 68000.soft-float.no-baserel; do echo $d.$b; done; done) \
      ixnet.68020.soft-float.amigaos \
      $(for d in general intops string stdlib stdio library; do echo $d.68020.68881.amigaos $d.68020.soft-float.amigaos; done) \
      >make.log 2>&1'
ls -l build295/library/68020/68881/amigaos/ixemul.library build295/library/68020/soft-float/amigaos/ixemul.library build295/ixnet/68020/amigaos/ixnet.library
