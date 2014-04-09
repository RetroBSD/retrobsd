#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.1 (2.11BSD) 1996/1/28
#
# make file for cribbage
#
HDRS=	cribbage.h deck.h cribcur.h
OBJS=	extern.o crib.o support.o cards.o score.o io.o
CFILES=	extern.c crib.c support.c cards.c score.c io.c
TOBJS=	test.o cards.o score.o io.o extern.o
MAN=	crib.0
SEPFLAG= -i
CFLAGS=	-O
DESTDIR=

all:	cribbage ${MAN}

cribbage:	${OBJS}
	${CC} ${SEPFLAG} ${CFLAGS} -o cribbage ${OBJS} -lcurses -ltermlib

all: cribbage ${MAN}

crib.o io.o support.o: cribcur.h

test:		${TOBJS}
	${CC} ${SEPFLAG} ${CFLAGS} -o test ${TOBJS} -lcurses -ltermlib

crib.0: cribbage.n macro
	nroff cribbage.n > crib.0

tags: ${HDRS} ${CFILES}
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

clean:
	rm -f ${OBJS} ? a.out core crib.0 cribbage errs

crib.o:		deck.h	cribbage.h	cribcur.h
support.o:	deck.h	cribbage.h	cribcur.h
test.o:		deck.h
cards.o:	deck.h
score.o:	deck.h
io.o:		deck.h			cribcur.h

install: cribbage crib.0
	install -s cribbage $(DESTDIR)/usr/games
	install -m 644 crib.0 $(DESTDIR)/usr/games/lib/crib.instr
