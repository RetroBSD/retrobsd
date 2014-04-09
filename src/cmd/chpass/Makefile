#
# Copyright (c) 1988 The Regents of the University of California.
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
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= chpass.c field.c util.c
OBJS		= chpass.o field.o util.o
MAN		= chpass.0
MANSRC		= chpass.1

all:            chpass chpass.0

chpass:         ${OBJS}
		${CC} ${LDFLAGS} -o chpass.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S chpass.elf > chpass.dis
		${SIZE} chpass.elf
		${ELF2AOUT} chpass.elf $@ && rm chpass.elf

chpass.0:       ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} chpass *.elf *.dis tags *~

cleandir: 	clean
		rm -f tags .depend

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install: 	all
		install -m 4755 chpass ${DESTDIR}/bin/chpass
		rm -f ${DESTDIR}/bin/chfn; ln ${DESTDIR}/bin/chpass ${DESTDIR}/bin/chfn
		rm -f ${DESTDIR}/bin/chsh; ln ${DESTDIR}/bin/chpass ${DESTDIR}/bin/chsh
		cp ${MAN} ${DESTDIR}/share/man/cat1/
		rm -f ${DESTDIR}/share/man/cat1/chfn.0
		rm -f ${DESTDIR}/share/man/cat1/chsh.0
		-ln ${DESTDIR}/share/man/cat1/chpass.0 ${DESTDIR}/share/man/cat1/chfn.0
		-ln ${DESTDIR}/share/man/cat1/chpass.0 ${DESTDIR}/share/man/cat1/chsh.0

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
