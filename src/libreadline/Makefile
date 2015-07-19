TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -O -Wall -Werror

OBJS		= readline.o

all:            ../libreadline.a

../libreadline.a: ${OBJS}
		@$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

install: 	all readline.h history.h
		install -d $(DESTDIR)/include/readline/
		cp -p readline.h history.h $(DESTDIR)/include/readline/

clean:
		rm -f *~ *.o a.out ../libreadline*.a
