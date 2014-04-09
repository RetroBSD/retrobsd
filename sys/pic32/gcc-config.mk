# chipKIT PIC32 compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Download from https://github.com/jasonkajita/chipKIT-cxx/downloads
# and unzip to /usr/local.
# Need to copy pic32-tools/pic32mx/include/stdarg.h
# to pic32-tools/lib/gcc/pic32mx/4.5.1/include.
# MPLABX C32 compiler doesn't support some functionality
# we need, so use chipKIT compiler by default.
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/pic32-tools/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
endif

# Generic MIPS toolchain
# ~~~~~~~~~~~~~~~~~~~~~~
# You can build it from sources, as described on page
# http://retrobsd.org/wiki/doku.php/doc/toolchain-mips
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/mips-gcc-4.7.2/bin/mips-elf-
    LDFLAGS     =
endif

# chipKIT MPIDE on Mac OS X
ifneq (,$(wildcard /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr))
    AVRDUDE     = /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr/bin/avrdude \
                  -C /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr/etc/avrdude.conf \
                  -P /dev/tty.usbserial-*
endif

# chipKIT MPIDE on Linux
ifneq (,$(wildcard /opt/mpide-0023-linux-20120903/hardware/tools/avrdude))
    AVRDUDE     = /opt/mpide-0023-linux-20120903/hardware/tools/avrdude \
                  -C /opt/mpide-0023-linux-20120903/hardware/tools/avrdude.conf \
                  -P /dev/ttyUSB0
endif
