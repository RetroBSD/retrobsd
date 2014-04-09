CC		= gcc -g
CFLAGS		= -O -Wall
DESTDIR		= /usr/local
OBJS		= elf2aout.o
PROG		= elf2aout

# For Mac OS X
#LIBS		= -largp

all:		$(PROG)

install:	$(PROG)
		install -s $(PROG) ${DESTDIR}/bin/$(PROG)
clean:
		rm -f *~ *.o *.lst *.dis $(PROG)

$(PROG):	$(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
