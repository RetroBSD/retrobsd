#
# Public Domain.  1995/05/02 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -DNOFPU

SRCS		= printf.c
OBJS		= printf.o
MAN		= printf.0
MANSRC		= printf.1

all:            printf ${MAN}

printf:         ${OBJS}
		${CC} ${LDFLAGS} -o printf.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S printf.elf > printf.dis
		${SIZE} printf.elf
		${ELF2AOUT} printf.elf $@ && rm printf.elf

${MAN}:         ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} printf *.elf *.dis tags *~

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		cp ${MAN} ${DESTDIR}/share/man/cat1/
		install printf ${DESTDIR}/bin/printf

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
