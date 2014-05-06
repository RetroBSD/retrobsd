#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Os -Werror -Wall

OBJS            = abandon.o attack.o autover.o capture.o check_out.o checkcond.o \
                  compkl.o computer.o damage.o damaged.o dcrept.o destruct.o \
                  dock.o dumpgame.o dumpme.o dumpssradio.o events.o externs.o \
                  getcodi.o getpar.o help.o impulse.o initquad.o kill.o klmove.o \
                  lose.o lrscan.o main.o move.o nova.o out.o phaser.o play.o ram.o \
                  ranf.o rest.o schedule.o score.o setup.o setwarp.o shell.o \
                  shield.o snova.o srscan.o systemname.o torped.o utility.o \
                  visual.o warp.o win.o
LIBS            = -lm -lc
MAN		= trek.0
MANSRC		= trek.6

all:            trek $(MAN)

trek:           ${OBJS}
		${CC} ${LDFLAGS} -o trek.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S trek.elf > trek.dis
		${SIZE} trek.elf
		${ELF2AOUT} trek.elf $@ && rm trek.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} trek *.elf *.dis tags *~

install:        all
		install trek $(DESTDIR)/games/
		cp ${MAN} $(DESTDIR)/share/man/cat6/

#$(OBJS):        trek.h
