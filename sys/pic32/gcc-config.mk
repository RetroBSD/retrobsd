# Clang compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~
# Install by:
#   sudo apt install clang-12
# Newer versions are also OK.
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-12/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-13/bin/clang))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/lib/llvm-14/bin/clang))
endif

# Clang compiler on MacOS
# ~~~~~~~~~~~~~~~~~~~~~~~
# Install by:
#   brew install clang@12
# Newer versions are also OK.
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@12/12.*/bin/clang-12))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@13/13.*/bin/clang-13))
endif
ifeq ($(LLVMBIN),)
    LLVMBIN = $(dir $(wildcard /usr/local/Cellar/llvm@14/14.*/bin/clang-14))
endif

ifeq ($(LLVMBIN),)
    $(error Unable to find any CLANG toolchain!)
endif
