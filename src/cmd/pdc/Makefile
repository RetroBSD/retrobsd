TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS          = -DCROSS

OBJS            = pdc.o

LDFLAGS         += -g

CFLAGS		+= -Werror -Wall -Os
CFLAGS          += -DGCC_COMPAT -DHAVE_CPP_VARARG_MACRO_GCC -DHAVE_READLINE
LIBS            = -lreadline -lc

all:            pdc

pdc:            $(OBJS)
		${CC} ${LDFLAGS} -o pdc.elf $(OBJS) ${LIBS}
		${OBJDUMP} -S pdc.elf > pdc.dis
		${SIZE} pdc.elf
		${ELF2AOUT} pdc.elf $@ && rm pdc.elf

clean:
		rm -f *.o *.0 *.elf pdc *.dis tags *~ y.tab.[ch]

install:        all
		install pdc $(DESTDIR)/bin/

pdc.o:          pdc.y
		$(YACC) -d $(YFLAGS) pdc.y
		$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o pdc.o y.tab.c
