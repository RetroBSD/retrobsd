TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

CFLAGS		+= ${DEFS} -Os -Wall -Werror -Wno-error=pointer-sign -Wno-pointer-sign
CFLAGS		+= -DTCL_FILE_CMDS -DTCL_ENV_CMDS

OBJS		= tclget.o tclproc.o tclvar.o tclassem.o \
		  tclcmdah.o tclcmdmz.o tclhash.o tclparse.o \
		  tclcmdil.o tclbasic.o tclexpr.o tclutil.o \
		  regexp.o regsub.o tclenv.o tclglob.o \
                  tclunxaz.o tcluxstr.o tcluxutl.o

../libtcl.a:    ${OBJS}
		$(AR) cru $@ ${OBJS}
		$(RANLIB) $@

clean:
		rm -f ../libtcl.a *.o *~

###
regexp.o: regexp.c regexp.h regpriv.h
regsub.o: regsub.c regexp.h regpriv.h
tclassem.o: tclassem.c internal.h hash.h
tclbasic.o: tclbasic.c internal.h hash.h
tclcmdah.o: tclcmdah.c internal.h hash.h
tclcmdil.o: tclcmdil.c internal.h hash.h
tclcmdmz.o: tclcmdmz.c internal.h hash.h regexp.h regpriv.h
tclenv.o: tclenv.c internal.h hash.h
tclexpr.o: tclexpr.c internal.h hash.h
tclget.o: tclget.c internal.h hash.h
tclglob.o: tclglob.c internal.h hash.h
tclhash.o: tclhash.c internal.h hash.h
tclparse.o: tclparse.c internal.h hash.h
tclproc.o: tclproc.c internal.h hash.h
tclunxaz.o: tclunxaz.c internal.h hash.h
tclutil.o: tclutil.c internal.h hash.h regexp.h
tcluxstr.o: tcluxstr.c internal.h hash.h
tcluxutl.o: tcluxutl.c internal.h hash.h
tclvar.o: tclvar.c internal.h hash.h
