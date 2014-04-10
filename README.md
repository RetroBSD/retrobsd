This is the RetroBSD source directory.
======================================

Source Roadmap
--------------
    bin         User commands.
    etc         Template files for /etc.
    include     System include files.
    lib         System libraries.
    libexec     System binaries.
    sbin        System administration commands.
    share       Shared resources.
    sys         Kernel sources.
    tools       Build tools and simulators.


Supported hardware
------------------
 * chipKIT Max32 board.
 * Sparkfun UBW32 board.
 * Maximite and Colour Maximite computers.
 * Microchip Explorer 16 board, with PIC32 CAN-USB plug-in module and SD & MMC pictail.
 * Microchip PIC32 USB or Ethernet Starter Kit, with I/O Expansion board and SD & MMC pictail.
 * Olimex Duinomite, Duinomite-Mini and Duinomite-Mega boards.
 * Olimex Pinguino-Micro board with PIC32MX795F512H microcontroller.
 * eflightworks DIP board.


Build
-----
By default, the system is configured for the Max32 board.
To select another target board, edit a top-level user-specific Makefile called "Makefile.user"
and set a TARGET value:

    TARGET = $(UBW32)          # for the UBW32 board with USB console
    TARGET = $(UBW32UART)      # for the UBW32 board with UART console
    TARGET = $(UBW32UARTSDRAM) # for the UBW32 boars with UART console and 8MB SRAM ramdisk
    TARGET = $(MAXIMITE)       # for the Maximite board
    TARGET = $(MAXCOLOR)       # for the Colour Maximite board
    TARGET = $(EXPLORER16)     # for the Explorer 16 board
    TARGET = $(STARTERKIT)     # for the PIC32 USB or Ethernet Starter Kit
    TARGET = $(MAX32)          # default
    TARGET = $(DUINOMITE)      # for the Duinomite board with USB console
    TARGET = $(DUINOMITEUART)  # for the Duinomite board with UART console
    TARGET = $(DUINOMITEE)     # for the Duinomite E board with USB console
    TARGET = $(DUINOMITEEUART) # for the Duinomite E board with UART console
    TARGET = $(PINGUINO)       # for the Pinguino-Micro board
    TARGET = $(DIP)            # for the DIP board
    TARGET = $(BAREMETAL)      # Bare PIC32 chip on a breakout board
    TARGET = $(FUBARINO)       # Fubarino SD board
    TARGET = $(FUBARINOBIG)    # Fubarino SD board with 8MB SRAM RAMDISK
    TARGET = $(MMBMX7)         # MMB MX7 board


You can also change a desired filesystem size and swap area size,
as required.  Default is:

    FS_KBYTES   = 16384
    SWAP_KBYTES = 2048

To compile the kernel and build a filesystem image, run:

    make

A resulting root filesystem image is in file `sdcard.rd`.
A kernel is in file `unix.hex` in your target board subdirectory.


Filesystem image
~~~~~~~~~~~~~~~~
You need to put a filesystem image on a SD card.  On Windows, use
Win32DiskImager utility (https://launchpad.net/win32-image-writer/+download).
On Linux, run:

    sudo dd if=sdcard.rd of=/dev/XYZ

Here `XYZ` is a device name of SD card, as recognized by Linux (sdb in my case).


Install kernel
~~~~~~~~~~~~~~
Kernel image should be written to PIC32 flash memory.  The procedure depends
on a board used.

Max32 board:
    cd sys/pic32/ubw32
    AVRTOOLS=/Applications/Mpide.app/Contents/Resources/Java/hardware/tools
    $AVRTOOLS/bin/avrdude -C$AVRTOOLS/etc/avrdude.conf -c stk500v2 -p pic32 \
        -P /dev/tty.usbserial-* -b 115200 -v -U flash:w:unix.hex:i

    Here you need to change AVRTOOLS path and tty name according to your system.

UBW32 board:
    Use a pic32prog utility (http://code.google.com/p/pic32prog/)
    and a USB cable to install a kernel:

    pic32prog sys/pic32/ubw32/unix.hex

Maximite:
    Use the bootload program for Windows, available for download by link:
    http://geoffg.net/Downloads/Maximite/Maximite_Update_V2.7B.zip

Explorer 16 board:
    There is an auxiliary PIC18 chip on the Explorer 16 board, which can be
    used as a built-in programmer device.  You will need a PICkit 2 adapter
    to install a needed firmware, as described in article:
    http://www.paintyourdragon.com/?p=51
    (section "Hack #2: Lose the PICkit 2, Save $35").
    This should be done only once.

    Then, you can use a pic32prog utility (http://code.google.com/p/pic32prog/)
    and a USB cable to install a kernel:

    pic32prog sys/pic32/explorer16/unix.hex

PIC32 Starter Kit:
    Use PICkit 2 adapter and software to install a boot loader from
    file `sys/pic32/starter-kit/boot.hex`.  This should be done only once.

    Then, you can use a pic32prog utility (http://code.google.com/p/pic32prog/)
    and a USB cable to install a kernel:

    pic32prog sys/pic32/starter-kit/unix.hex


Simulator
---------
You can use a MIPS32 simulator to develop a debug a RetroBSD software,
without a need for hardware board.  By default, a simulator is configured
to imitate a Max32 board.  To build it:

    cd tools/virtualmips
    make

Run it:

    ./pic32

Configuration of simulated board is stored in file `pic32_max32.conf`.

Build packages
--------------

For building under Ubuntu you need the following packages installed:

    $ sudo apt-get install byacc libelf-dev

