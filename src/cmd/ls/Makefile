#
# Public Domain.  12/3/1994 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		= -O
SRCS		= ls.c stat_flags.c
OBJS		= ls.o stat_flags.o

all:		ls

ls:		${OBJS}
		${CC} ${LDFLAGS} -o ls.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S ls.elf > ls.dis
		${SIZE} ls.elf
		${ELF2AOUT} ls.elf $@ && rm ls.elf

clean:
		rm -f *.o *.elf *.dis ls tags *~

depend:		${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:	all
		install ls ${DESTDIR}/bin/ls

lint:		${SRCS}
		lint -hax ${SRCS}

tags:		${SRCS}
		ctags ${SRCS}
