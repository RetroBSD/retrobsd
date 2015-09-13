# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# This makefile is designed to be run as:
#	make
#
# The `make' will compile everything, including a kernel, utilities
# and a root filesystem image.

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

TOPSRC       = $(shell pwd)
CONFIG       = $(TOPSRC)/tools/configsys/config

all:
		$(MAKE) -C tools
		$(MAKE) -C lib
		$(MAKE) -C src install
		$(MAKE) kernel
		$(MAKE) fs

kernel:         $(CONFIG)
		$(MAKE) -C sys/pic32 all

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
		rm -f var/log/aculog sdcard.img
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
