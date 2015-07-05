TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          = -O
SRCS            = uudecode.c
OBJS            = uudecode.o

all:            uudecode

uudecode:          ${OBJS}
		${CC} ${LDFLAGS} -o uudecode.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S uudecode.elf > uudecode.dis
		${SIZE} uudecode.elf
		${ELF2AOUT} uudecode.elf $@ && rm uudecode.elf

clean:
		rm -f *.o uudecode uudecode.elf uudecode.dis tags *~ uudecode.0

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install uudecode ${DESTDIR}/bin/uudecode

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
