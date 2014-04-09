#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# Machine dependent routines for the MIPS are located here
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

ALL		= gen string sys

mips.a:		${ALL}
		-mkdir tmp
		for i in ${ALL}; do (cd tmp; $(AR) x ../$$i/$$i.a); done
		$(AR) cr mips.a `ls tmp/*.o | sort`
		rm -rf tmp

${ALL}:	FRC
	cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:

tags:
		for i in ${ALL}; do \
			(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
		done

clean:
		for i in ${ALL}; do (cd $$i; make ${MFLAGS} clean); done
		rm -rf *.a tmp *~

depend:
		for i in ${ALL}; do \
			(cd $$i; make ${MFLAGS} DEFS=${DEFS} depend); done
