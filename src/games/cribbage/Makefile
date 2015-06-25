#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.1 (2.11BSD) 1996/1/28
#
# make file for cribbage
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -Werror -DCROSS

CFLAGS          += -Wall -Werror
OBJS            = extern.o crib.o support.o cards.o score.o io.o
TOBJS           = test.o cards.o score.o io.o extern.o
MAN             = cribbage.0
MANSRC		= cribbage.6
LIBS            = -lcurses -ltermcap -lc

all:            cribbage ${MAN} crib.instr

cribbage:       ${OBJS}
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

crib.instr:     cribbage.n macro
		nroff cribbage.n > $@

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o core cribbage ${MAN} *.dis crib.instr

install:        all
		install cribbage $(DESTDIR)/games/
		install -m 644 crib.instr $(DESTDIR)/games/lib/crib.instr
		cp ${MAN} $(DESTDIR)/share/man/cat6/
