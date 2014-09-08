#
# as      - Assembler
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -Os
LDFLAGS         +=

all:            as

as:             as.o
		${CC} ${LDFLAGS} -o as.elf as.o ${LIBS}
		${OBJDUMP} -S as.elf > as.dis
		${SIZE} as.elf
		${ELF2AOUT} as.elf $@ && rm as.elf

clean:
		rm -f *.o *.0 *.elf as tags *~ *.dis tests/*.dis tests/*.gcc-dis tests/*.o

install:        all
		install as $(DESTDIR)/bin/
