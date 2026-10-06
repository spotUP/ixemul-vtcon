# libixcompat.a: libc functions ixemul.library 48.2 has no vectors for,
# as a static library the ports link (UP-Term P7). Built with bebbo's gcc
# against the SDK (install the headers first: docker/install-sdk-headers.sh,
# or point CPPFLAGS at an overlay of them, as ~/Code/upterm-ports does).
#   make -C compat            -> compat/libixcompat.a
#   make -C compat install    -> into the SDK's lib directory
#   make -C compat test       -> host tests of the wide-character core
#   make -C compat spawnprobe -> test/spawnprobe, the posix_spawn rig probe
#   make -C compat tables     -> regenerate uclass_tab.h (gen_uclass.py)
# wcwidth includes vtcon's engine/vtwidth.h (VTCON), the terminal's table.
AGCC ?= $(HOME)/opt/amiga/bin/m68k-amigaos-gcc
XAR  ?= $(HOME)/opt/amiga/bin/m68k-amigaos-ar
SDK  ?= $(HOME)/opt/amiga/m68k-amigaos/ixemul
UPTERM_ROOT ?= $(abspath $(CURDIR)/../..)
VTCON ?= $(UPTERM_ROOT)/vtcon
CPPFLAGS ?=
HOSTCC ?= cc
OBJS = poll.o realpath.o time_r.o if_nametoindex.o libgen.o c99.o selfpath.o langinfo.o \
       eprintf.o spawn.o locale.o utf8.o uclass.o wchar.o wctype.o wcwidth.o strbuiltin.o

# C95's wide string functions, which gnulib takes for granted (GNU grep
# 3.12 failed to link on wcscat, wmemcpy): the self-contained members of the
# toolchain's newlib libc.a, as cpython-amiga's libnewlibwcs.a has them
# (their only references are memcpy/memmove, ixemul's; checked with nm -u)
NEWLIBC   ?= $(dir $(AGCC))../m68k-amigaos/lib/libc.a
WCS_FUNCS  = wcscat wcschr wcscmp wcscoll wcscpy wcscspn wcslcpy wcslen wcsncat \
             wcsncmp wcsncpy wcsnlen wcspbrk wcsrchr wcsspn wcsstr wcstok wcsxfrm \
             wmemchr wmemcmp wmemcpy wmemmove wmemset
WCS_OBJS   = $(addprefix newlibwcs/lib_a-,$(addsuffix .o,$(WCS_FUNCS)))

libixcompat.a: $(OBJS) $(WCS_OBJS)
	rm -f $@
	$(XAR) rcs $@ $(OBJS) $(WCS_OBJS)

$(WCS_OBJS): $(NEWLIBC)
	rm -rf newlibwcs && mkdir -p newlibwcs
	cd newlibwcs && $(XAR) x $(NEWLIBC) $(notdir $(WCS_OBJS))

%.o: %.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -c -o $@ $<

uclass.o: uclass_tab.h ixc_wide.h
utf8.o locale.o wchar.o wctype.o: ixc_wide.h
wcwidth.o: wcwidth.c $(VTCON)/engine/vtwidth.h
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -I$(VTCON)/engine -c -o $@ wcwidth.c

tables:
	python3 gen_uclass.py $(VTCON)/engine/vtwidth.h

# host: the codec and the classes (against Python's unicodedata), wcwidth
test: test/wide_test test/wcwidth_test
	./test/wide_test
	./test/wide_test dump | python3 test/check_uclass.py \
	  $$(sed -n 's/^#define IXC_UNICODE_VERSION "\(.*\)"/\1/p' uclass_tab.h)
	./test/wcwidth_test
test/wide_test: test/wide_test.c utf8.c uclass.c uclass_tab.h ixc_wide.h
	$(HOSTCC) -Wall -O2 -o $@ test/wide_test.c utf8.c uclass.c
test/wcwidth_test: test/wcwidth_test.c wcwidth.c $(VTCON)/engine/vtwidth.h
	$(HOSTCC) -Wall -O2 -I$(VTCON)/engine -o $@ test/wcwidth_test.c wcwidth.c

spawnprobe: test/spawnprobe
# libixcompat.a by its path: the driver searches the SDK's lib (an older
# copy) before any -L given here
test/spawnprobe: test/spawnprobe.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/spawnprobe.c libixcompat.a

install: libixcompat.a
	cp libixcompat.a $(SDK)/lib/libixcompat.a

clean:
	rm -rf $(OBJS) newlibwcs libixcompat.a test/wide_test test/wcwidth_test test/spawnprobe
.PHONY: tables test spawnprobe install clean
