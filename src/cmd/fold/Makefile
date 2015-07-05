TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = fold.c
OBJS            = fold.o
MAN             = fold.0
MANSRC          = fold.1

all:            fold $(MAN)

fold:          ${OBJS}
		${CC} ${LDFLAGS} -o fold.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S fold.elf > fold.dis
		${SIZE} fold.elf
		${ELF2AOUT} fold.elf $@ && rm fold.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o fold fold.elf fold.dis tags *~ fold.0

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install fold ${DESTDIR}/bin/fold
		cp ${MAN} ${DESTDIR}/share/man/cat1/

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
