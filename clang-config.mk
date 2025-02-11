# Clang compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~
# Install by:
#   sudo apt install clang-18
# Newer versions are also OK.
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-18/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-17/bin/clang))
endif
ifeq ($(LLVMBIN),)
    # Default on Ubuntu 24.04
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-16/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-15/bin/clang))
endif
ifeq ($(LLVMBIN),)
    # Default on Debian 12 amd Ubuntu 22.04
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-14/bin/clang))
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
    LLVMBIN = $(dir $(wildcard /opt/homebrew/Cellar/llvm@18/18.*/bin/clang-18))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@17/17.*/bin/clang-17))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /opt/homebrew/Cellar/llvm@17/17.*/bin/clang-17))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@16/16.*/bin/clang-16))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /opt/homebrew/Cellar/llvm@16/16.*/bin/clang-16))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@15/15.*/bin/clang-15))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /opt/homebrew/Cellar/llvm@15/15.*/bin/clang-15))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@14/14.*/bin/clang-14))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /opt/homebrew/Cellar/llvm@14/14.*/bin/clang-14))
endif

# Default Clang compiler
# ~~~~~~~~~~~~~~~~~~~~~~
ifeq ($(LLVMBIN),)
    # Note: native clang on MacOS has no support for mips32.
    ifneq ($(shell uname -s),Darwin)
        LLVMBIN = $(dir $(wildcard /usr/bin/clang))
    endif
endif
ifeq ($(LLVMBIN),)
    $(error Unable to find any CLANG toolchain!)
endif
