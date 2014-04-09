#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= mknod.c
OBJS		= mknod.o
MAN		= mknod.0
MANSRC		= mknod.8

all:            mknod mknod.0

mknod:       	${OBJS}
		${CC} ${LDFLAGS} -o mknod.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S mknod.elf > mknod.dis
		${SIZE} mknod.elf
		${ELF2AOUT} mknod.elf $@ && rm mknod.elf

mknod.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} mknod *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		cp ${MAN} ${DESTDIR}/share/man/cat8/
		install mknod ${DESTDIR}/sbin/mknod

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
