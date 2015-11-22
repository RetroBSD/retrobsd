TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

vpath %.c $(TOPSRC)/src/libgpanel

CFLAGS		= -Os -B$(TOPSRC)/lib/ $(DEFS) -Wa,-x -Wall -Werror

OBJS            = open.o clear.o pixel.o line.o rect.o fill.o fill_triangle.o \
                  circle.o image.o char.o text.o text_width.o

all:            ../libgpanel.a

../libgpanel.a: ../ar ../ranlib $(OBJS)
		../ar rc $@ $(OBJS)
		../ranlib $@

clean:
		rm -f *~ *.o a.out *.a
