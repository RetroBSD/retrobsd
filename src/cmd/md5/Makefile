#==========================================
#    Makefile: makefile for md5
#	Copyright 1993,1998 Toyoda Masashi
#		(toyoda@is.titech.ac.jp)
#	Last Modified: 1998/ 7/22
#==========================================

TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

OBJS            = md5.o md5c.o
SRCS            = md5.c md5c.c
MAN		= md5.0
MANSRC		= md5.1

all:            md5 $(MAN)

md5:             ${OBJS}
		${CC} ${LDFLAGS} -o md5.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S md5.elf > md5.dis
		${SIZE} md5.elf
		${ELF2AOUT} md5.elf $@ && rm md5.elf

$(MAN):         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		-rm -f md5 ${OBJS} ${MAN} md5.elf md5.dis

install:        all
		install md5 $(DESTDIR)/bin/
		cp $(MAN) $(DESTDIR)/share/man/cat1
