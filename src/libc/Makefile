#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
#
# The C run-time startup code is always machine dependent and expected
# to be located in ../startup-${MACHINE}.
#
# All files contain sccsid strings, but these are not compiled into
# library objects by default, as a space-saving measure.  To produce
# a library that contains these strings in every object except
# system call stubs, add -DLIBC_SCCS to DEFS below; to put these
# strings into system call stubs, use -DSYSLIBC_SCCS.
#
# To compile a non-floating point versions of some standard library
# routines add -DNONFP.  This will speed up some operations if you don't
# have hardware floating point.  To compile a non-separate I&D version add
# -DNONSEPARATE.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

DEFS		= -Wall -Werror
ALL		= gen stdio stdlib string inet compat ${MACHINE}

all:            ../libc.a

../libc.a:      ${ALL}
		rm -rf tmp
		mkdir tmp
		cd tmp; for i in ${ALL}; do $(AR) x ../$$i/$$i.a; done; \
			$(AR) cr ../$@ *.o
#			$(AR) cr ../$@ `sh ../../cmd/lorder.sh *.o | tsort`
		rm -rf tmp

${ALL}:		FRC
		cd $@; make ${MFLAGS} DEFS="${DEFS}"

FRC:

install:	../libc.a
		${INSTALLDIR} ${DESTDIR}/lib
#		${INSTALL} ../libc.a ${DESTDIR}/lib/libc.a
#		$(RANLIB) ${DESTDIR}/lib/libc.a

clean:
		for i in ${ALL}; \
			do (cd $$i; make ${MFLAGS} clean); done
		rm -rf tmp *.a *~

depend:
		for i in ${ALL}; \
			do (cd $$i; make ${MFLAGS} DEFS="${DEFS}" depend); done
