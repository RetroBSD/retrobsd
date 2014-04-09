#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= startup-$(MACHINE) libc libm libutil libtermlib libcurses \
                  libvmf libwiznet share cmd games

all:		$(SUBDIR)

$(SUBDIR):	FRC
		$(MAKE) -C $@

FRC:

install:        elf32-mips.ld
		-for i in $(SUBDIR); do \
			$(MAKE) -C $$i DESTDIR=$(DESTDIR) all install; done
#		cp -p elf32-mips.ld $(DESTDIR)/lib/elf32-mips.ld

clean:
		rm -f a.out core *.s *.o *.a *~
		-for i in $(SUBDIR); do $(MAKE) -C $$i clean; done
