TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

SRCS            = dc.c dc.h
OBJS            = dc.o

all:            dc

dc:             ${OBJS}
		${CC} ${LDFLAGS} -o dc.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S dc.elf > dc.dis
		${SIZE} dc.elf
		${ELF2AOUT} dc.elf $@ && rm dc.elf

install:        dc
		install dc ${DESTDIR}/bin/dc

clean:
		rm -f *.o *.elf ${MAN} dc *.elf *.dis tags *~
