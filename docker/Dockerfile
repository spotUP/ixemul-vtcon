# gcc 2.95.3 + binutils 2.14 for m68k-amigaos: the compiler ixemul 48.2 was
# built with, so a rebuilt ixemul.library keeps the ABI every GG-built Unix
# port expects. Only the two recipes of adtools/amigaos-cross-toolchain that
# matter here, with its flags (its full script needs dead 2016 download
# links). Ubuntu 16.04 amd64 with 32-bit gcc 5, as that project requires.
# Build from the repo root:  docker build -f docker/Dockerfile -t ixemul-gcc295 .
FROM --platform=linux/amd64 ubuntu:16.04
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
      gcc gcc-multilib g++-multilib libc6-dev-i386 make perl git patch gperf \
      bison flex texinfo gettext ca-certificates file lhasa \
      && rm -rf /var/lib/apt/lists/*
ENV PREFIX=/opt/m68k-amigaos
RUN git clone --depth 1 https://github.com/adtools/amigaos-binutils-2.14 /src/binutils && \
    git clone --depth 1 https://github.com/cahirwpz/amigaos-gcc-2.95.3 /src/gcc
# generated parsers and scanners must look newer than their sources, or the
# build tries to regenerate them with tools that no longer understand them
RUN for d in /src/binutils /src/gcc; do \
      find $d -name '*.[ly]' | while read f; do b=${f%.*}; \
        for g in $b.c $b.h; do [ -f $g ] && touch $g; done; done; done; true
ENV CC="gcc -m32 -std=gnu11" CXX="g++ -m32 -std=gnu++11" CFLAGS="-g -O2 -w" CXXFLAGS="-g -O2 -w"
RUN mkdir -p /build/binutils && cd /build/binutils && \
    /src/binutils/configure --prefix=$PREFIX --disable-nls \
      --host=i686-linux-gnu --target=m68k-amigaos && \
    make -j4 && make install-binutils install-gas install-ld
ENV PATH=$PREFIX/bin:$PATH
COPY include /src/ixemul-include
RUN touch /src/gcc/gcc/c-parse.gperf /src/gcc/gcc/configure && \
    mkdir -p /build/gcc && cd /build/gcc && \
    /src/gcc/configure --prefix=$PREFIX --host=i686-linux-gnu --build=i686-linux-gnu \
      --target=m68k-amigaos --enable-languages=c --enable-version-specific-runtime-libs \
      --with-headers=/src/ixemul-include && \
    make all-gcc MAKEINFO=true CFLAGS_FOR_TARGET=-noixemul && \
    make -C gcc install-common install-headers install-libgcc install-driver install-cpp MAKEINFO=true \
      CFLAGS_FOR_TARGET=-noixemul   # install-normal without the manuals
