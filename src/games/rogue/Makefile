#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific written prior permission. This software
# is provided ``as is'' without express or implied warranty.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -mips16
CFLAGS		+= -Os -DUNIX -DUNIX_BSD4_2 -DCURSES -DANSI -Werror -Wall

OBJS            = curses.o	\
                  doprnt.o	\
                  hit.o		\
                  init.o	\
                  inventory.o	\
                  level.o	\
                  machdep.o	\
                  main.o	\
                  message.o	\
                  monster.o	\
                  move.o	\
                  object.o	\
                  pack.o	\
                  play.o	\
                  random.o	\
                  ring.o	\
                  room.o	\
                  save.o	\
                  score.o	\
                  spec_hit.o	\
                  throw.o	\
                  trap.o	\
                  use.o		\
                  zap.o
MAN		= rogue.0
MANSRC		= rogue.6

all:            rogue $(MAN)

rogue:          ${OBJS}
		${CC} ${LDFLAGS} -o rogue.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S rogue.elf > rogue.dis
		${SIZE} rogue.elf
		${ELF2AOUT} rogue.elf $@ && rm rogue.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} rogue *.elf *.dis tags *~

install:        all
		install rogue $(DESTDIR)/games/
		cp ${MAN} $(DESTDIR)/share/man/cat6/

#$(OBJS):        rogue.h
