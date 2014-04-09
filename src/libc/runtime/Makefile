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
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += ${DEFS}

OBJS            = addsf3.o comparesf2.o divsf3.o fixsfsi.o floatsisf.o \
                  mulsf3.o negsf2.o subsf3.o sc_case.o

all:		${OBJS}

clean:
		rm -f *.a *.o *~ profiled/*.o tags

cleandir:       clean
		rm -f .depend

install:        all
#		cp ../libgcc.a ${DESTDIR}/lib/
#		$(RANLIB) -t ${DESTDIR}/lib/libgcc.a
#		cp libgcc_p.a ${DESTDIR}/lib/
#		$(RANLIB) -t ${DESTDIR}/lib/libgcc_p.a
