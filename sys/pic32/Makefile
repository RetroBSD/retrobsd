
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= explorer16 olimex

default:

all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i all; done

install:

clean:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean; done

clean-all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean-all; done

reconfig:
		-for i in $(SUBDIR); do ${MAKE} -C $$i reconfig; done
