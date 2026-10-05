#!/bin/sh
# mixlink.sh OUT PART...: link ixemul.library from the gcc 16 build
# (buildgcc16) with the named parts taken from the gcc 2.95 build
# (build295) -- to find which objects a compiler gets wrong. Parts: the
# library.a members (a4 __ ah ii jo pr ss tz hwck trap ix_timer debugstub),
# start, tracecntl, and the archives string general intops stdlib stdio glue.
set -e
out=$1; shift
R=$(cd "$(dirname "$0")/.." && pwd)
N=$R/${NEW:-buildgcc16}; O=$R/${OLD:-build295}
L=library/68020/68881/amigaos
w=$(mktemp -d); trap 'rm -rf "$w"' EXIT
from() { for p in $PARTS; do [ "$p" = "$1" ] && { echo "$O"; return; }; done; echo "$N"; }
PARTS="$*"
cd "$w"
mkdir lib && (cd lib && m68k-amigaos-ar x "$N/$L/library.a")
for m in a4 __ ah ii jo pr ss tz hwck trap ix_timer debugstub; do
  if [ "$(from $m)" = "$O" ]; then (cd lib && m68k-amigaos-ar x "$O/$L/library.a" $m.o); fi
done
# A4OBJS=dir: the -ffixed-a4 group as one object per file from dir
# instead of a4.o (to tell its seven files apart)
if [ -n "$A4OBJS" ]; then rm -f lib/a4.o; cp "$A4OBJS"/*.o lib/; fi
# GROUP=name GROUPOBJS=dir: the same for any library.a group (__ ah ii ...)
if [ -n "$GROUPOBJS" ]; then rm -f lib/$GROUP.o; cp "$GROUPOBJS"/*.o lib/; fi
# an archive takes the object format of its first member: gcc 2.95's
# objects are a.out-amiga, gcc 6+'s hunk, so the a.out ones go first
aout=; hunk=
for o in lib/*.o; do
  case "$(m68k-amigaos-objdump -f "$o" 2>/dev/null | sed -n 's/.*file format //p')" in
    a.out-amiga) aout="$aout $o";; *) hunk="$hunk $o";;
  esac
done
m68k-amigaos-ar rc library.a $aout $hunk
for a in start tracecntl; do cp "$(from $a)/$L/$a.o" .; done
arch() { echo "$(from $1)/$1/$2/lib$1.a"; }
m68k-amigaos-gcc -s -nostdlib -nostartfiles -Xlinker -u -Xlinker ___load_seg start.o tracecntl.o \
  -Wl,--start-group library.a $(arch string 68020/68881/amigaos) $(arch general 68020/68881/amigaos) \
  $(arch intops 68020/68881/amigaos) $(arch stdlib 68020/68881/amigaos) $(arch stdio 68020/68881/amigaos) \
  "$(from glue)/glue/no-baserel/libglue.a" library.a -lgcc -lamiga -Wl,--end-group -o "$out"
ls -l "$out"
