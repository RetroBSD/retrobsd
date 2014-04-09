.SUFFIXES:      .o .c

.c.o:
        /u/clewis/lib/sccvax -c $*.c
        as -o $*.o $*.s

OBJ     = crunvax.o chiovax.o iovax.o

libl.a: $(OBJ) crt0.o
        ar ru libl.a $(OBJ)
        ucb ranlib libl.a
