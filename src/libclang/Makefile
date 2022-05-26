TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += ${DEFS} -Werror -Wall

OBJS            = adddf3.o \
                  addsf3.o \
                  divdf3.o \
                  divsf3.o \
                  comparedf2.o \
                  comparesf2.o \
                  extendsfdf2.o \
                  fixdfsi.o \
                  fixsfsi.o \
                  floatsidf.o \
                  floatsisf.o \
                  floatunsisf.o \
                  fp_mode.o \
                  muldf3.o \
                  mulsf3.o \
                  subdf3.o \
                  subsf3.o \
                  truncdfsf2.o

all:            ../libclang.a

../libclang.a:	${OBJS}
		$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

install:	all
#		${INSTALLDIR} ${DESTDIR}/lib
#		${INSTALL} ../libclang.a ${DESTDIR}/lib/libclang.a
#		$(RANLIB) ${DESTDIR}/lib/libclang.a

clean:
		rm -f ../libclang.a *.o *~ tags
