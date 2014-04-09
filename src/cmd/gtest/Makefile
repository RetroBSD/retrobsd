#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -I../../../sys/include

SRCS            = globread.c globwrite.c
OBJS            = globread.o globwrite.o

all:            globread globwrite globdump

globread:	globread.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

globdump:	globdump.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

globwrite:	globwrite.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@


clean:
		rm -f *.o *.elf ${MAN} *.elf *.dis tags *~  globread globwrite globdump

depend:         ${SRCS}
		mkdep ${CFLAGS} ${SRCS}

install:        all
		install -m 755 globread ${DESTDIR}/bin
		install -m 755 globdump ${DESTDIR}/bin
		install -m 755 globwrite ${DESTDIR}/bin

lint:           ${SRCS}
		lint -hax ${SRCS}

tags:           ${SRCS}
		ctags ${SRCS}
