# Find a Clang/LLVM toolchain able to generate MIPS code.
#
# Install by:
#   sudo apt install clang lld              -- on Linux
#   brew install llvm lld                   -- on MacOS
#
# Any LLVM version from 14 up is fine.  When several are installed, the
# newest one is used.  To select a particular toolchain, set LLVMBIN
# explicitly, for example:
#   make LLVMBIN=/opt/homebrew/opt/llvm@21/bin/
#
LLVM_VERSIONS = 22 21 20 19 18 17 16 15 14

ifeq ($(LLVMBIN),)
    # Homebrew on MacOS installs into /opt/homebrew (arm64) or /usr/local (x86).
    # The unversioned formula is the newest one.  Probe the opt/ prefix, which
    # Homebrew always creates, rather than versioned Cellar directories.
    # On Linux, apt installs into /usr/lib/llvm-NN.
    LLVM_SEARCH := /opt/homebrew/opt/llvm /usr/local/opt/llvm \
        $(foreach v,$(LLVM_VERSIONS),/opt/homebrew/opt/llvm@$(v) /usr/local/opt/llvm@$(v)) \
        $(foreach v,$(LLVM_VERSIONS),/usr/lib/llvm-$(v))

    LLVMBIN := $(dir $(firstword $(wildcard $(addsuffix /bin/clang,$(LLVM_SEARCH)))))
endif

# Default Clang compiler.
# Note: native clang on MacOS has no support for mips32.
ifeq ($(LLVMBIN),)
    ifneq ($(shell uname -s),Darwin)
        LLVMBIN = $(dir $(wildcard /usr/bin/clang))
    endif
endif

ifeq ($(LLVMBIN),)
    $(error Unable to find any CLANG toolchain! Install it by "brew install llvm lld" on MacOS \
or "sudo apt install clang lld" on Linux, or set LLVMBIN=/path/to/llvm/bin/)
endif

# The LLVM linker.  Homebrew ships lld as a separate formula, so it is usually
# absent from the LLVM directory: fall back to whatever ld.lld is on the PATH.
LLD = $(if $(wildcard $(LLVMBIN)ld.lld),$(LLVMBIN))ld.lld
