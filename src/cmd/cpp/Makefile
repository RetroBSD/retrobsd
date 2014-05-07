TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk
#CFLAGS          = -DCROSS

OBJS            = cpp.o cpy.o token.o
MAN             = cpp.0
MANSRC          = cpp.1

LDFLAGS         += -g

CFLAGS		+= -Werror -Wall -Os
CFLAGS          += -DCPP_DEBUG -DGCC_COMPAT -DHAVE_CPP_VARARG_MACRO_GCC

all:            cpp $(MAN)

cpp:            $(OBJS)
		${CC} ${LDFLAGS} -o cpp.elf $(OBJS) ${LIBS}
		${OBJDUMP} -S cpp.elf > cpp.dis
		${SIZE} cpp.elf
		${ELF2AOUT} cpp.elf $@ && rm cpp.elf

$(MAN):         ${MANSRC}
		${MANROFF} $< > $@

clean:
		rm -f *.o *.0 *.elf cpp *.dis tags *~ lex.yy.c y.tab.[ch] tests/run*

install:        all
		install cpp $(DESTDIR)/bin/
		cp cpp.0 $(DESTDIR)/share/man/cat1/

cpp.o:          cpp.c y.tab.h

.l.o:
		$(LEX) $(LFLAGS) $<
		$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o $@ lex.yy.c

cpy.o y.tab.h:  cpy.y
		$(YACC) -d $(YFLAGS) cpy.y
		$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o cpy.o y.tab.c

test:
		./cpp < tests/test1 > tests/run1
		cmp tests/run1 tests/res1
		./cpp < tests/test2 > tests/run2
		cmp tests/run2 tests/res2
		./cpp < tests/test3 > tests/run3
		cmp tests/run3 tests/res3
		./cpp < tests/test4 > tests/run4
		cmp tests/run4 tests/res4
		./cpp < tests/test5 > tests/run5
		cmp tests/run5 tests/res5
		./cpp < tests/test6 > tests/run6
		cmp tests/run6 tests/res6
		./cpp < tests/test7 > tests/run7
		cmp tests/run7 tests/res7
		./cpp < tests/test8 > tests/run8
		cmp tests/run8 tests/res8
		./cpp < tests/test9 > tests/run9
		cmp tests/run9 tests/res9
		./cpp < tests/test10 > tests/run10
		cmp tests/run10 tests/res10
