#==========================================
#    Makefile: makefile for sl
#	Copyright 1993,1998 Toyoda Masashi
#		(toyoda@is.titech.ac.jp)
#	Last Modified: 1998/ 7/22
#==========================================

TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

OBJS            = sl.o
SRCS            = sl.c
LIBS            += -lcurses -ltermcap -lc

all:            sl

sl:             ${OBJS}
		${CC} ${LDFLAGS} -o sl.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S sl.elf > sl.dis
		${SIZE} sl.elf
		${ELF2AOUT} sl.elf $@ && rm sl.elf

clean:
		-rm -f sl ${OBJS} sl.elf sl.dis

install:        all
		install sl $(DESTDIR)/bin/
