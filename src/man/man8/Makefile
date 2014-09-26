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
#	@(#)Makefile	5.4.7 (2.11BSD) 1997/1/10
#
MDIR=	/usr/man/cat8
SRCS=	XNSrouted.8 adduser.8 autoconfig.8 \
	bugfiler.8 crash.8 \
	boot.8 \
	drtest.8 dump.8 dumpdir.8 \
	format.8 fsck.8 \
	getty.8 htable.8 \
	intro.8 lpc.8 lpd.8 makedev.8 \
	makekey.8 \
	named.8 pac.8 \
	rc.8 rdump.8 \
	routed.8 rrestore.8 \
	sendmail.8 \
	sticky.8 sync.8 \
	timed.8 timedc.8 uucico.8 \
	uuclean.8 uupoll.8 uusnap.8 uuxqt.8
OBJS=	XNSrouted.0 adduser.0 autoconfig.0 \
	bugfiler.0 crash.0 \
	boot.0 \
	drtest.0 dump.0 dumpdir.0 \
	format.0 fsck.0 \
	getty.0 htable.0 \
	intro.0 lpc.0 lpd.0 makedev.0 \
	makekey.0 \
	named.0 pac.0 \
	rc.0 rdump.0 \
	routed.0 rrestore.0 \
	sendmail.0 \
	sticky.0 sync.0 \
	timed.0 timedc.0 uucico.0 \
	uuclean.0 uupoll.0 uusnap.0 uuxqt.0

.SUFFIXES: .8 .0

.8.0:
	${DESTDIR}/usr/man/manroff $*.8 > $*.0

all: _make_01

_make_01: ${OBJS}

clean: FRC
	rm -f ${OBJS}

install: _make_01
	install -c -o bin -g bin -m 444 ${OBJS} ${DESTDIR}${MDIR}
	rm -f ${DESTDIR}${MDIR}/uucpd.0
	ln ${DESTDIR}${MDIR}/uucico.0 ${DESTDIR}${MDIR}/uucpd.0

FRC:
