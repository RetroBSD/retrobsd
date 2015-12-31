CC		= gcc -g
CFLAGS		= -O -Wall
DESTDIR		= /usr/local
OBJS		= fsutil.o superblock.o block.c inode.o create.o check.o \
                  file.o mount.o manifest.o
PROG		= fsutil

# For Mac OS X
ifneq ($(wildcard /usr/local/lib/pkgconfig),)
    FUSE_PATH   = /usr/local/lib/pkgconfig
endif

# Fuse
MOUNT_CFLAGS    = $(shell PKG_CONFIG_PATH=$(FUSE_PATH) pkg-config fuse --cflags)
LIBS            += $(shell PKG_CONFIG_PATH=$(FUSE_PATH) pkg-config fuse --libs)

all:		$(PROG)

install:	$(PROG)
		install -s $(PROG) ${DESTDIR}/bin/$(PROG)
clean:
		rm -rf *~ *.o *.lst *.dis $(PROG) $(PROG).dSYM

$(PROG):	$(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

root.bin:       $(PROG)
		./$(PROG) -n16384 -s2048 $@
		./$(PROG) -v $@
		./$(PROG) -c $@

swap.bin:
		dd bs=1k count=2048 < /dev/zero > $@

mount.o: mount.c bsdfs.h
		$(CC) $(CFLAGS) $(MOUNT_CFLAGS) -c -o $@ $<

block.o: block.c bsdfs.h
check.o: check.c bsdfs.h
create.o: create.c bsdfs.h
file.o: file.c bsdfs.h
fsutil.o: fsutil.c bsdfs.h manifest.h
inode.o: inode.c bsdfs.h
manifest.o: manifest.c bsdfs.h manifest.h
superblock.o: superblock.c bsdfs.h
