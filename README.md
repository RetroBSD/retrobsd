# This is the RetroBSD source directory.

## Source Roadmap

    bin         User commands.
    etc         Template files for /etc.
    include     System include files.
    lib         System libraries.
    libexec     System binaries.
    sbin        System administration commands.
    share       Shared resources.
    sys         Kernel sources.
    tools       Build tools and simulators.


## Supported hardware

 * Olimex PIC32-RetroBSD board.
 * Microchip Explorer 16 board, with PIC32 CAN-USB plug-in module and SD & MMC pictail.


## Build

To compile everything from sources, you'll need some packages installed, namely:
Clang compiler, LLVM linker, Berkeley YACC, GNU bison, flex, groff and ELF library.
Under Ubuntu, for example, you can do it by command:
```shell
$ sudo apt-get install clang lld byacc bison flex groff-base libelf-dev
```

Under MacOS:
```shell
$ brew install llvm@18 lld@18 byacc bison flex groff libelf
```

Build everything with single-threaded make:
```shell
$ make -j1
```

A resulting root filesystem image is in file `sdcard.img`.
A kernel is in file `unix.hex` in your target board subdirectory.


### Filesystem image

You can put the file system image on the SD card using the [balenaEtcher](https://etcher.balena.io) utility.

### Install kernel

Kernel image should be written to PIC32 flash memory.  The procedure depends
on a board used.

#### Olimex PIC32-RetroBSD board:
Use the [pic32prog](https://github.com/sergev/pic32prog) utility and a USB cable
to install the kernel:

```shell
$ pic32prog -d /dev/ttyUSB0 sys/pic32/olimex/unix.hex
```

#### Explorer 16 board:
Use Pickit2 clone and [pic32prog](https://github.com/sergev/pic32prog) utility
to install the kernel:

``` shell
$ pic32prog sys/pic32/explorer16/unix.hex
```

## Simulate
RetroBSD can be simulated using [QEMU for pic32](https://github.com/sergev/qemu).
Use kernel for Explorer 16 board. For example:
```
$ qemu-pic32 -machine pic32mx7-explorer16 -nographic -monitor none \
    -serial vc -serial stdio -kernel unix.hex -sd sdcard.img
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
