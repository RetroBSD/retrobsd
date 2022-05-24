MACHINE     = mips
DESTDIR     ?= $(TOPSRC)
RELEASE     = 0.0
BUILD       = $(shell git rev-list HEAD --count)
VERSION     = $(RELEASE)-$(BUILD)

# PIC32 compiler from UECIDE
# Use UECIDE package from http://uecide.org/download
ifndef GCCPREFIX
    ifeq ($(HOME)/.uecide/compilers/pic32-tools/bin/pic32-gcc,$(wildcard $(HOME)/.uecide/compilers/pic32-tools/bin/pic32-gcc))
        GCCPREFIX   = $(HOME)/.uecide/compilers/pic32-tools/bin/pic32-
        LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
        INCLUDES    = -I$(HOME)/.uecide/compilers/pic32-tools/lib/gcc/pic32mx/4.5.2/include
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
