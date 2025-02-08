MACHINE     = mips
DESTDIR     ?= $(TOPSRC)
RELEASE     = 0.0
BUILD       = $(shell git rev-list HEAD --count)
VERSION     = $(RELEASE)-$(BUILD)

# Clang compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~
# Install by:
#   sudo apt install clang-18
# Newer versions are also OK.
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-18/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-16/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-15/bin/clang))
endif

# Clang compiler on MacOS
# ~~~~~~~~~~~~~~~~~~~~~~~
# Install by:
#   brew install clang@18
# Newer versions are also OK.
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@18/18.*/bin/clang-18))
endif

ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/bin/clang))
endif
ifeq ($(LLVMBIN),)
    $(error Unable to find any CLANG toolchain!)
endif

# TODO: remove -Wno-deprecated-non-prototype -Wno-implicit-int
CC		= $(LLVMBIN)clang -target mipsel -mcpu=mips32r2 -mabi=o32 -msoft-float \
                  -fomit-frame-pointer -finline-hint-functions -I$(TOPSRC)/include \
                  -Wno-builtin-requires-header -Wno-deprecated-non-prototype -Wno-implicit-int
CXX             = $(LLVMBIN)clang++ -target mipsel -mcpu=mips32r2 -mabi=o32 -msoft-float \
                  -fomit-frame-pointer -finline-hint-functions -I$(TOPSRC)/include -Wno-deprecated-non-prototype -Wno-implicit-int
LD		= $(LLVMBIN)ld.lld -m elf32ltsmip
AR		= $(LLVMBIN)llvm-ar
RANLIB          = $(LLVMBIN)llvm-ranlib
SIZE            = $(LLVMBIN)llvm-size
OBJDUMP         = $(LLVMBIN)llvm-objdump --mcpu=mips32r2
AS		= $(CC) -x assembler-with-cpp -c
YACC            = byacc
LEX             = flex
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h -Tascii
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -Os -nostdinc

LDFLAGS		= --nmagic -T$(TOPSRC)/src/elf32-mips.ld $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src
LIBS		= -lc

# Enable mips16e instruction set by default
#CFLAGS		+= -mips16

# Catch all warnings
CC              += -Werror
