#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= vipw.c
OBJS		= vipw.o
MAN		= vipw.0
MANSRC		= vipw.8

all:            vipw ${MAN}

vipw:        	${OBJS}
		${CC} ${LDFLAGS} -o vipw.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S vipw.elf > vipw.dis
		${SIZE} vipw.elf
		${ELF2AOUT} vipw.elf $@ && rm vipw.elf

${MAN}:         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} vipw *.elf *.dis tags *~

cleandir:       clean
		rm -f tags .depend

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install vipw ${DESTDIR}/sbin/
		cp ${MAN} ${DESTDIR}/share/man/cat8/

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
