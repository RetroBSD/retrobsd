TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall
LIBS            += -lm -lc

OBJS            = forth.o func.o

all:            forth

forth:          ${OBJS}
		${CC} ${LDFLAGS} -o forth.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S forth.elf > forth.dis
		${SIZE} forth.elf
		${ELF2AOUT} forth.elf $@ && rm forth.elf

clean:
		rm -f *.o *.elf forth *.dis *~

func.txt:	func.c
		grep '^ \*+' func.c | expand | sed 's/^ \*+ *$$//' |\
		sed 's/^ \*+ //' > func.txt

install:        all
		install forth $(DESTDIR)/bin/

###
forth.o: forth.c io.h forth.h
func.o: func.c io.h forth.h
