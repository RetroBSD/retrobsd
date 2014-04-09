TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = chown.c
OBJS            = chown.o
MAN             = chown.0
MANSRC          = chown.8

all:            chown $(MAN)

chown:          ${OBJS}
		${CC} ${LDFLAGS} -o chown.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S chown.elf > chown.dis
		${SIZE} chown.elf
		${ELF2AOUT} chown.elf $@ && rm chown.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o chown chown.elf chown.dis tags *~ chown.0

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install chown ${DESTDIR}/sbin/chown
		cp ${MAN} ${DESTDIR}/share/man/cat8/

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
