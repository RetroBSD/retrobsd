# PIC32 compiler from UECIDE
# Use UECIDE package from http://uecide.org/download
ifndef MIPS_GCC_PREFIX
    ifeq ($(HOME)/.uecide/compilers/pic32-tools/bin/pic32-gcc,$(wildcard $(HOME)/.uecide/compilers/pic32-tools/bin/pic32-gcc))
        MIPS_GCC_PREFIX = $(HOME)/.uecide/compilers/pic32-tools/bin/pic32-
        MIPS_GCC_FORMAT = elf32-tradlittlemips
    endif
endif

ifndef MIPS_GCC_PREFIX
    $(error Unable to locate any GCC MIPS toolchain!)
endif
