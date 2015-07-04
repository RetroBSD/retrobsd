#	@(#)Makefile	4.2	(Berkeley)	9/7/85
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS  	= -O -DCROSS -Werror -Wall

#CFLAGS  	+= -Werror -Wall

LIBS		= -lm -lc

OBJS            = pom.o
MAN             = pom.0

all:            pom $(MAN)

pom:           $(OBJS)
		$(CC) $(LDFLAGS) -o $@.elf $(OBJS) $(LIBS)
		$(OBJDUMP) -S $@.elf > $@.dis
		$(SIZE) $@.elf
		$(ELF2AOUT) $@.elf $@ && rm $@.elf

$(MAN):         pom.6
		nroff -man $< > $@

clean:
		rm -f *.o core pom *.0 *.dis

install:        all
		install pom $(DESTDIR)/games/
		install -m 644 $(MAN) $(DESTDIR)/share/man/cat6/
