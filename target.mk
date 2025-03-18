MACHINE     = mips
DESTDIR     ?= $(TOPSRC)
RELEASE     = 0.0
BUILD       = $(shell git rev-list HEAD --count)
VERSION     = $(RELEASE)-$(BUILD)

include $(TOPSRC)/clang-config.mk

CC		= $(LLVMBIN)clang -target mipsel -mcpu=mips32r2 -mabi=o32 -msoft-float \
                  -fomit-frame-pointer -finline-hint-functions -I$(TOPSRC)/include \
                  -Wno-builtin-requires-header -Werror
CXX             = $(LLVMBIN)clang++ -target mipsel -mcpu=mips32r2 -mabi=o32 -msoft-float \
                  -fomit-frame-pointer -finline-hint-functions -I$(TOPSRC)/include
LD		= $(LLVMBIN)ld.lld -m elf32ltsmip
AR		= $(LLVMBIN)llvm-ar
RANLIB          = $(LLVMBIN)llvm-ranlib
SIZE            = $(LLVMBIN)llvm-size
NM              = $(LLVMBIN)llvm-nm
OBJDUMP         = $(LLVMBIN)llvm-objdump --mcpu=mips32r2
AS		= $(CC) -x assembler-with-cpp -c
YACC            = byacc
LEX             = flex
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
DOCROFF		= nroff -mdoc -h
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -Os -nostdinc

LDFLAGS		= --nmagic -T$(TOPSRC)/src/elf32-mips.ld $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src
LIBS		= -lc

# Enable mips16e instruction set by default
#CFLAGS		+= -mips16

# Catch all warnings
CC              += -Werror
