#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.1 (2.11BSD GTE) 1/16/95
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -g -DCROSS

CFLAGS  	+= -Werror -Wall

OBJS            = snake.o move.o
MAN             = snake.0
LIBS            = -lm -ltermcap -lc

all:            snake snscore $(MAN)

snake:          $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

snscore:        snscore.o
		$(CC) $(LDFLAGS) -o $@.elf snscore.o $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         snake.6
		nroff -man $< > $@

clean:
		rm -f *.o core snake snscore *.0 *.dis

install:        all
		install snake $(DESTDIR)/games/
		install snscore $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
		ln -f $(DESTDIR)/share/man/cat6/$(MAN) ${DESTDIR}/share/man/cat6/snscore.0
		install -c -m 644 /dev/null ${DESTDIR}/games/lib/snakerawscores
