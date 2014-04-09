#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# Device "make" file.  Valid arguments:
#	std	standard devices
#	local	configuration specific devices
#	fd	file descriptor driver
# Disks:
#	sd*	flash cards SecureDigital
# Pseudo terminals:
#	pty*	set of 16 master and slave pseudo terminals

PATH=/etc:/sbin:/usr/sbin:/bin:/usr/bin
umask 77
for i
do
case $i in

std)
	mknod console	c 0 0
	mknod mem	c 1 0	; chmod 640 mem ; chgrp kmem mem
	mknod kmem	c 1 1	; chmod 640 kmem ; chgrp kmem kmem
	mknod null	c 1 2	; chmod 666 null
	mknod zero	c 1 3	; chmod 444 zero
	mknod tty	c 2 0	; chmod 666 tty
 	mknod klog	c 4 0	; chmod 600 klog
	mknod errlog	c 4 1	; chmod 600 errlog
	mknod acctlog	c 4 2	; chmod 600 acctlog
	;;

fd)
	umask 0
	rm -rf fd
	rm -f stdin stdout stderr
	mkdir fd
	chmod 755 fd
	mknod stdin c 5 0
	mknod stdout c 5 1
	mknod stderr c 5 2
	eval `echo "" | awk '{ for (i = 0; i < 32; i++)
			printf("mknod fd/%d c 5 %d; ",i,i); }'`
	;;

sd*)
	# The 2.11BSD sd driver doesn't support partitions.  We create
	# a single block and character inode pair for each unit and
	# call it sdNh.
	umask 2
        unit=`expr $i : '..\(.*\)'`
	mknod sd${unit}h b 0 ${unit}
	mknod rsd${unit}h c 3 ${unit}
	chgrp operator sd${unit}h rsd${unit}h
	chmod 640 sd${unit}h rsd${unit}h
	;;

local)
	sh MAKEDEV.local
	;;
esac
done
