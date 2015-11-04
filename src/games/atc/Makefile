#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2	(2.11BSD)	1999/10/25
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS          += -Os -g -Werror -Wall
CFLAGS          += -DBSD -DDEST=\"/games/lib/atc/\"
#CFLAGS          += -DSYSV -DDEST=\"games/\"
YFLAGS          = -d
LEX             = lex
LIBS            = -lm -lcurses -ltermcap -lc
OBJS            = extern.o grammar.o input.o lex.o list.o log.o main.o tunable.o \
                  graphics.o update.o
MAN		= atc.0
MANSRC		= atc.6

all:            atc $(MAN)

atc:            $(OBJS)
		$(CC) $(LDFLAGS) -o atc.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S atc.elf > atc.dis
		$(SIZE) atc.elf
		$(ELF2AOUT) atc.elf $@ && rm atc.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf atc ${MAN} *.elf *.dis tags *~ grammar.c lex.c y.tab.h

install:        all
		install atc $(DESTDIR)/games/
		mkdir -p ${DESTDIR}/games/lib/atc
		cp -p games/* ${DESTDIR}/games/lib/atc
		cp ${MAN} $(DESTDIR)/share/man/cat6/
