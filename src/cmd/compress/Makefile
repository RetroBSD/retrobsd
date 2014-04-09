#
#	@(#)Makefile	5.5 (Berkeley) 9/18/85
#
TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

OBJS 		= compress.o

#COMFLAGS       = -DBSD4_2 -O -DSACREDMEM=256000

CFLAGS          += -DUSERMEM=1024 -Wall -Werror

compress: $(OBJS)
	${CC} ${LDFLAGS} -o $@.elf ${OBJS} ${LIBS}
	${OBJDUMP} -S $@.elf > $@.dis
	${SIZE} $@.elf
	${ELF2AOUT} $@.elf $@

# USERMEM may have to be set by hand.  It should contain the amount of
# available user memory in bytes.  Set it to zero, for physical memory
# less than 1 Meg.
USERMEM:
	sh usermem.sh > USERMEM

install: compress
	cp compress $(DESTDIR)/bin/compress
	cp compress $(DESTDIR)/bin/uncompress
	cp compress $(DESTDIR)/bin/zcat

# Temporarily don't delete USERMEM.  When chroot'ed to /nbsd, usermem.sh
# fails totally.
clean:
	rm -f compress core errs compress.o *.elf *.dis
#	rm -f compress USERMEM core errs
