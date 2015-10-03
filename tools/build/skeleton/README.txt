
RetroBSD is a version of 2.11BSD Unix for microcontrollers.
It can run on PIC32 in only 128 kbytes of RAM. The operating
system includes not only a set of basic Unix utilities, but also
a C compiler, assembler, linker, libraries and include files
sufficient to develop user applications directly on the target
processor. Cross-compile is possible as well.

Files in this package:

    unix.hex          - Unix kernel image
    unix.dis          - Disassembly of the kernel image, for debugging
    sdcard.img        - Root filesystem image
    pic32prog.exe     - PIC32 programmer utility for Windows
    linux32/pic32prog - PIC32 programmer utility for 32-bit Linux
    linux64/pic32prog - PIC32 programmer utility for 64-bit Linux
    macos/pic32prog   - PIC32 programmer utility for Mac OS X
    pic32prog.txt     - Brief description of pic32prog utility
    README.txt        - This file

The installation of RetroBSD to your board consists of
three (or four) steps:

(0) Optional: Restore a bootloader on your board
(1) Transfer the Unix kernel to the board
(2) Put the filesystem image on to a SD card
(3) Connect to the console port and start RetroBSD


(0) Optional: Restore a bootloader on your board
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Most PIC32 boards come with some kind of bootloader preinstalled.
RetroBSD relies on a presence of the bootloader on these boards.
In case you have lost or damaged your bootloader,
you may use pic32prog and pickit2 programmer to restore it.
For the latest bootloader ask at the RetroBSD forum.

Do NOT use pic32prog and pickit2 programmer (or any other one)
to flash unix.hex from Autobuild versions into the chip - it
will NOT work.

In order to get a unix.hex version for these boards able to function
without a bootloader you have to build the whole system by yourself
and configure the unix.hex appropriately. Use configuration
sys/pic32/baremetal/Config as an example.
Mind the clock settings are currently done in the bootloader.


(1) Transfer the Unix kernel on to the board
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For working with USB based bootloaders under Windows you may
require an USB driver. For example, for Win7(64b) you may use
"Stk500v2.inf" file from http://chipkit.net/forum/viewtopic.php?t=2289
You need to install the USB driver by pointing the installer to the
Stk500v2.inf file.

Connect the board via microUSB cable to your computer. To enter
a bootloader mode, press the PRG key first, hold it, then press
the RESET key. The LED will now flash and your board is ready
to accept new code. Use pic32prog utility to program the flash
code. The bootloader can appear as a HID device, or as a virtual
serial port (COM port) on your computer, depending on your
board type.

Boards which use HID bootloader: Maximite, Color Maximite,
Duinomite, CGMMSTICK1, SnadPIC. For these boards you do not
need to specify the -d option and device name.

Boards which use a virtual serial port for bootloader: Fubarino
SD, chipKIT Max32, Picadillo 35T, Majenko SDXL. For these
boards you need to know the exact device name to specify for
pic32prog (typically /dev/ACM0 on Linux, something like COM12
on Windows, /dev/tty.usbmodemfa131 on Mac OS X).

Use proper pic32prog binary for your operating system:

    pic32prog.exe     - for Windows
    linux32/pic32prog - for 32-bit Linux
    linux64/pic32prog - for 64-bit Linux
    macos/pic32prog   - for Mac OS X

Unpack the package and run command (say, for 64-bit Linux):

    linux64/pic32prog -d /dev/ttyACM0 unix.hex

For Windows, it should be like:

    pic32prog -d COM12 unix.hex

On Mac OS X:

    macosx/pic32prog -d /dev/tty.usbmodemfa131 unix.hex

You should see:

    Programmer for Microchip PIC32 microcontrollers, Version 1.112
        Copyright: (C) 2011-2014 Serge Vakulenko
          Adapter: STK500v2 Bootloader
     Program area: 1d000000-1d07ffff
        Processor: Bootloader
     Flash memory: 512 kbytes
             Data: 153384 bytes
            Erase: done
    Program flash: ###################################### done
     Verify flash: ###################################### done
    Rate: 9307 bytes per second


(2) Put the filesystem image on to a SD card
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Use any USB-to-SD card reader to attach the SD card to your PC.

a) On Windows, use "Win32DiskImager" utility, version 0.9.5 works
fine under Win7(64b).

b) On Linux or Mac OS X, run:

    sudo dd bs=32k if=sdcard.img of=/dev/XYZ

where XYZ is a name of SD card on your computer (use lsblk or
"diskutil list" to obtain).

Once that is done remove the SD card from card reader and plug
it into SD slot on your board. The RetroBSD system is ready
to run.


(3) Connect to the console port and start RetroBSD
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Connect USB cable to the board and to your computer. The RetroBSD
console will appear as a virtual COM port on your computer. Use
any terminal emulation program (like putty or TeraTerm on Windows,
or minicom on Linux) to connect to this virtual COM port at baud
rate 115200. Press <Enter> to start RetroBSD. On Login prompt,
enter "root". Password is empty:

    2.11 BSD Unix for PIC32, revision G226 build 1:
         Compiled 2015-05-19 by vak@tundra.local:
         /Users/vak/retrobsd-github/sys/pic32/fubarino
    cpu: 795F512H 80 MHz, bus 80 MHz
    oscillator: HS crystal, PLL div 1:2 mult x20
    console: ttyUSB0 (5,0)
    sd0: port SPI2, select pin G9
    sd0: type SDHC, size 3914752 kbytes, speed 13 Mbit/sec
    phys mem  = 128 kbytes
    user mem  = 96 kbytes
    root dev  = rd0a (0,1)
    root size = 102400 kbytes
    swap dev  = rd0b (0,2)
    swap size = 2048 kbytes
    /dev/rd0a: 690 files, 10880 used, 91119 free
    Starting daemons: update cron


    2.11 BSD UNIX (pic32) (console)

    login: root
    Password:
    Welcome to RetroBSD!
    erase, kill ^U, intr ^C
    # _
