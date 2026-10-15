# Copyright (C) 2026 spotUP.
# This file is part of ixemul-vtcon, a modified ixemul.library. It is free
# software; you can redistribute it and/or modify it under the terms of the
# GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version. See LICENSES.README and COPYING.LIB.
# libixcompat.a: libc functions ixemul.library 48.2 has no vectors for,
# as a static library the ports link (UP-Term P7). Built with bebbo's gcc
# against the SDK (install the headers first: docker/install-sdk-headers.sh,
# or point CPPFLAGS at an overlay of them, as upterm-ports does).
#   make -C compat            -> compat/libixcompat.a
#   make -C compat install    -> into the SDK's lib directory
#   make -C compat test       -> host tests of the wide-character core, strtonum,
#                                reallocarray, isnormal, strptime, the pthread subset, getline
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
OBJS = poll.o realpath.o time_r.o if_nametoindex.o libgen.o $(C99_OBJS) selfpath.o langinfo.o \
       getopt_long.o pselect.o pty.o eprintf.o spawn.o locale.o utf8.o uclass.o wchar.o wctype.o wcwidth.o strbuiltin.o math.o bswap.o wstdio.o nanosleep.o getpgid.o lchown.o \
       strtonum.o reallocarray.o isnormal.o strptime.o pthread.o getline.o

# c99.c, one archive member per function (c99.c says why)
C99_FUNCS = strtoll strtoull ftello fseeko ctime_r strsignal fmod round log2 exp2 lldiv llabs strtoimax strtoumax
C99_OBJS  = $(addprefix c99_,$(addsuffix .o,$(C99_FUNCS)))

# C95's wide string functions, which gnulib takes for granted (GNU grep
# 3.12 failed to link on wcscat, wmemcpy): the self-contained members of the
# toolchain's newlib libc.a, as cpython-amiga's libnewlibwcs.a has them
# (their only references are memcpy/memmove, ixemul's; checked with nm -u)
NEWLIBC   ?= $(dir $(AGCC))../m68k-amigaos/lib/libc.a
WCS_FUNCS  = wcscat wcschr wcscmp wcscoll wcscpy wcscspn wcslcpy wcslen wcsncat \
             wcsncmp wcsncpy wcsnlen wcspbrk wcsrchr wcsspn wcsstr wcstok wcsxfrm \
             wmemchr wmemcmp wmemcpy wmemmove wmemset
# POSIX tsearch/tfind/tdelete/twalk the same way (ncurses 6.6's colour pairs
# failed to link on them under less 710; references: malloc, free)
SEARCH_FUNCS = tsearch tfind tdelete twalk
# POSIX.1-2008 strnlen: the SDK's string.h declares it, so gnulib (nano 9.2's
# fnmatch) took it as present and the link failed (no references); POSIX
# strtok_r, which findutils 4.11 calls (no references)
STR_FUNCS  = strnlen strtok_r
WCS_OBJS   = $(addprefix newlibwcs/lib_a-,$(addsuffix .o,$(WCS_FUNCS) $(SEARCH_FUNCS) $(STR_FUNCS)))

# libm.a: what "-lm" must find in ixemul's library directory. The toolchain's
# m68k-amigaos/lib/libm.a is newlib's: its errno goes through _impure_ptr,
# which ixemul.library does not export, so every "-lm" link failed on
# _impure_ptr (jq's configure, Lua's log2). ixemul's libc.a already holds
# the classic functions (sqrt pow sin ... 27 of them); this archive holds the
# C99 math functions it lacks. ixemul/lib is searched before m68k-amigaos/lib,
# so installing this shadows newlib's.
LIBM_FUNCS = fmod round log2 exp2
LIBM_OBJS  = $(addprefix c99_,$(addsuffix .o,$(LIBM_FUNCS)))
libm.a: $(LIBM_OBJS)
	rm -f $@
	$(XAR) rcs $@ $(LIBM_OBJS)

libixcompat.a: $(OBJS) $(WCS_OBJS)
	rm -f $@
	$(XAR) rcs $@ $(OBJS) $(WCS_OBJS)

$(WCS_OBJS): $(NEWLIBC)
	rm -rf newlibwcs && mkdir -p newlibwcs
	cd newlibwcs && $(XAR) x $(NEWLIBC) $(notdir $(WCS_OBJS))

%.o: %.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -c -o $@ $<

c99_%.o: c99.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -DC99_$* -c -o $@ c99.c

uclass.o: uclass_tab.h ixc_wide.h
utf8.o locale.o wchar.o wctype.o: ixc_wide.h
# the toolchain ships newlib's <pthread.h>, which shadows ixemul's until the SDK headers are
# reinstalled; build against the tree's own header so the struct layout matches
pthread.o: pthread.c ../include/pthread.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -c -o $@ pthread.c

wcwidth.o: wcwidth.c $(VTCON)/engine/vtwidth.h
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -I$(VTCON)/engine -c -o $@ wcwidth.c

tables:
	python3 gen_uclass.py $(VTCON)/engine/vtwidth.h

# host: the codec and the classes (against Python's unicodedata), wcwidth
test: test/wide_test test/wcwidth_test test/math_test test/getopt_test test/nums_test test/strptime_test test/pthread_test test/getline_test test/own_c99_link test/ixc_link test/math_header.o test/inttypes_header.o test/libm_link
	./test/wide_test
	./test/wide_test dump | python3 test/check_uclass.py \
	  $$(sed -n 's/^#define IXC_UNICODE_VERSION "\(.*\)"/\1/p' uclass_tab.h)
	./test/wcwidth_test
	./test/math_test
	./test/getopt_test
	./test/nums_test
	./test/strptime_test
	./test/pthread_test
	./test/getline_test
test/wide_test: test/wide_test.c utf8.c uclass.c uclass_tab.h ixc_wide.h
	$(HOSTCC) -Wall -O2 -o $@ test/wide_test.c utf8.c uclass.c
test/math_test: test/math_test.c math.c
	$(HOSTCC) -Wall -O2 -Datan2=ixc_atan2 -c -o test/math_ixc.o math.c
	$(HOSTCC) -Wall -O2 -o $@ test/math_test.c test/math_ixc.o
test/getopt_test: test/getopt_test.c getopt_long.c ../include/getopt.h
	$(HOSTCC) -Wall -O2 -Dgetopt_long=ixc_getopt_long -Dgetopt_long_only=ixc_getopt_long_only -c -o test/getopt_ixc.o getopt_long.c
	$(HOSTCC) -Wall -O2 -o $@ test/getopt_test.c test/getopt_ixc.o
test/nums_test: test/nums_test.c strtonum.c reallocarray.c isnormal.c
	$(HOSTCC) -Wall -O2 -DIXC_STRTONUM=ixc_strtonum -c -o test/strtonum_ixc.o strtonum.c
	$(HOSTCC) -Wall -O2 -DIXC_REALLOCARRAY=ixc_reallocarray -c -o test/reallocarray_ixc.o reallocarray.c
	$(HOSTCC) -Wall -O2 -DIXC_ISNORMAL=ixc_isnormal -c -o test/isnormal_ixc.o isnormal.c
	$(HOSTCC) -Wall -O2 -o $@ test/nums_test.c test/strtonum_ixc.o test/reallocarray_ixc.o test/isnormal_ixc.o
test/strptime_test: test/strptime_test.c strptime.c
	$(HOSTCC) -Wall -O2 -DIXC_STRPTIME=ixc_strptime -c -o test/strptime_ixc.o strptime.c
	$(HOSTCC) -Wall -O2 -o $@ test/strptime_test.c test/strptime_ixc.o
# the host's <pthread.h> would be found first without ../include; the rename header keeps the two apart
test/getline_test: test/getline_test.c getline.c
	$(HOSTCC) -Wall -O2 -DIXC_GETDELIM=ixc_getdelim -DIXC_GETLINE=ixc_getline -c -o test/getline_ixc.o getline.c
	$(HOSTCC) -Wall -O2 -o $@ test/getline_test.c test/getline_ixc.o
test/pthread_test: test/pthread_test.c pthread.c ../include/pthread.h test/pthread_ixc.h
	$(HOSTCC) -Wall -O2 -include test/pthread_ixc.h -I../include -c -o test/pthread_ixc.o pthread.c
	$(HOSTCC) -Wall -O2 -include test/pthread_ixc.h -I../include -o $@ test/pthread_test.c test/pthread_ixc.o
test/wcwidth_test: test/wcwidth_test.c wcwidth.c $(VTCON)/engine/vtwidth.h
	$(HOSTCC) -Wall -O2 -I$(VTCON)/engine -o $@ test/wcwidth_test.c wcwidth.c

# a program's own definition of a C99 function wins over libixcompat's: each
# is its own archive member (link only; bebbo's gcc, as libixcompat.a)
test/own_c99_link: test/own_c99_link.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/own_c99_link.c libixcompat.a

# "-lm" links sqrt, log2 and pow and the result has no _impure_ptr. -L. puts
# this directory's libm.a ahead of newlib's, as the installed one is
# (link only; bebbo's gcc). The fixed-up search order is asserted too: with
# libm.a installed in ixemul/lib, -print-file-name must name it first.
test/libm_link: test/libm_link.c libm.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -L. -o $@ test/libm_link.c -lm
	@if $(dir $(AGCC))m68k-amigaos-nm $@ | grep -q impure_ptr; then \
	  echo "FAIL: test/libm_link references _impure_ptr"; rm -f $@; exit 1; fi
	@$(dir $(AGCC))m68k-amigaos-nm $@ | grep -q ' T _log2' || { echo "FAIL: no log2 linked"; rm -f $@; exit 1; }

# <math.h>'s C99 constants (compile only, against the repo's headers)
test/math_header.o: test/math_header.c ../include/math.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -c -o $@ test/math_header.c

# getopt_long and pselect resolve from libixcompat.a (link only)
test/ixc_link: test/ixc_link.c libixcompat.a ../include/getopt.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -o $@ test/ixc_link.c libixcompat.a

test/inttypes_header.o: test/inttypes_header.c ../include/inttypes.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -Werror=format -I../include $(CPPFLAGS) -c -o $@ test/inttypes_header.c

spawnprobe: test/spawnprobe
# libixcompat.a by its path: the driver searches the SDK's lib (an older
# copy) before any -L given here
test/spawnprobe: test/spawnprobe.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/spawnprobe.c libixcompat.a

install: libixcompat.a libm.a
	cp libixcompat.a $(SDK)/lib/libixcompat.a
	cp libm.a $(SDK)/lib/libm.a

clean:
	rm -rf $(OBJS) newlibwcs libixcompat.a libm.a test/libm_link test/wide_test test/wcwidth_test test/nums_test test/strptime_test test/pthread_test test/getline_test test/*_ixc.o test/spawnprobe test/own_c99_link test/math_header.o test/ixc_link test/inttypes_header.o
.PHONY: tables test spawnprobe install clean
