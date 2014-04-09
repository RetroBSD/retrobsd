TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= sed0.c sed1.c
OBJS		= sed0.o sed1.o

all:            sed

sed:            ${OBJS}
		${CC} ${LDFLAGS} -o sed.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S sed.elf > sed.dis
		${SIZE} sed.elf
		${ELF2AOUT} sed.elf $@ && rm sed.elf

clean:
		rm -f *.o *.elf ${MAN} sed *.elf *.dis tags *~

install:        all
		install sed $(DESTDIR)/bin/

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h
