CC		= gcc -g
CFLAGS		= -O -Wall
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
