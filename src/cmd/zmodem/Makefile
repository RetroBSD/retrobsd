#
# Driver makefile for zmodem.  Calls makefile.generic.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -DV7 -DMD=2 -DTXBSIZE=8192 -DNFGVMIN -DSEGMENTS=32

SRCS		= rz.c sz.c
OBJS		= rz.o sz.o
MAN		= rz.0 sz.0
MANSRC		= rz.1 sz.1

all:            rz sz $(MAN)

rz:             rz.o
		$(CC) $(LDFLAGS) -o rz.elf $< $(LIBS)
		$(OBJDUMP) -S rz.elf > rz.dis
		$(SIZE) rz.elf
		$(ELF2AOUT) rz.elf $@ && rm rz.elf

sz:             sz.o
		$(CC) $(LDFLAGS) -o sz.elf $< $(LIBS)
		$(OBJDUMP) -S sz.elf > sz.dis
		$(SIZE) sz.elf
		$(ELF2AOUT) sz.elf $@ && rm sz.elf

rz.0:           rz.1
		$(MANROFF) $< > $@

sz.0:           sz.1
		$(MANROFF) $< > $@

clean:
		rm -f *.o *.elf $(MAN) rz sz *.elf *.dis tags *~

install:        all
		install rz $(DESTDIR)/bin/
		install sz $(DESTDIR)/bin/
		cp rz.0 $(DESTDIR)/share/man/cat1/
		cp sz.0 $(DESTDIR)/share/man/cat1/
		@rm -f $(DESTDIR)/bin/sb $(DESTDIR)/bin/sx
		ln $(DESTDIR)/bin/sz $(DESTDIR)/bin/sb
		ln $(DESTDIR)/bin/sz $(DESTDIR)/bin/sx
		@rm -f $(DESTDIR)/bin/rb $(DESTDIR)/bin/rx
		ln $(DESTDIR)/bin/rz $(DESTDIR)/bin/rb
		ln $(DESTDIR)/bin/rz $(DESTDIR)/bin/rx
		@rm -f $(DESTDIR)/share/man/cat1/sb.0 $(DESTDIR)/share/man/cat1/sx.0
		ln $(DESTDIR)/share/man/cat1/sz.0 $(DESTDIR)/share/man/cat1/sb.0
		ln $(DESTDIR)/share/man/cat1/sz.0 $(DESTDIR)/share/man/cat1/sx.0
		@rm -f $(DESTDIR)/share/man/cat1/rb.0 $(DESTDIR)/share/man/cat1/rx.0
		ln $(DESTDIR)/share/man/cat1/rz.0 $(DESTDIR)/share/man/cat1/rb.0
		ln $(DESTDIR)/share/man/cat1/rz.0 $(DESTDIR)/share/man/cat1/rx.0
