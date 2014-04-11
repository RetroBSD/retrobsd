#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= ${DEFS}

SRCS		= ${STDSRC} fgetc.c fgets.c fputc.c fputs.c gets.c puts.c \
                  feof.c ferror.c fileno.c
OBJS		= ${STDOBJ} fgetc.o fgets.o fputc.o fputs.o gets.o puts.o \
                  feof.o ferror.o fileno.o

STDSRC		= clrerr.c doscan.c exit.c fdopen.c filbuf.c findiop.c \
		  flsbuf.c fopen.c fprintf.c fread.c freopen.c fseek.c \
		  ftell.c fwrite.c getchar.c getw.c printf.c putchar.c putw.c \
		  rew.c scanf.c setbuf.c setbuffer.c setvbuf.c snprintf.c sprintf.c \
		  strout.c ungetc.c vfprintf.c vprintf.c vsprintf.c doprnt.c \
		  remove.c
STDOBJ		= clrerr.o doscan.o exit.o fdopen.o filbuf.o findiop.o \
		  flsbuf.o fopen.o fprintf.o fread.o freopen.o fseek.o \
		  ftell.o fwrite.o getchar.o getw.o printf.o putchar.o putw.o \
		  rew.o scanf.o setbuf.o setbuffer.o setvbuf.o snprintf.o sprintf.o \
		  strout.o ungetc.o vfprintf.o vprintf.o vsprintf.o doprnt.o \
		  remove.o

TAGSFILE	= tags

stdio.a:	${OBJS}
		@echo "building stdio.a"
		@ar cru stdio.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f stdio.a *.o *~ profiled/*.o tags Makefile.bak

depend:
		for i in ${SRCS}; do \
		    cc -M ${CFLAGS} $$i | awk ' { if ($$1 != prev) \
			{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
			else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
			END { print rec } ' >> makedep; done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
