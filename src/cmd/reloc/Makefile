CC = gcc
CFLAGS = -g -ansi -Wall
LIBS =

reloc: reloc.c
	$(CC) $(CFLAGS) -o reloc reloc.c

clean:
	-rm *core reloc reloc.o

shar: reloc.c makefile reloc.1 notes README
	shar reloc.c makefile reloc.1 notes README > reloc.shar
