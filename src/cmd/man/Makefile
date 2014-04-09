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
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= man.c
OBJS		= man.o

all:            man apropos

man:            ${OBJS}
		${CC} ${LDFLAGS} -o man.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S man.elf > man.dis
		${SIZE} man.elf
		${ELF2AOUT} man.elf $@ && rm man.elf

apropos:        ${OBJS}
		${CC} ${LDFLAGS} -o apropos.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S apropos.elf > apropos.dis
		${SIZE} apropos.elf
		${ELF2AOUT} apropos.elf $@ && rm apropos.elf

clean:
		rm -f *.o *.elf ${MAN} man apropos *.elf *.dis tags *~

depend:         FRC
		mkdep ${CFLAGS} ${ASRCS} ${MSRCS}

install:        all
		@rm -f ${DESTDIR}/bin/whatis ${DESTDIR}/bin/apropos
		install apropos ${DESTDIR}/bin/apropos
		install man ${DESTDIR}/bin/man
		ln ${DESTDIR}/bin/apropos ${DESTDIR}/bin/whatis

lint:           FRC
		lint ${CFLAGS} ${ASRCS}
		lint ${CFLAGS} ${MSRCS}

tags:           FRC
		ctags ${ASRCS}
		ctags ${MSRCS}
		sort -o tags tags

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
