TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = calendar.c
OBJS            = calendar.o
MAN             = calendar.0
MANSRC          = calendar.1

all:            calendar $(MAN)

calendar:          ${OBJS}
		${CC} ${LDFLAGS} -o calendar.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S calendar.elf > calendar.dis
		${SIZE} calendar.elf
		${ELF2AOUT} calendar.elf $@ && rm calendar.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o calendar calendar.elf calendar.dis tags *~ calendar.0

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install calendar ${DESTDIR}/bin/calendar
		cp ${MAN} ${DESTDIR}/share/man/cat1/
		@mkdir -p ${DESTDIR}/share/calendar
		install calendars/calendar.* ${DESTDIR}/share/calendar/

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
