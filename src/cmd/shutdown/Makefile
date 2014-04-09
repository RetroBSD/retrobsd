#
# Public Domain.  1996/11/16 - Steven Schultz
# Should be installed with group operator.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= shutdown.c
OBJS		= shutdown.o
MAN		= shutdown.0
MANSRC		= shutdown.8

all:            shutdown ${MAN}

shutdown:       ${OBJS}
		${CC} ${LDFLAGS} -o shutdown.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S shutdown.elf > shutdown.dis
		${SIZE} shutdown.elf
		${ELF2AOUT} shutdown.elf $@ && rm shutdown.elf

${MAN}:         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} shutdown *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		cp ${MAN} ${DESTDIR}/share/man/cat8/
		install -m 4750 shutdown ${DESTDIR}/sbin/shutdown

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
