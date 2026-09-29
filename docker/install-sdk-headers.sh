#!/bin/sh
# Put the public headers this repo changed since 48.2 (the first commit)
# into the ixemul SDK the ports compile against, so programs see what the
# UP-Term library provides (CMSG_LEN, ...). Each SDK file is kept once as
# <name>.orig. SDK: $SDK, default bebbo's install.
set -e
cd "$(dirname "$0")/.."
SDK=${SDK:-$HOME/opt/amiga/m68k-amigaos/ixemul/include}
BASE=$(git rev-list --max-parents=0 HEAD)
for f in $(git diff --name-only "$BASE" -- include/); do
  rel=${f#include/}
  dst="$SDK/$rel"
  [ -f "$dst" ] && [ ! -f "$dst.orig" ] && cp "$dst" "$dst.orig"
  mkdir -p "$(dirname "$dst")"
  cp "$f" "$dst"
  echo "installed $rel"
done
