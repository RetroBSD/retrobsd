TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -Wall -Werror

SRCS		= init.c
OBJS		= init.o
MAN		= init.0
MANSRC		= init.8

all:		init init.0

init:		${OBJS}
		${CC} ${LDFLAGS} -o init.elf ${OBJS} -lc
		${OBJDUMP} -S init.elf > init.dis
		${SIZE} init.elf
		${ELF2AOUT} init.elf $@ && rm init.elf

init.0:		${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *~ *.elf *.dis ${MAN} init tags

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	all
		cp ${MAN} ${DESTDIR}/share/man/cat8/
		install -m 700 init ${DESTDIR}/sbin/init

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
