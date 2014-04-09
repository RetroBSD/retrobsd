#
# Public Domain.  2/4/1995 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= uname.c
OBJS		= uname.o
MAN		= uname.0
MANSRC		= uname.1

all:            uname uname.0

uname:          ${OBJS}
		${CC} ${LDFLAGS} -o uname.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S uname.elf > uname.dis
		${SIZE} uname.elf
		${ELF2AOUT} uname.elf $@ && rm uname.elf

uname.0:        ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} uname *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		cp ${MAN} ${DESTDIR}/share/man/cat1/
		install uname ${DESTDIR}/bin/uname

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
