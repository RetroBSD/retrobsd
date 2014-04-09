#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Wall -Werror -I../../../../sys/include

BIN		= smux
SRCS            = smux.c
OBJS            = smux.o

all: smux

smux: $(OBJS)
			${CC} ${LDFLAGS} -o $@.elf ${OBJS} ${LIBS}
			${OBJDUMP} -S $@.elf > $@.dis
			${SIZE} $@.elf
			${ELF2AOUT} $@.elf $@

clean:
		rm -f *.o *.elf *.elf *.dis tags *~ $(BIN)

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 755 $(BIN) ${DESTDIR}/bin

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
