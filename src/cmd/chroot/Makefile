#
# Copyright (c) 1989 The Regents of the University of California.
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
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

SRCS            = chroot.c
OBJS            = chroot.o
MAN             = chroot.0
MANSRC          = chroot.8

all:            chroot ${MAN}

chroot:         ${OBJS}
		${CC} ${LDFLAGS} -o chroot.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S chroot.elf > chroot.dis
		${SIZE} chroot.elf
		${ELF2AOUT} chroot.elf $@ && rm chroot.elf

chroot.0:       ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f ${OBJS} ${MAN} core chroot chroot.elf chroot.dis *~

cleandir:       clean
		rm -f tags .depend

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 4751 chroot ${DESTDIR}/sbin
		cp ${MAN} ${DESTDIR}/share/man/cat8/

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
