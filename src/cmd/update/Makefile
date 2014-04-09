#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		= -O
SRCS		= update.c
OBJS		= update.o
MAN		= update.0
MANSRC		= update.8

all:		update ${MAN}

update:		${OBJS}
		${CC} ${LDFLAGS} -o update.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S update.elf > update.dis
		${SIZE} update.elf
		${ELF2AOUT} update.elf $@ && rm update.elf

${MAN}:		${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *~ ${MAN} update update.elf update.dis tags

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	all
		cp ${MAN} ${DESTDIR}/share/man/cat8/
		install update ${DESTDIR}/sbin/update

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
