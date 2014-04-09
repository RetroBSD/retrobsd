#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2	(2.11BSD)	1999/10/25
#

DESTDIR =
CFLAGS  = -O -DSYSV -DDEST=\"${DESTDIR}/usr/games/lib/atc/\"
YFLAGS  = -d
LIBS    = -ll -lm -lcurses -ltermcap
OBJS    = extern.o grammar.o input.o lex.o list.o log.o main.o tunable.o \
	  graphics.o update.o

all:    atc

atc:    ${OBJS}
	${CC} ${OBJS} ${LIBS} -o $@

clean:
	rm -f *.o core atc grammar.c y.tab.h y.tab.c lex.yy.c lex.c

install:
	-[ -d ${DESTDIR}/usr/games/lib/atc ] || mkdir ${DESTDIR}/usr/games/lib/atc
	chmod 700 ${DESTDIR}/usr/games/lib/atc
	chown games.bin ${DESTDIR}/usr/games/lib/atc
	(cd games; install -c -o games -g bin -m 600 * ${DESTDIR}/usr/games/lib/atc)
	install -s -o games -g bin -m 4700 atc ${DESTDIR}/usr/games/hide/atc
	(cd ${DESTDIR}/usr/games; rm -f atc; ln -s dm atc; chown games.bin atc)
