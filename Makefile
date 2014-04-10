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
UBW32           = sys/pic32/ubw32/UBW32
UBW32UART       = sys/pic32/ubw32-uart/UBW32-UART
UBW32UARTSDRAM  = sys/pic32/ubw32-uart-sdram/UBW32-UART-SDRAM
MAXIMITE        = sys/pic32/maximite/MAXIMITE
MAXCOLOR        = sys/pic32/maximite-color/MAXCOLOR
EXPLORER16      = sys/pic32/explorer16/EXPLORER16
STARTERKIT      = sys/pic32/starter-kit/STARTER-KIT
DUINOMITE       = sys/pic32/duinomite/DUINOMITE
DUINOMITEUART   = sys/pic32/duinomite-uart/DUINOMITE-UART
DUINOMITEE      = sys/pic32/duinomite-e/DUINOMITE-E
DUINOMITEEUART  = sys/pic32/duinomite-e-uart/DUINOMITE-E-UART
PINGUINO        = sys/pic32/pinguino-micro/PINGUINO-MICRO
DIP             = sys/pic32/dip/DIP
BAREMETAL       = sys/pic32/baremetal/BAREMETAL
RETROONE	    = sys/pic32/retroone/RETROONE
FUBARINO	    = sys/pic32/fubarino/FUBARINO
FUBARINOBIG	    = sys/pic32/fubarino/FUBARINO-UART2CONS-UART1-SRAMC
MMBMX7          = sys/pic32/mmb-mx7/MMB-MX7

# Select target board
TARGET          ?= $(MAX32)

# Filesystem and swap sizes.
FS_KBYTES       = 102400
U_KBYTES        = 102400
SWAP_KBYTES     = 2048

# Set this to the device name for your SD card.  With this
# enabled you can use "make installfs" to copy the filesys.img
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
#
# Filesystem contents.
#
BIN_FILES	:= $(wildcard bin/*)
SBIN_FILES	:= $(wildcard sbin/*)
GAMES_FILES	:= $(shell find games -type f ! -path '*/.*')
LIBEXEC_FILES	:= $(wildcard libexec/*)
LIB_FILES	:= lib/crt0.o lib/retroImage $(wildcard lib/*.a)
ETC_FILES	= etc/rc etc/rc.local etc/ttys etc/gettytab etc/group \
                  etc/passwd etc/shadow etc/fstab etc/motd etc/shells \
                  etc/termcap etc/MAKEDEV etc/phones etc/remote
INC_FILES	= $(wildcard include/*.h) \
                  $(wildcard include/sys/*.h) \
                  $(wildcard include/machine/*.h) \
                  $(wildcard include/smallc/*.h) \
                  $(wildcard include/smallc/sys/*.h) \
                  $(wildcard include/arpa/*.h)
SHARE_FILES	= share/re.help share/example/Makefile \
                  share/example/ashello.S share/example/chello.c \
                  share/example/blkjack.bas share/example/hilow.bas \
                  share/example/stars.bas share/example/prime.scm \
                  share/example/fact.fth share/example/echo.S \
                  $(wildcard share/smallc/*)
MANFILES	= share/man/ share/man/cat1/ share/man/cat2/ share/man/cat3/ \
		  share/man/cat4/ share/man/cat5/ share/man/cat6/ share/man/cat7/ \
		  share/man/cat8/ $(wildcard share/man/cat?/*)
ALLFILES	= $(SBIN_FILES) $(ETC_FILES) $(BIN_FILES) $(LIB_FILES) $(LIBEXEC_FILES) \
                  $(INC_FILES) $(SHARE_FILES) $(GAMES_FILES) \
                  var/log/messages var/log/wtmp .profile
ALLDIRS         = games/ sbin/ bin/ dev/ etc/ tmp/ lib/ libexec/ share/ include/ \
                  var/ u/ share/example/ share/misc/ share/smallc/ \
                  var/run/ var/log/ var/lock/ games/ games/lib/ include/sys/ \
                  include/machine/ include/arpa/ include/smallc/ \
                  include/smallc/sys/ share/misc/ share/smallc/ include/sys/ \
                  games/lib/
BDEVS           = dev/rd0!b0:0 dev/rd0a!b0:1 dev/rd0b!b0:2 dev/rd0c!b0:3 dev/rd0d!b0:4
BDEVS           += dev/rd1!b1:0 dev/rd1a!b1:1 dev/rd1b!b1:2 dev/rd1c!b1:3 dev/rd1d!b1:4
BDEVS           += dev/rd2!b2:0 dev/rd2a!b2:1 dev/rd2b!b2:2 dev/rd2c!b2:3 dev/rd2d!b2:4
BDEVS           += dev/rd3!b3:0 dev/rd3a!b3:1 dev/rd3b!b3:2 dev/rd3c!b3:3 dev/rd3d!b3:4
BDEVS		    += dev/swap!b4:64 dev/swap0!b4:0 dev/swap1!b4:1 dev/swap2!b4:2

D_CONSOLE        = dev/console!c0:0
D_MEM		     = dev/mem!c1:0 dev/kmem!c1:1 dev/null!c1:2 dev/zero!c1:3
D_TTY		     = dev/tty!c2:0
D_FD		     = dev/stdin!c3:0 dev/stdout!c3:1 dev/stderr!c3:2
D_TEMP           = dev/temp0!c4:0 dev/temp1!c4:1 dev/temp2!c4:2

U_DIRS           = $(addsuffix /,$(shell find u -type d ! -path '*/.svn*'))
U_FILES          = $(shell find u -type f ! -path '*/.svn/*')
#U_ALL            = $(patsubst u/%,%,$(U_DIRS) $(U_FILES))

CDEVS            = $(D_CONSOLE) $(D_MEM) $(D_TTY) $(D_FD) $(D_TEMP)

all:            tools build kernel
		$(MAKE) fs

fs:             sdcard.rd

.PHONY: 	tools
tools:
		$(MAKE) -C tools

kernel: 	$(TARGETDIR)/Makefile
		$(MAKE) -C $(TARGETDIR)

$(TARGETDIR)/Makefile: $(CONFIG) $(TARGETDIR)/$(TARGETNAME)
		cd $(TARGETDIR) && ../../../tools/configsys/config $(TARGETNAME)

.PHONY: 	lib
lib:
		$(MAKE) -C lib

build: 		tools lib
		$(MAKE) -C src install

filesys.img:	$(FSUTIL) $(ALLFILES)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) $@
		$(FSUTIL) -a $@ $(ALLDIRS)
		$(FSUTIL) -a $@ $(CDEVS)
		$(FSUTIL) -a $@ $(BDEVS)
		$(FSUTIL) -a $@ $(ALLFILES)
		$(FSUTIL) -a $@ $(MANFILES)

swap.img:
		dd if=/dev/zero of=$@ bs=1k count=$(SWAP_KBYTES)

user.img:	$(FSUTIL)
ifneq ($(U_KBYTES), 0)
		rm -f $@
		$(FSUTIL) -n$(U_KBYTES) $@
		(cd u; find . -type d -exec ../$(FSUTIL) -a ../$@ '{}/' \;)
		(cd u; find . -type f -exec ../$(FSUTIL) -a ../$@ '{}' \+)
endif

sdcard.rd:	filesys.img swap.img user.img
ifneq ($(U_KBYTES), 0)
		tools/mkrd/mkrd -out $@ -boot filesys.img -swap swap.img -fs user.img
else
		tools/mkrd/mkrd -out $@ -boot filesys.img -swap swap.img
endif

$(FSUTIL):
		cd tools/fsutil; $(MAKE)

$(CONFIG):
		make -C tools/configsys

clean:
		rm -f *~
		for dir in tools lib src sys/pic32; do $(MAKE) -C $$dir -k clean; done

cleanall:       clean
		$(MAKE) -C lib clean
		rm -f sys/pic32/*/unix.hex bin/* sbin/* games/[a-k]* games/[m-z]* libexec/* share/man/cat*/*
		rm -f games/lib/adventure.dat
		rm -f games/lib/cfscores
		rm -f share/re.help
		rm -f share/misc/more.help
		rm -f etc/termcap etc/remote etc/phones
		rm -rf share/unixbench
		rm -f games/lib/adventure.dat games/lib/cfscores share/re.help share/misc/more.help etc/termcap
		rm -f tools/configsys/.depend
		rm -f var/log/aculog
		rm -rf var/lock

installfs: filesys.img
ifdef SDCARD
	sudo dd bs=32k if=sdcard.rd of=$(SDCARD)
else
	@echo "Error: No SDCARD defined."
endif

# TODO: make it relative to Target
installflash: 
	sudo  pic32prog  sys/pic32/fubarino/unix.hex

# TODO: make it relative to Target
installboot: 
	sudo  pic32prog  sys/pic32/fubarino/bootloader.hex

.profile:   etc/root/dot.profile
		cp etc/root/dot.profile .profile
