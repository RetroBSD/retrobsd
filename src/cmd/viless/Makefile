TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Wall -Werror -DMULTIWIN=0
#CFLAGS		+= -DDEBUG

OBJS            = vi.o

all:            vi

vi:          	${OBJS}
		${LD} ${LDFLAGS} -o vi.elf ${OBJS} ${LIBS}
#		${OBJDUMP} -S vi.elf > vi.dis
		${SIZE} vi.elf
		${ELF2AOUT} vi.elf $@

${OBJ}:         r.defs.h

clean:
		rm -f *.o *.elf vi *.elf *.dis *~

install:        all
		install vi $(DESTDIR)/bin/vi
