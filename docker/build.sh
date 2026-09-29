#!/bin/sh
# Build ixemul.library (68020/68881) with gcc 2.95.3 in the ixemul-gcc295
# image (docker/Dockerfile). Output: build295/library/68020/68881/amigaos/.
# Needs bebbo's amiga-gcc in ~/opt/amiga for the NDK 3.2 headers and
# libamiga.a (the GG libamiga-bin archive is no longer downloadable).
set -e
cd "$(dirname "$0")/.."
PATH=/Applications/Docker.app/Contents/Resources/bin:$PATH
docker build -f docker/Dockerfile -t ixemul-gcc295 . >/dev/null
rm -rf build295 && mkdir build295
docker run --rm --platform linux/amd64 \
  -v "$PWD":/ix \
  -v "$HOME/opt/amiga/m68k-amigaos/ndk-include":/ndk:ro \
  -v "$HOME/opt/amiga/m68k-amigaos/ixemul":/ixsdk:ro \
  -v "$HOME/opt/amiga/m68k-amigaos/lib":/amigalib:ro \
  ixemul-gcc295 sh -c '
    cc="m68k-amigaos-gcc -idirafter /ix/compat-include -idirafter /ndk -B/ixsdk/lib/ -L/ixsdk/lib -L/amigalib"
    cd /ix/build295 &&
    CC="$cc" CPP="m68k-amigaos-gcc -I/inlines/include -idirafter /ix/compat-include -idirafter /ndk -E" \
      sh /ix/configure --host=m68k-amigaos --target=m68k-amigaos --build=i686-pc-linux-gnu >configure.log &&
    make CPU-FPU-TYPES="68020.68881" OTHER_CFLAGS="-static -fomit-frame-pointer -Wall" \
      RANLIB=m68k-amigaos-ranlib AR=m68k-amigaos-ar CC="$cc" \
      $(for d in glue stack static net db; do for b in 68000.soft-float.baserel 68020.soft-float.baserel32 68000.soft-float.no-baserel; do echo $d.$b; done; done) \
      ixnet.68020.soft-float.amigaos \
      $(for d in general string stdlib stdio library; do echo $d.68020.68881.amigaos; done) \
      >make.log 2>&1'
ls -l build295/library/68020/68881/amigaos/ixemul.library build295/ixnet/68020/amigaos/ixnet.library
