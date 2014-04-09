#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Os -mips16 -Werror #-Wall
CFLAGS          += -DPATH=\"${DEST}\" -DWIZARD=\"daemon\" -DUID=1 \
                   -DRAND=32768.0 -DACCESS=\"r+\" -DENEMY -DBSD42 -O

OBJS            = main.o func0.o func1.o func2.o fight.o
LIBS            = -lm -lcurses -ltermcap
MAN		= phantasia.0
MANSRC		= phant.nr

all:            phantasia $(MAN)

phantasia:      ${OBJS}
		${CC} ${LDFLAGS} -o phantasia.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S phantasia.elf > phantasia.dis
		${SIZE} phantasia.elf
		${ELF2AOUT} phantasia.elf $@ && rm phantasia.elf

${MAN}:         ${MANSRC}
		tbl -TX $< | ${MANROFF} > phantasia.0

clean:
		rm -f *.o *.0 *.elf ${MAN} phantasia setfiles phant.help *.elf *.dis tags *~

install:        all
		install phantasia $(DESTDIR)/games/
		cp ${MAN} $(DESTDIR)/share/man/cat6/
