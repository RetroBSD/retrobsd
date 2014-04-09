#!/bin/sh

# putman.sh - install a man page according to local custom
# vixie 27dec93 [original]
#
# $Id:$

PAGE=$1
DIR=$2

SECT=`expr $PAGE : '[a-z]*.\([0-9]\)'`
MDIR="$DIR/cat$SECT"
DEST="$MDIR/`basename $PAGE .$SECT`.0"

set -x
if [ ! -d $MDIR ]; then
	rm -f $MDIR
	mkdir -p $MDIR
	chmod 755 $MDIR
fi

nroff -man $PAGE >$DEST
chmod 644 $DEST
set +x

exit 0
