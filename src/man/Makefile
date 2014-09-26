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
#	@(#)Makefile	5.3 (Berkeley) 7/9/88
#
DIRS=	man1 man2 man3 man3f man4 man5 man6 \
	man7 man8
CDIRS=	cat1 cat2 cat3 cat3f cat4 cat5 cat6 \
	cat7 cat8
LCDIRS=	cat1 cat2 cat3 cat4 cat5 cat6 cat7 cat8

all: scriptinstall FRC
	for i in ${DIRS}; do \
		(cd $$i; echo "Starting $$i"; make ${MFLAGS}); \
	done

scriptinstall: FRC
	install -c -o bin -g bin -m 444 makewhatis.sed ${DESTDIR}/usr/man/makewhatis.sed
	install -c -o bin -g bin -m 755 manroff ${DESTDIR}/usr/man/manroff

install: FRC
	for i in ${DIRS}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
	done
	cp /dev/null whatis_tmp
	for dirext in /usr/man/cat* ; do \
	echo $$dirext; \
	for file in `find $$dirext -type f -name '*.0' -print`; do \
		sed -n -f /usr/man/makewhatis.sed $$file; \
	done >> whatis_tmp; \
	done
	sort -u whatis_tmp > whatis.db
	-rm -f whatis_tmp
	install -o bin -g bin -m 444 whatis.db ${DESTDIR}/usr/man/whatis
	install -c -o bin -g bin -m 444 man.template ${DESTDIR}/usr/man/man.template

clean: FRC
	for i in ${DIRS}; do \
		(cd $$i; make ${MFLAGS} clean); \
	done
	rm -f whatis_tmp whatis.db

depend:
lint:
tags:
FRC:
