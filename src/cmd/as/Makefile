#
# as      - Assembler
# aout    - Display information from a.out files
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -Os
LDFLAGS         +=

AOUTOBJS        = aout.o mips-dis.o

all:            as aout

as:             as.o
		${CC} ${LDFLAGS} -o as.elf as.o ${LIBS}
		${OBJDUMP} -S as.elf > as.dis
		${SIZE} as.elf
		${ELF2AOUT} as.elf $@ && rm as.elf

aout:           $(AOUTOBJS)
		${CC} ${LDFLAGS} -o aout.elf $(AOUTOBJS) ${LIBS}
		${OBJDUMP} -S aout.elf > aout.dis
		${SIZE} aout.elf
		${ELF2AOUT} aout.elf $@ && rm aout.elf

clean:
		rm -f *.o *.0 *.elf as aout tags *~ *.dis tests/*.dis tests/*.gcc-dis tests/*.o

test:
		/usr/local/pic32-tools/bin/pic32-as -al example.s

install:        all
		install as $(DESTDIR)/bin/
		install aout $(DESTDIR)/bin/

test.dis-gcc:   test.s
		$(AS) $< -o test.o
		${OBJDUMP} -D test.o > $@

test.dis:       test.s as aout
		./as $< -o test.o
		./aout test.o > $@
