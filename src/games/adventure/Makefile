TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Os -Wall -Werror

OBJS            = done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
MAN		= adventure.0
MANSRC		= adventure.6

all:            adventure $(MAN) adventure.dat

# Data file format is compatible for all little-endian machines.
# To rebuild it, compile the program on Linux.
adventure.dat:  #glorkz
		./adventure glorkz

adventure:      ${OBJS}
		${CC} ${LDFLAGS} -o adventure.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S adventure.elf > adventure.dis
		${SIZE} adventure.elf
		${ELF2AOUT} adventure.elf $@ && rm adventure.elf

${MAN}:         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf ${MAN} adventure *.elf *.dis tags *~

install:        all
		install adventure $(DESTDIR)/games/
		cp adventure.dat $(DESTDIR)/games/lib/
		cp ${MAN} $(DESTDIR)/share/man/cat6/

$(OBJS):        hdr.h
