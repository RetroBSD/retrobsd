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

SRCS		= mkpasswd.c
OBJS		= mkpasswd.o
MAN		= mkpasswd.0
MANSRC		= mkpasswd.8

all:            mkpasswd ${MAN}

mkpasswd:    	${OBJS}
		${CC} ${LDFLAGS} -o mkpasswd.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S mkpasswd.elf > mkpasswd.dis
		${SIZE} mkpasswd.elf
		${ELF2AOUT} mkpasswd.elf $@ && rm mkpasswd.elf

${MAN}:         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} mkpasswd *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep -p ${CFLAGS} ${SRCS}

install:        all
		install mkpasswd ${DESTDIR}/sbin/mkpasswd
		cp ${MAN} ${DESTDIR}/share/man/cat8/

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
