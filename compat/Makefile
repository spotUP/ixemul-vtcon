# libixcompat.a: libc functions ixemul.library 48.2 has no vectors for,
# as a static library the ports link (UP-Term P7). Built with bebbo's gcc
# against the SDK (install the headers first: docker/install-sdk-headers.sh,
# or point CPPFLAGS at an overlay of them, as ~/Code/upterm-ports does).
#   make -C compat            -> compat/libixcompat.a
#   make -C compat install    -> into the SDK's lib directory
#   make -C compat spawnprobe -> test/spawnprobe, the posix_spawn rig probe
AGCC ?= $(HOME)/opt/amiga/bin/m68k-amigaos-gcc
XAR  ?= $(HOME)/opt/amiga/bin/m68k-amigaos-ar
SDK  ?= $(HOME)/opt/amiga/m68k-amigaos/ixemul
CPPFLAGS ?=
OBJS = poll.o realpath.o time_r.o if_nametoindex.o libgen.o c99.o selfpath.o langinfo.o \
       spawn.o

libixcompat.a: $(OBJS)
	rm -f $@
	$(XAR) rcs $@ $(OBJS)

%.o: %.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -c -o $@ $<

spawnprobe: test/spawnprobe
# libixcompat.a by its path: the driver searches the SDK's lib (an older
# copy) before any -L given here
test/spawnprobe: test/spawnprobe.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/spawnprobe.c libixcompat.a

install: libixcompat.a
	cp libixcompat.a $(SDK)/lib/libixcompat.a

clean:
	rm -f $(OBJS) libixcompat.a test/spawnprobe
.PHONY: spawnprobe install clean
