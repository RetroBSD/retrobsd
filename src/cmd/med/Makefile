#==========================================
#    Makefile: makefile for med
#	Copyright 2012 Majenko Technolohies
#		(matt@majenko.co.uk
#	Last Modified: 29/01/2012
#==========================================

TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

OBJS            = med.o
SRCS            = med.c
LIBS            += -lcurses -ltermcap -lvmf -lc

all:            med

med: 		${OBJS}
		${CC} ${LDFLAGS} -o med.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S med.elf > med.dis
		${SIZE} med.elf
		${ELF2AOUT} med.elf $@ && rm med.elf

clean:
		-rm -f med ${OBJS} med.elf med.dis

install:        all
		install med $(DESTDIR)/bin/
