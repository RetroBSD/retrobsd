MACHINE		= mips
DESTDIR		?= $(TOPSRC)

# chipKIT PIC32 compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Download from https://github.com/jasonkajita/chipKIT-cxx/downloads
# and unzip to /usr/local.
# Need to copy pic32-tools/pic32mx/include/stdarg.h
# to pic32-tools/lib/gcc/pic32mx/4.5.1/include.
# MPLABX C32 compiler doesn't support some functionality
# we need, so use chipKIT compiler by default.
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/pic32-tools/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    INCLUDES    = -I/usr/local/pic32-tools/lib/gcc/pic32mx/4.5.1/include
endif

# Generic MIPS toolchain
# ~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/mips-gcc-4.7.2/bin/mips-elf-
    LDFLAGS     =
    INCLUDES    =
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

CFLAGS		= -O

LDFLAGS		+= -N -nostartfiles -fno-dwarf2-cfi-asm -T$(TOPSRC)/src/elf32-mips.ld \
		   $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src
LIBS		= -lc
