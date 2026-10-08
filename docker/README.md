# Building ixemul.library and ixnet.library for UP-Term

    sh docker/build.sh

Builds the Docker image `ixemul-gcc295` once (gcc 2.95.3 and binutils 2.14 for
m68k-amigaos on Ubuntu 16.04 amd64: network needed, long under emulation on an
Apple-silicon Mac; `IMAGE_REBUILD=1` rebuilds it, `KEEP=1` keeps `build295/` for
a `.c`-only rebuild), then builds into `build295/`:

- `build295/library/68020/68881/amigaos/ixemul.library` (vtcon's `IXEMUL_LIB`)
- `build295/ixnet/68020/amigaos/ixnet.library` (vtcon's `IXNET_LIB`)

It reads three directories from bebbo's gcc build (`~/opt/amiga/m68k-amigaos/`:
`ndk-include`, `ixemul`, `lib`), so build amiga-gcc first (`upterm/toolchain/README.md`).
Needs Docker (`/Applications/Docker.app` or `docker` on `PATH`; the script adds
the Docker.app path). After it:

    sh docker/install-sdk-headers.sh        # this tree's public header changes into the SDK ($SDK, default ~/opt/amiga/m68k-amigaos/ixemul/include), originals kept as *.orig
    make -C compat && make -C compat install    # libixcompat.a into the SDK's lib

Host tests of the library's pure parts (the argument line splitter,
`library/cli_args.c`): `make -C tests/host test`; vtcon's `make test` runs
them too (`ONLY=ixemul` alone).

Everything that links with `-mcrt=ixemul` (tmux, screen, python, neovim, vsh's
helpers) needs those two steps once. The branch UP-Term uses is
`feature/ixemul-80` (pinned in `upterm/repos.lock`). The rest of this repo's
README is the upstream ixemul text. The path from an empty Mac:
`upterm/README.md`, "Set up the whole thing".
