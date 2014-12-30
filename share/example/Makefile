
all:	ashello echo chello stdarg skeleton

ashello: ashello.o
	$(LD) ashello.o -o $@

chello: chello.o
	$(CC) chello.o -o $@

echo:   echo.o
	$(LD) $@.o -o $@

stdarg: stdarg.o
	$(CC) stdarg.o -o $@

skeleton: skeleton.o
	$(CC) skeleton.o -o $@

clean:
	rm -f *.o ashello echo chello stdarg skeleton *.dis *~
