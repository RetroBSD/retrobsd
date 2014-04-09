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
LIBS            := -lutil $(LIBS)

SRCS		= login.c
OBJS		= login.o

all:            login

login:       	${OBJS}
		${CC} ${LDFLAGS} -o login.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S login.elf > login.dis
		${SIZE} login.elf
		${ELF2AOUT} login.elf $@ && rm login.elf

clean:
		rm -f *.o *.elf ${MAN} login *.elf *.dis tags *~


cleandir:       clean
		rm -f ${MAN} tags .depend

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 4755 login ${DESTDIR}/bin/login

lint:           ${SRCS}
		lint ${CFLAGS} ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
