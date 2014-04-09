TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS		= -DCROSS -I$(TOPSRC)/include -I/usr/include -D__mips__ -ggdb3

OBJS            = message.o access.o command.o expr.o findfn.o format.o input.o \
                  main.o opset.o output.o pcs.o print.o \
                  runpcs.o setup.o sym.o

CFLAGS		+= -DNUM_SYMS_CACHE=50 -Wall -Werror

all:            adb

adb:            $(OBJS)
		${CC} ${LDFLAGS} -o adb.elf $(OBJS) ${LIBS}
		${OBJDUMP} -S adb.elf > adb.dis
		${SIZE} adb.elf
		${ELF2AOUT} adb.elf $@ && rm adb.elf

clean:
		rm -f *.o *.0 *.elf adb *.elf *.dis tags *~

install:        all
		install adb $(DESTDIR)/bin/

$(OBJS): defs.h
