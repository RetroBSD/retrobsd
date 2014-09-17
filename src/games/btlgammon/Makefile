#  @(#)Makefile    4.2     (Berkeley)      9/7/85
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS  	= -O -Wall -Werror
OBJS    	= btlgammon.o

all:            btlgammon

btlgammon:      ${OBJS}
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

clean:
		rm -f *.o core btlgammon

install:
		install btlgammon $(DESTDIR)/games/
		install -m 644 backrules $(DESTDIR)/games/lib/backrules
