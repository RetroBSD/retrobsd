TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libreadline

CFLAGS		= -Os -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS		= readline.o

all:            ../libreadline.a

../libreadline.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
