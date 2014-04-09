#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += ${DEFS}

SRCS            = login.c logout.c logwtmp.c
OBJS            = login.o logout.o logwtmp.o

all:            ../libutil.a

../libutil.a:	${OBJS}
		$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

clean:
		rm -f *.a *.o *~ profiled/*.o tags

cleandir:       clean
		rm -f .depend

depend:
		mkdep ${CFLAGS} ${SRCS}

install:        all
#		cp ../libutil.a ${DESTDIR}/lib/
#		$(RANLIB) -t ${DESTDIR}/lib/libutil.a
#		cp libutil_p.a ${DESTDIR}/lib/
#		$(RANLIB) -t ${DESTDIR}/lib/libutil_p.a

tags:
		ctags ${SRCS}
