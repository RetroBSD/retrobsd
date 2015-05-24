# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# This makefile is designed to be run as:
#	make
#
# The `make' will compile everything, including a kernel, utilities
# and a root filesystem image.

#
# Supported boards
#
MAX32           = sys/pic32/max32/MAX32
FUBARINO        = sys/pic32/fubarino/FUBARINO
FUBARINOBIG     = sys/pic32/fubarino/FUBARINO-UART2CONS-UART1-SRAMC
SDXL            = sys/pic32/sdxl/SDXL
MAXIMITE        = sys/pic32/maximite/MAXIMITE
MAXCOLOR        = sys/pic32/maximite-color/MAXCOLOR
DUINOMITE       = sys/pic32/duinomite/DUINOMITE
DUINOMITEUART   = sys/pic32/duinomite-uart/DUINOMITE-UART
DUINOMITEE      = sys/pic32/duinomite-e/DUINOMITE-E
DUINOMITEEUART  = sys/pic32/duinomite-e-uart/DUINOMITE-E-UART
MMBMX7          = sys/pic32/mmb-mx7/MMB-MX7
WF32            = sys/pic32/wf32/WF32
UBW32           = sys/pic32/ubw32/UBW32
UBW32UART       = sys/pic32/ubw32-uart/UBW32-UART
UBW32UARTSDRAM  = sys/pic32/ubw32-uart-sdram/UBW32-UART-SDRAM
EXPLORER16      = sys/pic32/explorer16/EXPLORER16
STARTERKIT      = sys/pic32/starter-kit/STARTER-KIT
BAREMETAL       = sys/pic32/baremetal/BAREMETAL
RETROONE        = sys/pic32/retroone/RETROONE

# Select target board
TARGET          ?= $(MAX32)

# Filesystem and swap sizes.
FS_MBYTES       = 100
U_MBYTES        = 100
SWAP_MBYTES     = 2

# Set this to the device name for your SD card.  With this
# enabled you can use "make installfs" to copy the sdcard.img
# to the SD card.

#SDCARD          = /dev/sdb

#
# C library options: passed to libc makefile.
# See lib/libc/Makefile for explanation.
#
DEFS		=

FSUTIL		= tools/fsutil/fsutil

-include Makefile.user

TARGETDIR    = $(shell dirname $(TARGET))
TARGETNAME   = $(shell basename $(TARGET))
TOPSRC       = $(shell pwd)
CONFIG       = $(TOPSRC)/tools/configsys/config

all:            .profile
		$(MAKE) -C tools
		$(MAKE) -C lib
		$(MAKE) -C src install
		$(MAKE) kernel
		$(MAKE) fs

kernel: 	$(TARGETDIR)/Makefile
		$(MAKE) -C $(TARGETDIR)

$(TARGETDIR)/Makefile: $(CONFIG) $(TARGETDIR)/$(TARGETNAME)
		cd $(TARGETDIR) && ../../../tools/configsys/config $(TARGETNAME)

fs:             sdcard.img

.PHONY:         sdcard.img
sdcard.img:	$(FSUTIL) rootfs.manifest userfs.manifest
		rm -f $@
		$(FSUTIL) --repartition=fs=$(FS_MBYTES)M:swap=$(SWAP_MBYTES)M:fs=$(U_MBYTES)M $@
		$(FSUTIL) --new --partition=1 --manifest=rootfs.manifest $@ .
# In case you need a separate /u partition,
# uncomment the following line.
#		$(FSUTIL) --new --partition=3 --manifest=userfs.manifest $@ u

$(FSUTIL):
		cd tools/fsutil; $(MAKE)

$(CONFIG):
		make -C tools/configsys

clean:
		rm -f *~
		for dir in tools lib src sys/pic32; do $(MAKE) -C $$dir -k clean; done

cleanall:       clean
		$(MAKE) -C lib clean
		rm -f sys/pic32/*/unix.hex bin/* sbin/* libexec/*
		rm -f games/[a-k]* games/[m-z]* share/man/cat*/*
		rm -f games/lib/adventure.dat games/lib/cfscores
		rm -f share/re.help share/emg.keys share/misc/more.help
		rm -f etc/termcap etc/remote etc/phones
		rm -f tools/configsys/.depend
		rm -f var/log/aculog
		rm -rf var/lock share/unixbench

installfs:
ifdef SDCARD
		@[ -f sdcard.img ] || $(MAKE) sdcard.img
		sudo dd bs=32k if=sdcard.img of=$(SDCARD)
else
		@echo "Error: No SDCARD defined."
endif

# TODO: make it relative to Target
installflash:
		sudo pic32prog sys/pic32/fubarino/unix.hex

# TODO: make it relative to Target
installboot:
		sudo pic32prog sys/pic32/fubarino/bootloader.hex

.profile:       etc/root/dot.profile
		cp etc/root/dot.profile .profile
