MACHINE     = mips
DESTDIR     ?= $(TOPSRC)
RELEASE     = 0.0
BUILD       = $(shell git rev-list HEAD --count)
VERSION     = $(RELEASE)-$(BUILD)

# chipKIT PIC32 compiler from UECIDE
ifdef UECIDE
ifndef GCCPREFIX
	ifeq ($(UECIDE)/compilers/pic32-tools-452/bin/pic32-gcc,$(wildcard $(UECIDE)/compilers/pic32-tools-452/bin/pic32-gcc))
		GCCPREFIX   = ${UECIDE}/compilers/pic32-tools-452/bin/pic32-
		LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
		INCLUDES    = -I${UECIDE}/compilers/pic32-tools-452/lib/gcc/pic32mx/4.5.2/include
	endif
endif
endif

# chipKIT PIC32 compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Download from https://github.com/jasonkajita/chipKIT-cxx/downloads
# and unzip to /usr/local.
# Need to copy pic32-tools/pic32mx/include/stdarg.h
# to pic32-tools/lib/gcc/pic32mx/4.5.1/include.
# MPLABX C32 compiler doesn't support some functionality
# we need, so use chipKIT compiler by default.
ifndef GCCPREFIX
ifeq (/usr/local/pic32-tools/bin/pic32-gcc,$(wildcard /usr/local/pic32-tools/bin/pic32-gcc))
    GCCPREFIX   = /usr/local/pic32-tools/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    INCLUDES    = -I/usr/local/pic32-tools/lib/gcc/pic32mx/4.5.1/include
endif
endif

# Generic MIPS toolchain
# ~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
ifndef GCCPREFIX
ifeq (/usr/local/mips-gcc-4.8.1/bin/mips-elf-gcc,$(wildcard /usr/local/mips-gcc-4.8.1/bin/mips-elf-gcc))
    GCCPREFIX   = /usr/local/mips-gcc-4.8.1/bin/mips-elf-
    LDFLAGS     =
    INCLUDES    =
endif
endif

# Generic MIPS toolchain on *BSD
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
# Maybe you can install it from packages one day too.
ifndef GCCPREFIX
ifeq (/usr/local/mips-elf/bin/mips-elf-gcc,$(wildcard /usr/local/mips-elf/bin/mips-elf-gcc))
    GCCPREFIX   = /usr/local/mips-elf/bin/mips-elf-
    LDFLAGS     =
    INCLUDES    =
endif
endif

# Mentor Sourcery CodeBench Lite toolchain -- DG Downloaded this compiler suite from given link
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# You can download a Linux or Windows binary package from
# https://sourcery.mentor.com/GNUToolchain/release2641
ifndef GCCPREFIX
ifeq (/usr/local/mips-2013.11/bin/mips-sde-elf-gcc,$(wildcard /usr/local/mips-2013.11/bin/mips-sde-elf-gcc))
    GCCPREFIX   = /usr/local/mips-2013.11/bin/mips-sde-elf-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    INCLUDES    =
endif
endif
ifndef GCCPREFIX
ifeq (/usr/local/mips-2014.05/bin/mips-sde-elf-gcc,$(wildcard /usr/local/mips-2014.05/bin/mips-sde-elf-gcc))
    GCCPREFIX   = /usr/local/mips-2014.05/bin/mips-sde-elf-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    INCLUDES    =
endif
endif

ifndef GCCPREFIX
    $(error Unable to locate any GCC MIPS toolchain!)
endif

CC		= $(GCCPREFIX)gcc -mips32r2 -EL -msoft-float -nostdinc -fshort-double -I$(TOPSRC)/include $(INCLUDES)
CXX             = $(GCCPREFIX)g++ -mips32r2 -EL -msoft-float -nostdinc -fshort-double -I$(TOPSRC)/include $(INCLUDES)
LD		= $(GCCPREFIX)ld
AR		= $(GCCPREFIX)ar
RANLIB          = $(GCCPREFIX)ranlib
SIZE            = $(GCCPREFIX)size
OBJDUMP         = $(GCCPREFIX)objdump -mmips:isa32r2
AS		= $(CC) -x assembler-with-cpp -c
YACC            = byacc
LEX             = flex
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h -Tascii
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -Os

LDFLAGS		+= -N -nostartfiles -fno-dwarf2-cfi-asm -T$(TOPSRC)/src/elf32-mips.ld \
		   $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src
LIBS		= -lc

# Enable mips16e instruction set by default
CFLAGS		+= -mips16
