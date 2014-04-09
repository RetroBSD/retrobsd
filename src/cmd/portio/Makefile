TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -Wall

SRCS		= portio.c
OBJS		= portio.o

all:            portio lol

portio:         ${OBJS}
		${CC} ${LDFLAGS} -o portio.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S portio.elf > portio.dis
		${SIZE} portio.elf
		${ELF2AOUT} portio.elf $@ && rm portio.elf

lol:            lol.o
		${CC} ${LDFLAGS} -o lol.elf lol.o ${LIBS}
		${OBJDUMP} -S lol.elf > lol.dis
		${SIZE} lol.elf
		${ELF2AOUT} lol.elf $@ && rm lol.elf

clean:
		rm -f *.o *.elf portio lol *.elf *.dis *~

install:        all
		install portio $(DESTDIR)/bin/
		install lol $(DESTDIR)/bin/
