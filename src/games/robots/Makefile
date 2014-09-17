#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (Berkeley) 5/15/86
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -DCROSS -g -DFANCY

CFLAGS  	+= -DMAX_PER_UID=5 -Werror -Wall

OBJS            = extern.o init_field.o main.o make_level.o move.o \
		  move_robs.o play_level.o query.o rnd_pos.o score.o \
		  flush_in.o
MAN             = robots.0
LIBS            = -lcurses -ltermcap -lc

all:            robots $(MAN)

robots:         $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         robots.6
		nroff -man $< > $@

clean:
		rm -f *.o core robots *.0 *.dis

install:        all
		install robots $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
		install -c -m 644 /dev/null ${DESTDIR}/games/lib/robots_roll
###
extern.o: extern.c robots.h
flush_in.o: flush_in.c
init_field.o: init_field.c robots.h
main.o: main.c robots.h
make_level.o: make_level.c robots.h
move.o: move.c robots.h
move_robs.o: move_robs.c robots.h
play_level.o: play_level.c robots.h
query.o: query.c robots.h
rnd_pos.o: rnd_pos.c robots.h
score.o: score.c robots.h
