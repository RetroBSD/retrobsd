Pic32prog is a utility for loading data into a flash memory of Microchip PIC32
microcontrollers.  Supported adapters and bootloaders:
 * Microchip PICkit2
 * Microchip PICkit3 with scripting firmware
 * Olimex ARM-USB-Tiny (not finished yet)
 * Olimex ARM-USB-Tiny, ARM-USB-Tiny-H and ARM-USB-OCH-H JTAG adapters
 * Olimex MIPS-USB-OCH-H JTAG adapter
 * Bus Blaster v2 JTAG adapter from Dangerous Prototypes
 * Flyswatter JTAG adapter from TinCanTools
 * AN1388 HID bootloader
 * Legacy FS_USB HID bootloader


=== Usage ===

When called without parameters, pic32prog utility detects a type of microcontroller
and device configuration.  For example:

    % pic32prog
    Programmer for Microchip PIC32 microcontrollers, Version 1.0
        Copyright: (C) 2011 Serge Vakulenko
          Adapter: PICkit2 Version 2.32.0
        Processor: 795F512L (id 04307053)
     Flash memory: 512 kbytes
    Configuration:
        DEVCFG0 = 7ffffffd
                         1 Debugger enabled
                         8 Use PGC2/PGD2
        DEVCFG1 = ff6afd5b
                         3 Primary oscillator with PLL
                       1   Primary oscillator: XT
                       4   CLKO output active
                      3    Peripheral bus clock: SYSCLK / 8
                      4    Fail-safe clock monitor disable
                      8    Clock switching disable
                     a     Watchdog postscale: 1/1024
        DEVCFG2 = fff8f9d9
                         1 PLL divider: 1/2
                        5  PLL multiplier: 20x
                       1   USB PLL divider: 1/2
                      8    Disable USB PLL
                           Enable USB PLL
                     0     PLL postscaler: 1/1
        DEVCFG3 = 3affffff
                     7     Assign irq priority 7 to shadow set
                           Ethernet RMII enabled
                   2       Default Ethernet i/o pins
                           Alternate CAN i/o pins
                           USBID pin: controlled by port
                           VBuson pin: controlled by port

Writing to flash memory:

    pic32prog [-v] file.srec
    pic32prog [-v] file.hex

Reading memory to file:

    pic32prog -r file.bin address length

Parameters:

    file.srec   - file with firmware in SREC format
    file.srec   - file with firmware in Intel HEX format
    file.bin    - binary file
    address     - address in memory
    -v          - verify only (no write)
    -r          - read mode

Input file should have format SREC or Intel HEX.
You can convert ELF format (also COFF or A.OUT) to SREC using objcopy utility,
for example:

    objcopy -O srec firmware.elf firmware.srec


=== Sources ===

Sources are distributed under the terms of GPL.
You can download sources using Git:

    git clone https://github.com/sergev/pic32prog.git

To build it on Ubuntu, a few additional packages need
to be installed:

    sudo apt-get install libusb-dev libusb-1.0-0-dev libudev-dev

___
Regards,
Serge Vakulenko
