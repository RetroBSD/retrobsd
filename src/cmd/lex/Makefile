CFLAGS = -O
SEPFLAG= -i

all: lex

lex: lmain.o y.tab.o sub1.o sub2.o header.o
	cc ${SEPFLAG} lmain.o y.tab.o sub1.o sub2.o header.o -o lex

smallex:
	cc ${SEPFLAG} lmain.o y.tab.o sub1.o sub2.o header.o -o lex

y.tab.c: parser.y
	yacc parser.y
lmain.o:lmain.c ldefs.c once.c
	cc -c -O lmain.c
sub1.o: sub1.c ldefs.c
	cc -c -O sub1.c
sub2.o: sub2.c ldefs.c
	cc -c -O sub2.c
header.o: header.c ldefs.c
	cc -c -O header.c

install: all
	-mkdir -p ${DESTDIR}/share/lex
	chmod 755 ${DESTDIR}/share/lex
	install -s -m 755 lex $(DESTDIR)/bin
	install -c -m 444 ncform $(DESTDIR)/share/lex
	install -c -m 444 nrform $(DESTDIR)/share/lex

clean:
	rm -f *.o lex y.tab.c
