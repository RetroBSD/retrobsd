LOBJS=fs.o util.o fops.o set.o
CFLAGS=-Wall -Werror -ggdb3
PROGS=umkfs umkdir uls uchmod uchown uchgrp ulogin ucd ucat
POBJS=umkfs.o umkdir.o uls.o uchmod.o uchown.o uchgrp.o ulogin.o ucd.o ucat.o
LDFLAGS=-L. -lufs -ggdb3
CC=gcc
ARFLAGS=-cr

LIB=libufs.a

all: $(LIB) $(PROGS)

$(LIB): $(LOBJS)
	$(AR) $(ARFLAGS) $(LIB) $(LOBJS)

$(PROGS): $(POBJS) $(LIB)
	$(CC) -o $@ $@.o $(LDFLAGS)

clean:
	rm -f $(LIB) $(LOBJS) $(PROGS) $(POBJS)
