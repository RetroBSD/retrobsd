#	@(#)Makefile	4.2	(Berkeley)	9/7/85
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -DCROSS -Werror -Wall

#CFLAGS  	+= -Werror -Wall

LIBS		= -lm -lc

OBJS            = caesar.o
MAN             = caesar.0

all:            caesar $(MAN)

caesar:           $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         caesar.6
		nroff -man $< > $@

clean:
		rm -f *.o core caesar *.0 *.dis

install:        all
		install caesar $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
