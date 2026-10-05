# intops/Makefile
#
# CPU/FPU-specific integer helper routines for ixemul.library.
# This Makefile is normally invoked from a generated build directory:
#
#   intops/<cpu>/<fpu>/<os>/
#
# In that case the assembler sources are three levels above the build
# directory.  The source-directory test also permits a direct build from
# the intops source directory.

CC      ?= gcc
AR      ?= ar
RANLIB  ?= ranlib
RM      ?= rm -f

ifneq ($(wildcard bswap16.s),)
SRCDIR = .
else
SRCDIR = ../../..
endif

VPATH = $(SRCDIR)

OBJS = \
	bswap16.o \
	bswap32.o \
	bswap64.o

LIB = libintops.a

# Use paths relative to the source directory, not to the generated
# CPU/FPU/OS build directory.
INTOPS_CPPFLAGS = -I$(SRCDIR)/.. -I$(SRCDIR)/../include

.PHONY: all clean clobber

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

%.o: $(SRCDIR)/%.s
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INTOPS_CPPFLAGS) -c $< -o $@

# When called in a generated build directory, this removes the local
# objects/archive.  When called in intops/, it additionally removes every
# CPU/FPU/OS build directory created by the top-level build.
clean:
	$(RM) $(OBJS) $(LIB) *.o *.a
	rm -rf 68000 68020 68040 68060 notrap

clobber: clean
