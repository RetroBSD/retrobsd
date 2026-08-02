# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

RetroBSD is 2.11BSD ported to PIC32 (MIPS32r2) microcontrollers — a full Unix (kernel + userland +
libc + man pages) running in 128 KB of on-chip RAM with no MMU. Consequences that shape the whole
tree: user programs are BSD **a.out**, not ELF; processes are swapped whole (no demand paging);
`-Os`/`-O` everywhere; and almost every source file is historic BSD C that has been kept working
rather than modernized.

A build produces two artifacts: `sdcard.img` (root filesystem) at the top level, and `unix.hex`
(kernel for flash) in each board subdirectory of `sys/pic32/`.

## Build

```shell
make              # everything: tools, kernel, userland, sdcard.img — `-j` is safe
make clean        # object files in tools, lib, src, sys/pic32
make cleanall     # also removes installed binaries in bin/ sbin/ libexec/ games/ and sdcard.img
```

Toolchain: Clang/LLVM + `ld.lld` (14 through 22), `byacc`, `bison`, `flex`, `groff`, `libelf`,
plus a host `gcc -m32` for the cross tools. [clang-config.mk](clang-config.mk) searches
`/opt/homebrew/opt/llvm*`, `/usr/local/opt/llvm*` and `/usr/lib/llvm-*` newest-first and sets
`LLVMBIN` to the first hit; if it picks the wrong one or finds none, override it:
`make LLVMBIN=/opt/homebrew/opt/llvm@21/bin/`. It also exports `LLD`, used for `LD` by both
[target.mk](target.mk) and [sys/pic32/Makefile.kconf](sys/pic32/Makefile.kconf) — Homebrew keeps
`lld` in a separate formula, so `LLD` falls back to whatever `ld.lld` is on `PATH`. Note native
Apple clang cannot target mips32, so on macOS a Homebrew LLVM is mandatory.

Local overrides go in `Makefile.user` (gitignored, `-include`d by the top Makefile) — e.g. `SDCARD`
for `make installfs`.

There is no test suite. Verification is running the kernel under
[qemu-pic32](https://github.com/sergev/qemu) with `sdcard.img` attached; see [README.md](README.md).

### Build order (top-level Makefile)

`tools` → `kernel` (`sys/pic32`) → `src` (libs then commands) → `make -C src install` → `fs`.

The install step is unusual: `DESTDIR` defaults to `TOPSRC`, so cross-compiled target binaries are
installed **back into the working tree** (`bin/`, `sbin/`, `libexec/`, `games/`, `share/`). `fsutil`
then packs those paths into `sdcard.img` according to [rootfs.manifest](rootfs.manifest). A new
program is not on the image until it is both installed and listed in the manifest.

### Three separate build configurations

| File | Used by | Compiler |
|---|---|---|
| [target.mk](target.mk) | `src/` — userland libs and commands | `clang -target mipsel -mcpu=mips32r2 -mabi=o32 -msoft-float -nostdinc -Werror` |
| [sys/pic32/Makefile.kconf](sys/pic32/Makefile.kconf) | kernel | same triple, plus `-DKERNEL -fno-builtin -Wall -Werror`, separate `CFLAGS` |
| [cross.mk](cross.mk) | `tools/`, `lib/` | host `gcc -m32 -DCROSS` |

`-Werror` is on for both target and kernel builds, so any new warning breaks the build.

Target link recipe (repeated in nearly every `src/cmd/*/Makefile`): link with `ld.lld` against
[src/elf32-mips.ld](src/elf32-mips.ld) + `crt0.o` to `prog.elf`, dump disassembly to `prog.dis`,
then `tools/elf2aout` converts the ELF to an a.out named `prog` and the `.elf` is deleted.

## Kernel configuration (kconfig)

Boards live in `sys/pic32/<board>/` (currently `explorer16`, `olimex`). Each has a `Config` file
declaring CPU, board, linker script, options, and devices with their pins.
[tools/kconfig](tools/kconfig) reads `Config` together with [sys/pic32/files.kconf](sys/pic32/files.kconf),
[sys/pic32/devices.kconf](sys/pic32/devices.kconf), and the `Makefile.kconf` template, and generates
the board's `Makefile`, `ioconf.c`, and `swapunix.c`.

**The generated board `Makefile` is committed to git.** After editing a `Config`, `files.kconf`, or
`devices.kconf`, regenerate it:

```shell
make -C sys/pic32/explorer16 reconfig    # or: cd sys/pic32/explorer16 && ../../../tools/kconfig/kconfig Config
```

Adding a kernel source file means adding it to `files.kconf` and regenerating — the object lists in
board Makefiles are generated output, not hand-edited.

Kernel build products in the board directory: `unix.hex` (flash image), `unix.bin`, `unix.dis`
(annotated disassembly), `unix.nm` (sorted symbols), `boot.bin`.

## Source layout

- `sys/kernel/` — machine-independent kernel: 2.11BSD-style `kern_*`, `ufs_*`, `vm_*`, `sys_*`,
  `tty*`, `exec_*`. `sys/include/` are the kernel headers.
- `sys/pic32/` — machine-dependent layer: `machdep.c`, `startup.S`, `exception.c`, and the device
  drivers (`uart.c`, `sd.c`, `spi*.c`, `gpio.c`, `adc.c`, `pwm.c`, `gpanel*.c`, `usb_*.c`).
- `src/` — everything that runs on the target: `libc/`, `libm/`, `libcurses/`, `libtermlib/`,
  `libutil/`, `libvmf/`, `libreadline/`, `libgpanel/`, `libwiznet/`, plus `cmd/`, `games/`, `man/`.
- `include/` — target userland headers (`machine` and `pic32` are symlinks). Distinct from
  `sys/include/`.
- `tools/` — host programs: `kconfig`, `fsutil` (filesystem image builder/checker), `elf2aout`.
- `lib/` — host builds of the binutils (`ar as ld nm ranlib size strip`) from the same sources as
  `src/cmd/*`, via `vpath`. **Currently disabled** — the `$(MAKE) -C lib` line in the top Makefile
  is commented out.
- `bin/ sbin/ libexec/ games/ etc/ share/ var/` — install destinations, largely build output.

## Adding a userland program

1. Either a single `.c` in `src/cmd/` added to the `STD` list in [src/cmd/Makefile](src/cmd/Makefile),
   or a subdirectory with its own Makefile added to `SUBDIR` there. Subdirectory Makefiles compute
   `TOPSRC = $(shell cd ../../..; pwd)` and `include $(TOPSRC)/target.mk`. Copy an existing one
   ([src/cmd/ls/Makefile](src/cmd/ls/Makefile) is a minimal example) — the elf → dis → a.out recipe
   must be repeated.
2. Add the installed path to [rootfs.manifest](rootfs.manifest), which also encodes owner, group,
   mode, and setuid bits (`fsutil --scan dir` regenerates a manifest from a directory).

Rebuild one utility without a full build: `make -C src/cmd/ls` (target libraries under `src/` must
already be built).

## Filesystem images

`tools/fsutil/fsutil` manipulates the RetroBSD (2.11BSD-style) filesystem from the host:
`--check [--fix]`, `--new --manifest=...`, `--add`, `--extract`, `--repartition=...`, `--scan`,
all with `--partition=N`. Layout in the top Makefile: partition 1 = root (`FS_MBYTES`),
2 = swap (`SWAP_MBYTES`), 3 = optional `/u` (`U_MBYTES`, commented out by default).

## Style

[.clang-format](.clang-format) — Google base, 4-space indent, 100 columns, WebKit brace style,
pointers bound right (`int *a`). Most of the tree predates it and is K&R-era BSD code; match the
surrounding file rather than reformatting it, and keep diffs to the lines being changed.
