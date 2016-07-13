# chipKIT PIC32 compiler from UECIDE
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Use UECIDE package from http://uecide.org/download
ifndef MIPS_GCC_PREFIX
    ifdef UECIDE
		ifeq ($(UECIDE)/compilers/pic32-tools-452/bin/pic32-gcc,$(wildcard $(UECIDE)/compilers/pic32-tools-452/bin/pic32-gcc))
			MIPS_GCC_PREFIX = $(UECIDE)/compilers/pic32-tools-452/bin/pic32-
			MIPS_GCC_FORMAT = elf32-tradlittlemips
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
ifndef MIPS_GCC_PREFIX
    ifeq (/usr/local/pic32-tools/bin/pic32-gcc,$(wildcard /usr/local/pic32-tools/bin/pic32-gcc))
        MIPS_GCC_PREFIX = /usr/local/pic32-tools/bin/pic32-
        MIPS_GCC_FORMAT = elf32-tradlittlemips
    endif
endif

# Generic MIPS toolchain
# ~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
ifndef MIPS_GCC_PREFIX
    ifeq (/usr/local/mips-gcc-4.8.1/bin/mips-elf-gcc,$(wildcard /usr/local/mips-gcc-4.8.1/bin/mips-elf-gcc))
        MIPS_GCC_PREFIX = /usr/local/mips-gcc-4.8.1/bin/mips-elf-
    endif
endif

# Generic MIPS toolchain on *BSD
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
# Maybe you can install it from packages one day too.
ifndef MIPS_GCC_PREFIX
    ifeq (/usr/local/mips-elf/bin/mips-elf-gcc,$(wildcard /usr/local/mips-elf/bin/mips-elf-gcc))
        MIPS_GCC_PREFIX = /usr/local/mips-elf/bin/mips-elf-
    endif
endif

# Mentor Sourcery CodeBench Lite toolchain
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ifndef MIPS_GCC_PREFIX
    # Download a Linux binary package from
    # https://sourcery.mentor.com/GNUToolchain/release2641
    ifeq (/usr/local/mips-2013.11/bin/mips-sde-elf-gcc,$(wildcard /usr/local/mips-2013.11/bin/mips-sde-elf-gcc))
        MIPS_GCC_PREFIX = /usr/local/mips-2013.11/bin/mips-sde-elf-
        MIPS_GCC_FORMAT = elf32-tradlittlemips
    endif
endif
ifndef MIPS_GCC_PREFIX
    # Download a Linux binary package from
    # https://sourcery.mentor.com/GNUToolchain/release2774
    ifeq (/usr/local/mips-2014.05/bin/mips-sde-elf-gcc,$(wildcard /usr/local/mips-2014.05/bin/mips-sde-elf-gcc))
        MIPS_GCC_PREFIX = /usr/local/mips-2014.05/bin/mips-sde-elf-
        MIPS_GCC_FORMAT = elf32-tradlittlemips
    endif
endif

# Imagination Codescape MIPS SDK Essentials
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Download a Linux binary package from:
# http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/download-codescape-mips-sdk-essentials/
ifndef MIPS_GCC_ROOT
    ifeq (/opt/imgtec/Toolchains/mips-mti-elf/2015.01-7,$(wildcard /opt/imgtec/Toolchains/mips-mti-elf/2015.01-7))
        MIPS_GCC_ROOT   = /opt/imgtec/Toolchains/mips-mti-elf/2015.01-7
        MIPS_GCC_FORMAT = elf32-tradlittlemips
    endif
endif

ifndef MIPS_GCC_PREFIX
    $(error Unable to locate any GCC MIPS toolchain!)
endif
