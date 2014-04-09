#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (Berkeley) 9/7/85
#
DESTDIR=
CC = /bin/cc
SEPFLAG= -i
CFLAGS = -O

all:	boggle bogdict

bogdict: comp sfile
	-if [ -f /usr/local/lib/w2a ]; \
		then dict=/usr/local/lib/w2a; \
		else dict=/usr/dict/words; fi; \
	sed -f sfile $$dict | uniq | ./comp >bogdict

install: all
	install -s boggle ${DESTDIR}/usr/games/boggle
	install -m 644 bogdict ${DESTDIR}/usr/games/lib/bogdict

boggle: boggle.c
	cc ${SEPFLAG} -o boggle ${CFLAGS} boggle.c

comp: comp.c
	cc ${SEPFLAG} -o comp ${CFLAGS} comp.c

clean:
	rm -f bogdict boggle comp *.o
