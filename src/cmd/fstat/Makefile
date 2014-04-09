#
# Public Domain.  1996/11/16 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= fstat.c
OBJS		= fstat.o
MAN		= fstat.0
MANSRC		= fstat.8

all:            fstat fstat.0

fstat:          ${OBJS}
		${CC} ${LDFLAGS} -o fstat.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S fstat.elf > fstat.dis
		${SIZE} fstat.elf
		${ELF2AOUT} fstat.elf $@ && rm fstat.elf

fstat.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} fstat *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		cp ${MAN} ${DESTDIR}/share/man/cat8
		install -m 2755 fstat ${DESTDIR}/bin/fstat

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
