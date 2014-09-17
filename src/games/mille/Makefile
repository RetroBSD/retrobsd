#
# mille bourne game makefile
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -DCROSS -Werror -Wall

CFLAGS  	+= -Werror -Wall

OBJS            = comp.o end.o extern.o init.o mille.o misc.o move.o print.o \
                  roll.o save.o types.o varpush.o
MAN             = mille.0
LIBS            = -lcurses -ltermcap -lc

all:            mille $(MAN)

mille:          $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         mille.6
		nroff -man $< > $@

clean:
		rm -f *.o core mille *.0 *.dis

install:        all
		install mille $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
