TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libtermlib

CFLAGS		+= -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS		= termcap.o tgoto.o tputs.o tcattr.o

all:            ../libtermlib.a

../libtermlib.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
