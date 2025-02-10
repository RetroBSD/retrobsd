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
