
all:	ashello echo

ashello: ashello.o
	$(LD) ashello.o -o $@

chello: chello.o
	$(CC) chello.o -o $@

echo:   echo.o
	$(LD) $@.o -o $@

clean:
	rm -f *.o ashello echo *.dis *~
