TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -O -Wall -Werror

OBJS    	= open.o clear.o pixel.o line.o rect.o fill.o circle.o \
                  image.o char.o text.o text_width.o

all:    	../libgpanel.a

../libgpanel.a: ${OBJS}
		@$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

install: 	all

clean:
		rm -f *.o a.out core test errs ../libgpanel*.a
