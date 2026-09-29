# libixcompat.a: libc functions ixemul.library 48.2 has no vectors for,
# as a static library the ports link (UP-Term P7). Built with bebbo's gcc
# against the SDK (install the headers first: docker/install-sdk-headers.sh).
#   make -C compat            -> compat/libixcompat.a
#   make -C compat install    -> into the SDK's lib directory
AGCC ?= $(HOME)/opt/amiga/bin/m68k-amigaos-gcc
XAR  ?= $(HOME)/opt/amiga/bin/m68k-amigaos-ar
SDK  ?= $(HOME)/opt/amiga/m68k-amigaos/ixemul
OBJS = poll.o realpath.o

libixcompat.a: $(OBJS)
	rm -f $@
	$(XAR) rcs $@ $(OBJS)

%.o: %.c
	$(AGCC) -mcrt=ixemul -O2 -Wall -c -o $@ $<

install: libixcompat.a
	cp libixcompat.a $(SDK)/lib/libixcompat.a

clean:
	rm -f $(OBJS) libixcompat.a
