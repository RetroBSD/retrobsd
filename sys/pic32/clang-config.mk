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
