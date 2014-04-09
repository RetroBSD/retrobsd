#	@(#)Makefile	4.2	(Berkeley)	9/7/85
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

btlgammon: btlgammon.c
	cc ${SEPFLAG} ${CFLAGS} -o btlgammon btlgammon.c

install: btlgammon backrules
	install -s btlgammon ${DESTDIR}/usr/games/btlgammon
	install -c backrules ${DESTDIR}/usr/games/lib/backrules

clean:
	rm -f a.out core *.s *.o btlgammon

depend:
	cat </dev/null >x.c
	for i in btlgammon; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

