#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/01/26
#
MDIR=	/usr/man/cat6
SRCS=	aardvark.6 adventure.6 arithmetic.6 backgammon.6 banner.6 \
	bcd.6 bj.6 boggle.6 canfield.6 chess.6 ching.6 \
	cribbage.6 doctor.6 fish.6 fortune.6 hangman.6 hunt.6 mille.6 \
	monop.6 moo.6 number.6 quiz.6 rain.6 robots.6 rogue.6 \
	snake.6 trek.6 ttt.6 warp.6 words.6 worm.6 worms.6 wump.6 zork.6
OBJS=	aardvark.0 adventure.0 arithmetic.0 backgammon.0 banner.0 \
	bcd.0 bj.0 boggle.0 canfield.0 chess.0 ching.0 \
	cribbage.0 doctor.0 fish.0 fortune.0 hangman.0 hunt.0 mille.0 \
	monop.0 moo.0 number.0 quiz.0 rain.0 robots.0 rogue.0 \
	snake.0 trek.0 ttt.0 warp.0 words.0 worm.0 worms.0 wump.0 zork.0

.SUFFIXES: .6 .0

.6.0:
	${DESTDIR}/usr/man/manroff $*.6 > $*.0

all: _make_01

_make_01: ${OBJS}

clean: FRC
	rm -f ${OBJS}

install: _make_01
	install -c -o bin -g bin -m 444 ${OBJS} ${DESTDIR}${MDIR}
	rm -f ${DESTDIR}${MDIR}/snscore.0
	ln ${DESTDIR}${MDIR}/snake.0 ${DESTDIR}${MDIR}/snscore.0
	rm -f ${DESTDIR}${MDIR}/cfscores.0
	ln ${DESTDIR}${MDIR}/canfield.0 ${DESTDIR}${MDIR}/cfscores.0
FRC:

