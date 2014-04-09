TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

OBJS            = cpp.o lex.o nlist.o tokens.o macro.o eval.o \
                  include.o hideset.o unix.o

LDFLAGS         += -g

CFLAGS		+= -Werror -Wall -Os

all:            lcpp

lcpp:           $(OBJS)
		${CC} ${LDFLAGS} -o lcpp.elf $(OBJS) ${LIBS}
		${OBJDUMP} -S lcpp.elf > lcpp.dis
		${SIZE} lcpp.elf
		${ELF2AOUT} lcpp.elf $@ && rm lcpp.elf

clean:
		rm -rf *.o *.elf lcpp *.dis *~ tests

install:        all
		install lcpp $(DESTDIR)/bin/

$(OBJS):	cpp.h

test:
		mkdir -p tests
		./lcpp < ../cpp/tests/test1 > tests/run1
		-diff ../cpp/tests/res1 tests/run1
		./lcpp < ../cpp/tests/test2 > tests/run2
		-diff ../cpp/tests/res2 tests/run2
		./lcpp < ../cpp/tests/test3 > tests/run3
		-diff ../cpp/tests/res3 tests/run3
		./lcpp < ../cpp/tests/test4 > tests/run4
		-diff ../cpp/tests/res4 tests/run4
		-./lcpp < ../cpp/tests/test5 > tests/run5
		-diff ../cpp/tests/res5 tests/run5
		./lcpp < ../cpp/tests/test6 > tests/run6
		-diff ../cpp/tests/res6 tests/run6
		./lcpp < ../cpp/tests/test7 > tests/run7
		-diff ../cpp/tests/res7 tests/run7
		./lcpp < ../cpp/tests/test8 > tests/run8
		-diff ../cpp/tests/res8 tests/run8
		./lcpp < ../cpp/tests/test9 > tests/run9
		-diff ../cpp/tests/res9 tests/run9
		./lcpp < ../cpp/tests/test10 > tests/run10
		-diff ../cpp/tests/res10 tests/run10
