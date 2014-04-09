#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= renice.c
OBJS		= renice.o
MAN		= renice.0
MANSRC		= renice.8

all:            renice ${MAN}

renice:         ${OBJS}
		${CC} ${LDFLAGS} -o renice.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S renice.elf > renice.dis
		${SIZE} renice.elf
		${ELF2AOUT} renice.elf $@ && rm renice.elf

${MAN}:         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} renice *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        renice
		cp ${MAN} ${DESTDIR}/share/man/cat8/
		install renice ${DESTDIR}/bin/renice

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
