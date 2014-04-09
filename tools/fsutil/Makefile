CC		= gcc -g
CFLAGS		= -O -Wall
DESTDIR		= /usr/local
OBJS		= fsutil.o superblock.o block.c inode.o create.o check.o file.o
PROG		= fsutil

# For Mac OS X
#LIBS		= -largp

all:		$(PROG)

install:	$(PROG)
		install -s $(PROG) ${DESTDIR}/bin/$(PROG)
clean:
		rm -f *~ *.o *.lst *.dis $(PROG)

$(PROG):	$(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

root.bin:       $(PROG)
		./$(PROG) -n16384 -s2048 $@
		./$(PROG) -v $@
		./$(PROG) -c $@

swap.bin:
		dd bs=1k count=2048 < /dev/zero > $@

block.o: block.c bsdfs.h
check.o: check.c bsdfs.h
create.o: create.c bsdfs.h
file.o: file.c bsdfs.h
fsutil.o: fsutil.c bsdfs.h
inode.o: inode.c bsdfs.h
superblock.o: superblock.c bsdfs.h
