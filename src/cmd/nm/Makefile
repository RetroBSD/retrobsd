TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -I../ar

SRCS		= nm.c
OBJS		= nm.o

all:            nm

nm:          	${OBJS}
		${CC} ${LDFLAGS} -o nm.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S nm.elf > nm.dis
		${SIZE} nm.elf
		${ELF2AOUT} nm.elf $@ && rm nm.elf

clean:
		rm -f *.o *.elf ${MAN} nm *.elf *.dis tags *~

install:        all
		install nm ${DESTDIR}/bin/nm

lint:
		lint -havx -I../ar ${SRCS}
