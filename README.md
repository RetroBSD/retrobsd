# RetroBSD

**A real Unix that fits in 128 kbytes of RAM.**

RetroBSD is [2.11BSD](https://en.wikipedia.org/wiki/History_of_the_Berkeley_Software_Distribution)
ported to PIC32 microcontrollers — a multi-user, multi-tasking operating system with a kernel,
a C compiler, shells, `vi`, man pages and a pile of BSD games, all running on a chip with no MMU
and no external memory.

* **Kernel** — under 128 kbytes of flash, fits in on-chip RAM.
* **Userland** — up to 96 kbytes per process, swapped whole to the SD card.
* **Storage** — a normal BSD filesystem on an SD card.
* **Binaries** — classic BSD `a.out`, built by a Clang/LLVM cross toolchain on your host.

Documentation lives in the [project wiki](https://github.com/RetroBSD/retrobsd/wiki).

## Quick start

No hardware needed — you can build the system and boot it in a simulator in about five minutes.

### 1. Install the toolchain

Clang, the LLVM linker, two YACCs, flex, groff and the ELF library.

Ubuntu / Debian:

```shell
sudo apt-get install clang lld byacc bison flex groff-base libelf-dev
```

macOS:

```shell
brew install llvm lld byacc bison flex groff libelf
```

> Apple's built-in Clang cannot generate MIPS code, so a Homebrew LLVM is required on macOS.
> LLVM 14 through 22 all work. The build finds the newest installed compiler automatically; if it
> picks the wrong one, point it at the right directory yourself:
> `make LLVMBIN=/opt/homebrew/opt/llvm@21/bin/`

### 2. Build everything

```shell
make -j1
```

Use `-j1` — the build is not parallel-safe. It produces:

| File | What it is |
| --- | --- |
| `sdcard.img` | Root filesystem image (100 Mbyte, with swap partition) |
| `sys/pic32/explorer16/unix.hex` | Kernel for the Microchip Explorer 16 board |
| `sys/pic32/olimex/unix.hex` | Kernel for the Olimex PIC32-RetroBSD board |

### 3. Boot it

Install [QEMU for PIC32](https://github.com/sergev/qemu), then run the Explorer 16 kernel against
the disk image:

```shell
qemu-pic32 -machine pic32mx7-explorer16 -nographic -monitor none \
    -serial vc -serial stdio \
    -kernel sys/pic32/explorer16/unix.hex -sd sdcard.img
```

You should get a `login:` prompt within a couple of seconds. Log in as `root` — the password is
blank, so just press Enter.

<details>
<summary>Example boot log</summary>

```
Board: Microchip Explorer16
Processor: M4K
RAM size: 128 kbytes
Load file: 'unix.hex', 166284 bytes
Card0 image 'sdcard.img', 102758 kbytes

2.11 BSD Unix for PIC32, revision G580 build 9:
     Compiled 2025-02-09 by vak@bumba:
     /home/vak/Project/BSD/retrobsd-clang/sys/pic32/explorer16
cpu: 795F512L 80 MHz, bus 80 MHz
oscillator: HS crystal, PLL div 1:2 mult x20
spi1: pins sdi=RC4/sdo=RD0/sck=RD10
uart2: pins rx=RF4/tx=RF5, interrupts 40/41/42, console
sd0: port SPI1, pin cs=RB1
gpio0: portA, pins ii---ii-iiiioooo
gpio1: portB, pins iiiiiiiiiiiiii-i
gpio2: portC, pins iiii--------iii-
gpio3: portD, pins iiiii-iiiiiiiii-
gpio4: portE, pins ------iiiiiiiiii
gpio5: portF, pins --ii---i----iiii
gpio6: portG, pins iiii--iiii--iiii
adc: 15 channels
pwm: 5 channels
sd0: type I, size 102400 kbytes, speed 10 Mbit/sec
sd0a: partition type b7, sector 2, size 102400 kbytes
sd0b: partition type b8, sector 204802, size 2048 kbytes
sd0c: partition type b7, sector 208898, size 102400 kbytes
phys mem  = 128 kbytes
user mem  = 96 kbytes
root dev  = (0,1)
swap dev  = (0,2)
root size = 102400 kbytes
swap size = 2048 kbytes
/dev/sd0a: 1438 files, 15410 used, 86589 free
Starting daemons: update cron


RetroBSD (pic32) (console)

login: _
```
</details>

## Running on real hardware

Two boards are supported:

| Board | Kernel | Console |
| --- | --- | --- |
| [Olimex PIC32-RetroBSD](https://github.com/RetroBSD/retrobsd/wiki/Board-Olimex-PIC32-RetroBSD) | `sys/pic32/olimex/unix.hex` | USB |
| Microchip Explorer 16 — with PIC32 CAN-USB plug-in module and SD & MMC pictail | `sys/pic32/explorer16/unix.hex` | UART2 |

### Write the filesystem to an SD card

Use [balenaEtcher](https://etcher.balena.io) to flash `sdcard.img` to the card.

Alternatively, set `SDCARD` in a `Makefile.user` file (for example `SDCARD = /dev/sdb`) and run:

```shell
make installfs
```

### Write the kernel to flash

Both boards are programmed with the [pic32prog](https://github.com/sergev/pic32prog) utility.

Olimex PIC32-RetroBSD, over a USB cable:

```shell
pic32prog -d /dev/ttyUSB0 sys/pic32/olimex/unix.hex
```

Explorer 16, using a PICkit 2 clone:

```shell
pic32prog sys/pic32/explorer16/unix.hex
```

## Source roadmap

| Directory | Contents |
| --- | --- |
| `sys/kernel` | Machine-independent kernel: processes, signals, filesystem, tty |
| `sys/pic32` | PIC32 support and drivers; one subdirectory per board |
| `src/cmd` | User commands — shell, editors, compilers, `vi`, `awk`, and the rest |
| `src/games` | BSD games, from `adventure` to `rogue` and `trek` |
| `src/lib*` | Target libraries: libc, libm, curses, readline, termlib and friends |
| `src/man` | Manual pages |
| `include` | System include files for user programs |
| `etc` | Template files for `/etc` |
| `share` | Shared resources: dictionary, examples, man pages |
| `tools` | Host-side build tools: `kconfig`, `fsutil`, `elf2aout` |

Compiled programs are installed into `bin`, `sbin`, `libexec` and `games` in this tree, and are
then packed into the disk image according to [rootfs.manifest](rootfs.manifest).

## Rebuilding parts

```shell
make -C sys/pic32/explorer16          # just one kernel
make -C src/cmd/ls                    # just one utility
make fs                               # just repack sdcard.img
make clean                            # object files
make cleanall                         # also installed binaries and sdcard.img
```

Kernel configuration for each board is described by its `Config` file — devices, pins, clock
speeds and options. After editing one, regenerate the board's makefile:

```shell
make -C sys/pic32/explorer16 reconfig
```

The file format is documented on the [kconfig wiki page](http://retrobsd.org/wiki/doku.php/doc/kconfig).

## License

BSD 2-clause style. See [LICENSE](LICENSE).
