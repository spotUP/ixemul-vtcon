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
# these files implement functions gcc knows as builtins: without -fno-builtin
# gcc folds an implementation into a call to itself (libm.c's sqrtf, return
# (float)sqrt(x), became `jsr _sqrtf`: endless recursion that overran the
# stack and hung the Amiga). test/selfcall checks the archives for it.
LIBFLAGS = -fno-builtin
HOSTCC ?= cc
OBJS = poll.o realpath.o time_r.o if_nametoindex.o libgen.o $(C99_OBJS) selfpath.o langinfo.o \
       getopt_long.o pselect.o pty.o eprintf.o spawn.o locale.o utf8.o uclass.o wchar.o wctype.o wcwidth.o strbuiltin.o math.o bswap.o wstdio.o nanosleep.o getpgid.o lchown.o lockf.o \
       strtonum.o reallocarray.o isnormal.o strptime.o pthread.o getline.o inet_pton.o inet_ntop.o \
       stralloc.o strlcpy.o gai.o

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
# The double/float/long double functions of libm.c (one member each), and the
# members of newlib's libm.a that need no errno (fdlibm: no w_ wrappers; found
# by closing the undefined symbols of each wanted function over that archive
# and rejecting any that reach _impure_ptr, __fdlib_version or matherr).
# cexp/cimag/creal (and f, l): gcc -O2 turns sin(x) and cos(x) of one argument into a call
# to cexp (sincos -> cexpi -> cexp); their only references are exp, sin, cos, cimag, creal
# (ixemul's libc.a and this archive), no errno.
LIBM_FUNCS = fmod round log2 exp2 \
  hypot acosh atanh remainder lgamma tgamma nexttoward nexttowardf atan2f \
  acosf asinf coshf expf exp2f logf log10f log2f sinhf sqrtf acoshf atanhf lgammaf tgammaf \
  pow powf fdim fdimf sqrtl fmodf remainderf hypotf ldexpf \
  acosl acoshl asinl asinhl atanl atanhl cbrtl ceill cosl coshl erfl erfcl expl exp2l expm1l \
  floorl lgammal logl log10l log1pl log2l logbl nearbyintl rintl roundl sinl sinhl tanl tanhl \
  tgammal truncl fabsl copysignl atan2l fdiml fmaxl fminl fmodl hypotl nextafterl powl \
  remainderl fmal nexttowardl ilogbl lrintl lroundl llrintl llroundl ldexpl scalbnl scalblnl \
  frexpl modfl remquol nanl
LIBM_C99   = fmod round log2 exp2
LIBM_NEWLIB = e_acosh e_atanh e_exp e_fmod e_hypot e_log e_pow e_remainder e_sqrt ef_exp ef_log ef_pow \
  ef_rem_pio2 ef_sqrt er_gamma er_lgamma k_cos k_sin kf_cos kf_rem_pio2 kf_sin kf_tan \
  s_asinh s_cbrt s_copysign s_erf s_expm1 s_fma s_fmax s_fmin s_fpclassify s_ilogb \
  s_llrint s_llround s_log1p s_logb s_lrint s_lround s_nan s_nearbyint s_nextafter s_remquo \
  s_rint s_scalbln s_scalbn s_trunc sf_asinh sf_atan sf_cbrt sf_ceil sf_copysign sf_cos \
  sf_erf sf_expm1 sf_fabs sf_floor sf_fma sf_fmax sf_fmin sf_fpclassify sf_frexp \
  sf_ilogb sf_llrint sf_llround sf_log1p sf_logb sf_lrint sf_lround sf_modf sf_nan \
  sf_nearbyint sf_nextafter sf_remquo sf_rint sf_round sf_scalbln sf_scalbn sf_sin sf_tan \
  sf_tanh sf_trunc \
  cexp cexpf cexpl cimag cimagf cimagl creal crealf creall
NEWLIBM    ?= $(dir $(AGCC))../m68k-amigaos/lib/libm.a
LIBM_OBJS  = $(addprefix c99_,$(addsuffix .o,$(LIBM_C99))) \
             $(addprefix libm_,$(addsuffix .o,$(filter-out $(LIBM_C99),$(LIBM_FUNCS)))) math.o
NEWLIBM_OBJS = $(addprefix newlibm/lib_a-,$(addsuffix .o,$(LIBM_NEWLIB)))
libm.a: $(LIBM_OBJS) $(NEWLIBM_OBJS)
	rm -f $@
	$(XAR) rcs $@ $(LIBM_OBJS) $(NEWLIBM_OBJS)

$(NEWLIBM_OBJS): $(NEWLIBM)
	rm -rf newlibm && mkdir -p newlibm
	cd newlibm && $(XAR) x $(NEWLIBM) $(notdir $(NEWLIBM_OBJS))

libm_%.o: libm.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(LIBFLAGS) $(CPPFLAGS) -DLIBM_$* -c -o $@ libm.c

libixcompat.a: $(OBJS) $(WCS_OBJS)
	rm -f $@
	$(XAR) rcs $@ $(OBJS) $(WCS_OBJS)

$(WCS_OBJS): $(NEWLIBC)
	rm -rf newlibwcs && mkdir -p newlibwcs
	cd newlibwcs && $(XAR) x $(NEWLIBC) $(notdir $(WCS_OBJS))

%.o: %.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(LIBFLAGS) $(CPPFLAGS) -c -o $@ $<

c99_%.o: c99.c
	$(AGCC) -mcrt=ixemul -O2 -Wall $(LIBFLAGS) $(CPPFLAGS) -DC99_$* -c -o $@ c99.c

uclass.o: uclass_tab.h ixc_wide.h
utf8.o locale.o wchar.o wctype.o: ixc_wide.h
# the toolchain ships newlib's <pthread.h>, which shadows ixemul's until the SDK headers are
# reinstalled; build against the tree's own header so the struct layout matches
pthread.o: pthread.c ../include/pthread.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -c -o $@ pthread.c

# F_LOCK and friends are in the tree's <unistd.h>, not yet the installed one
lockf.o: lockf.c ../include/unistd.h
	$(AGCC) -mcrt=ixemul -O2 -Wall $(LIBFLAGS) -I../include $(CPPFLAGS) -c -o $@ lockf.c

# AF_INET6 and INET6_ADDRSTRLEN are in the tree's headers, not yet the installed ones
gai.o inet_pton.o inet_ntop.o: %.o: %.c ../include/sys/socket.h ../include/arpa/inet.h ../include/netinet/in.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -c -o $@ $<

wcwidth.o: wcwidth.c $(VTCON)/engine/vtwidth.h
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -I$(VTCON)/engine -c -o $@ wcwidth.c

tables:
	python3 gen_uclass.py $(VTCON)/engine/vtwidth.h

# host: the codec and the classes (against Python's unicodedata), wcwidth
test: test/wide_test test/wcwidth_test test/math_test test/getopt_test test/getopt_bsd_test test/nums_test test/strptime_test test/pthread_test test/getline_test test/getopt_bsd_test test/inet_test test/libc_gaps_test test/libc_gaps_link test/lockf_test test/lockf_link test/inet_link test/own_c99_link test/ixc_link test/math_header.o test/inttypes_header.o test/libm_link test/math_c99.o test/math_c99_gnu89.o selfcall
	./test/wide_test
	./test/wide_test dump | python3 test/check_uclass.py \
	  $$(sed -n 's/^#define IXC_UNICODE_VERSION "\(.*\)"/\1/p' uclass_tab.h)
	./test/wcwidth_test
	./test/math_test
	./test/getopt_test
	./test/getopt_bsd_test
	./test/nums_test
	./test/strptime_test
	./test/pthread_test
	./test/getline_test
	./test/inet_test
	./test/libc_gaps_test
	./test/lockf_test
test/wide_test: test/wide_test.c utf8.c uclass.c uclass_tab.h ixc_wide.h
	$(HOSTCC) -Wall -O2 -o $@ test/wide_test.c utf8.c uclass.c
test/math_test: test/math_test.c math.c
	$(HOSTCC) -Wall -O2 -Datan2=ixc_atan2 -c -o test/math_ixc.o math.c
	$(HOSTCC) -Wall -O2 -o $@ test/math_test.c test/math_ixc.o
test/getopt_test: test/getopt_test.c getopt_long.c ../include/getopt.h
	$(HOSTCC) -Wall -O2 -Dgetopt_long=ixc_getopt_long -Dgetopt_long_only=ixc_getopt_long_only -c -o test/getopt_ixc.o getopt_long.c
	$(HOSTCC) -Wall -O2 -o $@ test/getopt_test.c test/getopt_ixc.o
test/getopt_bsd_test: test/getopt_bsd_test.c ../static/getopt.c
	$(HOSTCC) -Wall -O2 -Dgetopt=ixc_bsd_getopt -Doptind=ixc_bsd_optind -Dopterr=ixc_bsd_opterr -Doptopt=ixc_bsd_optopt -Doptarg=ixc_bsd_optarg -c -o test/getopt_bsd_ixc.o ../static/getopt.c
	$(HOSTCC) -Wall -O2 -o $@ test/getopt_bsd_test.c test/getopt_bsd_ixc.o
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
# inet_pton/inet_ntop (AF_INET, AF_INET6): compat's code under ixc_ names against
# the vectors; test/inet_host is the same vectors against this machine's libc
test/inet_test: test/inet_test.c inet_pton.c inet_ntop.c
	$(HOSTCC) -Wall -O2 -Dinet_pton=ixc_inet_pton -Dinet_ntop=ixc_inet_ntop -c -o test/inet_pton_ixc.o inet_pton.c
	$(HOSTCC) -Wall -O2 -Dinet_pton=ixc_inet_pton -Dinet_ntop=ixc_inet_ntop -c -o test/inet_ntop_ixc.o inet_ntop.c
	$(HOSTCC) -Wall -O2 -Dinet_pton=ixc_inet_pton -Dinet_ntop=ixc_inet_ntop -o $@ test/inet_test.c test/inet_pton_ixc.o test/inet_ntop_ixc.o
# asprintf vasprintf strndup strlcpy strlcat and getaddrinfo & co: compat's code under ixc_
# names, on the host. gai.c on the Amiga also needs the tree's <netinet/in.h> (sockaddr_in6)
GAPS_REN = -D_FORTIFY_SOURCE=0 -Dasprintf=ixc_asprintf -Dvasprintf=ixc_vasprintf -Dstrndup=ixc_strndup \
  -Dstrlcpy=ixc_strlcpy -Dstrlcat=ixc_strlcat -Dgetaddrinfo=ixc_getaddrinfo \
  -Dfreeaddrinfo=ixc_freeaddrinfo -Dgetnameinfo=ixc_getnameinfo -Dgai_strerror=ixc_gai_strerror \
  -Dinet_pton=ixc_inet_pton -Dinet_ntop=ixc_inet_ntop
test/libc_gaps_test: test/libc_gaps_test.c stralloc.c strlcpy.c gai.c inet_pton.c inet_ntop.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -c -o test/stralloc_ixc.o stralloc.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -c -o test/strlcpy_ixc.o strlcpy.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -c -o test/gai_ixc.o gai.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -c -o test/inet_pton_ixc.o inet_pton.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -c -o test/inet_ntop_ixc.o inet_ntop.c
	$(HOSTCC) -Wall -O2 $(GAPS_REN) -o $@ test/libc_gaps_test.c test/stralloc_ixc.o \
	  test/strlcpy_ixc.o test/gai_ixc.o test/inet_pton_ixc.o test/inet_ntop_ixc.o

# the same test built for the Amiga against the repo's headers, no implicit declaration
# allowed, linked with libixcompat.a (link only; no rig)
test/libc_gaps_link: test/libc_gaps_test.c libixcompat.a ../include/netdb.h ../include/string.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -Werror=implicit-function-declaration -I../include $(CPPFLAGS) -o $@ test/libc_gaps_test.c libixcompat.a

# lockf on flock: compat's code under the ixc_ name against the host's flock (two
# descriptors of one file conflict on the host as on the Amiga)
test/lockf_test: test/lockf_test.c lockf.c
	$(HOSTCC) -Wall -O2 -DIXC_LOCKF=ixc_lockf -c -o test/lockf_ixc.o lockf.c
	$(HOSTCC) -Wall -O2 -o $@ test/lockf_test.c test/lockf_ixc.o

# the same test for the Amiga (-Dixc_lockf=lockf), repo headers, no implicit declaration
# allowed: fails to compile without the prototype, to link without libixcompat.a's lockf
test/lockf_link: test/lockf_test.c libixcompat.a ../include/unistd.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -Werror=implicit-function-declaration -Dixc_lockf=lockf -I../include $(CPPFLAGS) -o $@ test/lockf_test.c libixcompat.a

test/inet_host: test/inet_test.c
	$(HOSTCC) -Wall -O2 -o $@ test/inet_test.c

# the same test, built for the Amiga against the repo's headers and linked
# with libixcompat.a (link only; no rig)
test/inet_link: test/inet_test.c libixcompat.a ../include/arpa/inet.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -o $@ test/inet_test.c libixcompat.a

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
test/libm_link: test/libm_link.c test/libm_names.txt libm.a
	$(AGCC) -mcrt=ixemul -O2 -Wall -fno-builtin $(CPPFLAGS) -L. -o $@ test/libm_link.c -lm
	@if $(dir $(AGCC))m68k-amigaos-nm $@ | grep -q impure_ptr; then \
	  echo "FAIL: test/libm_link references _impure_ptr"; rm -f $@; exit 1; fi
	@$(dir $(AGCC))m68k-amigaos-nm $@ | awk '$$2 ~ /[TtWw]/ {print $$3}' | sort -u > $@.syms
	@missing=$$(while read n; do grep -qx "_$$n" $@.syms || echo $$n; done < test/libm_names.txt); \
	  if [ -n "$$missing" ]; then echo "FAIL: not linked: $$missing"; rm -f $@; exit 1; fi; \
	  echo "libm_link: $$(wc -l < test/libm_names.txt) C99 math symbols present, no _impure_ptr"

# no libm.a function branches back to its own entry (LIBFLAGS). Not run on
# libixcompat.a: newlib's tdelete recurses on purpose (_trecurse) and
# uclass.c's casemap loops back to its entry (a loop, not a call)
selfcall: libm.a
	OBJDUMP=$(dir $(AGCC))m68k-amigaos-objdump python3 -I ../tools/selfcall_check.py libm.a

# values of every function at the edges (0, -0, inf, nan, negative); built for
# the Amiga, run under vamos or on the machine (not part of "make test").
# Linked against this directory's libm.a by path: with a libm.a installed in
# ixemul/lib, -L. -lm picks the installed one and tests the old build
test/softcmp_test: test/softcmp_test.c
	$(AGCC) -mcrt=ixemul -m68000 -msoft-float -O2 -Wall -fno-builtin $(CPPFLAGS) -o $@ test/softcmp_test.c

test/libm_num: test/libm_num.c libm.a
	$(AGCC) -mcrt=ixemul -O2 -Wall -fno-builtin $(CPPFLAGS) -o $@ test/libm_num.c libm.a

# <math.h>'s C99 constants (compile only, against the repo's headers)
test/math_header.o: test/math_header.c ../include/math.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -c -o $@ test/math_header.c

# <math.h> declares every name in test/libm_names.txt. -I../include comes
# first, ahead of the installed header; MATH_INC overrides the dir to run the
# test against another header (the pre-fix one, to see it fail).
MATH_INC ?= ../include
test/math_c99.o: test/math_c99.c test/libm_names.txt $(MATH_INC)/math.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -fno-builtin -std=c99 -Werror=implicit-function-declaration -I$(MATH_INC) $(CPPFLAGS) -c -o $@ test/math_c99.c
test/math_c99_gnu89.o: test/math_c99.c test/libm_names.txt $(MATH_INC)/math.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -fno-builtin -std=gnu89 -Werror=implicit-function-declaration -I$(MATH_INC) $(CPPFLAGS) -c -o $@ test/math_c99.c

# getopt_long and pselect resolve from libixcompat.a (link only)
test/ixc_link: test/ixc_link.c libixcompat.a ../include/getopt.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -I../include $(CPPFLAGS) -o $@ test/ixc_link.c libixcompat.a

test/inttypes_header.o: test/inttypes_header.c ../include/inttypes.h
	$(AGCC) -mcrt=ixemul -O2 -Wall -Werror=format -I../include $(CPPFLAGS) -c -o $@ test/inttypes_header.c

spawnprobe: test/spawnprobe test/envprobe
# libixcompat.a by its path: the driver searches the SDK's lib (an older
# copy) before any -L given here
test/spawnprobe: test/spawnprobe.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/spawnprobe.c libixcompat.a

test/envprobe: test/envprobe.c libixcompat.a
	$(AGCC) -mcrt=ixemul -O2 -Wall $(CPPFLAGS) -o $@ test/envprobe.c libixcompat.a

install: libixcompat.a libm.a
	cp libixcompat.a $(SDK)/lib/libixcompat.a
	cp libm.a $(SDK)/lib/libm.a

clean:
	rm -rf $(OBJS) $(LIBM_OBJS) newlibwcs libixcompat.a libm.a newlibm test/libm_link test/softcmp_test test/wide_test test/wcwidth_test test/nums_test test/strptime_test test/pthread_test test/getline_test test/inet_test test/inet_host test/inet_link test/libc_gaps_test test/libc_gaps_link test/lockf_test test/lockf_link test/*_ixc.o test/spawnprobe test/envprobe test/own_c99_link test/math_header.o test/math_c99.o test/math_c99_gnu89.o test/ixc_link test/inttypes_header.o
# clean must leave no object behind: a stale libm_sqrtf.o once survived it
# and kept a self-calling sqrtf in every rebuilt libm.a.
cleancheck: clean
	@left=$$(ls *.o test/*.o 2>/dev/null); if [ -n "$$left" ]; then echo "[ERROR] clean left: $$left"; exit 1; fi; echo "[OK] clean removes every object"

.PHONY: tables test spawnprobe install clean selfcall cleancheck
