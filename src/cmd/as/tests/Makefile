TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Wall -Os
TESTS           = test1 test2 test3 test4

all:            ../as ../aout
		for i in $(TESTS); do \
                    $(AS) $$i.s -o $$i.o; \
                    $(OBJDUMP) -D $$i.o > $$i.gcc-dis; \
                    ../as $$i.s -o $$i.o; \
                    ../aout -r $$i.o > $$i.dis; \
                done

clean:
		rm -f *.o *.dis *.gcc-dis

hello.s:        hello.c
		$(CC) $(CFLAGS) -S $<
