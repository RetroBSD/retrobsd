CC		= gcc -g
CFLAGS		= -O -Wall -Werror
YACC            = byacc -d
DESTDIR		= /usr/local
OBJS            = config.o main.o lang.o mkioconf.o mkmakefile.o mkheaders.o \
                  mkswapconf.o
PROG		= kconfig

all:		$(PROG)

install:	$(PROG)
		install -s $(PROG) ${DESTDIR}/bin/$(PROG)
clean:
		rm -rf *~ *.o *.lst *.dis $(PROG) $(PROG).dSYM y.tab.[ch] lang.c config.c

$(PROG):	$(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

main.o: main.c y.tab.h config.h
mkheaders.o: mkheaders.c config.h y.tab.h
mkioconf.o: mkioconf.c y.tab.h config.h
mkmakefile.o: mkmakefile.c y.tab.h config.h
mkswapconf.o: mkswapconf.c config.h
