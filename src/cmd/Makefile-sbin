#
# Public domain - 1996/10/26 - sms
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

SUBDIR		= chown-src chroot-src disktool-src fsck-src getty-src init-src \
                  mkfs-src mknod-src mkpasswd-src mount-src pstat-src \
                  reboot-src shutdown-src umount-src update-src vipw-src

all:		${SUBDIR}

${SUBDIR}:	FRC
		cd $@; make ${MFLAGS} all

FRC:

install:	FRC
		-for i in ${SUBDIR}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
		rm -f *~
