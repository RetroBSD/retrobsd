#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -I../../../sys/include

BIN		= date
SRCS            = $(BIN).c
OBJS            = $(BIN).o

all:            $(BIN)

$(BIN):    ${OBJS}
		${LD} ${LDFLAGS} -o $@.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

.SUFFIXES:      .0 .1

clean:
		rm -f *.o *.elf ${MAN} *.elf *.dis tags *~ $(BIN)

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 755 $(BIN) ${DESTDIR}/bin

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
