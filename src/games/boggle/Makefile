#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (Berkeley) 9/7/85
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS  	+= -O -Wall -Werror
OBJS    	= boggle.o
MAN             = boggle.0
MANSRC          = boggle.6

all:            boggle bogdict ${MAN}

boggle:         ${OBJS}
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

bogdict:        comp sfile
		sed -f sfile ../../../share/dict/words | uniq | ./comp > bogdict

comp:           comp.c
		cc -O comp.c -o comp

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o core boggle comp bogdict

install:
		install boggle $(DESTDIR)/games/
		install -m 644 bogdict $(DESTDIR)/games/lib/bogdict
		cp ${MAN} $(DESTDIR)/share/man/cat6/
