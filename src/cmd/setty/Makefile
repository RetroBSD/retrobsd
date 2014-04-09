#==========================================
#    Makefile: makefile for setty
#	Copyright 2012 Majenko Technolohies
#		(matt@majenko.co.uk
#	Last Modified: 29/01/2012
#==========================================

TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

OBJS            = setty.o
SRCS            = setty.c
LIBS            += -lcurses -ltermcap

all:            setty

setty:          ${OBJS}
		${CC} ${LDFLAGS} -o setty.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S setty.elf > setty.dis
		${SIZE} setty.elf
		${ELF2AOUT} setty.elf $@ && rm setty.elf

clean:
		-rm -f setty ${OBJS} setty.elf setty.dis

install:        all
		install setty $(DESTDIR)/bin/
