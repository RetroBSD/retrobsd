TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = uuencode.c
OBJS            = uuencode.o
MAN             = uuencode.0
MANSRC          = uuencode.1

all:            uuencode $(MAN)

uuencode:          ${OBJS}
		${CC} ${LDFLAGS} -o uuencode.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S uuencode.elf > uuencode.dis
		${SIZE} uuencode.elf
		${ELF2AOUT} uuencode.elf $@ && rm uuencode.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o uuencode uuencode.elf uuencode.dis tags *~ uuencode.0

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install uuencode ${DESTDIR}/bin/uuencode
		cp ${MAN} ${DESTDIR}/share/man/cat1/

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
